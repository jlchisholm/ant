#include "CalibType.h"

#include "tree/TCalibrationData.h"
#include "base/Logger.h"
#include "DataManager.h"

#include "TH1.h"

using namespace std;
using namespace ant;
using namespace ant::calibration;

double CalibType::Get(unsigned channel) const {

    if(Values.empty()) {
        if(DefaultValues.size() == 1) {
            return DefaultValues.front();
        }
        else {
            return DefaultValues.at(channel);
        }
    }
    else {
        return Values.at(channel);
    }
}

CalibType::CalibType(
        const std::shared_ptr<const Detector_t>& det,
        const string& name,
        const std::vector<double>& defaultValues,
        const string& histname) :
    Name(name),
    // use name for histogram if not provided different
    HistogramName(histname.empty() ? name : histname),
    Values(),
    DefaultValues(defaultValues)
{
    if(DefaultValues.size() != 1 && DefaultValues.size() != det->GetNChannels()) {
        throw runtime_error("Wrong size of default values for calibType="+name+" det="+Detector_t::ToString(det->Type));
    }
}

GUI_CalibType::GUI_CalibType(const string& basename, OptionsPtr opts,
                                     CalibType& type,
                                     const shared_ptr<DataManager>& calmgr,
                                     const detector_ptr_t& detector_,
                                     Calibration::AddMode_t mode) :
    gui::CalibModule_traits(basename),
    options(opts),
    calibType(type),
    calibrationManager(calmgr),
    detector(detector_),
    addMode(mode)
{}

string GUI_CalibType::GetName() const
{
    // serves as the CalibrationID for the manager
    return  CalibModule_traits::GetName()+"_"+calibType.Name;
}

shared_ptr<TH1> GUI_CalibType::GetHistogram(const WrapTFile& file) const
{
    // histogram name created by the specified Physics class
    return file.GetSharedHist<TH1>(options->Get<string>("HistogramPath", CalibModule_traits::GetName()) + "/"+calibType.HistogramName);
}

unsigned GUI_CalibType::GetNumberOfChannels() const
{
    return detector->GetNChannels();
}

void GUI_CalibType::InitGUI(gui::ManagerWindow_traits& window) {
    window.AddCheckBox("Ignore prev fit params", IgnorePreviousFitParameters);
    window.AddCheckBox("Use params from prev slice", UsePreviousSliceParams);
}

void GUI_CalibType::StartSlice(const interval<TID>& range)
{
    // clear previous values from slice first
    // then calibType.Get(ch) will return default value
    calibType.Values.clear();
    std::vector<double> values(GetNumberOfChannels());
    for(size_t ch=0; ch<values.size(); ++ch) {
        values.at(ch) = calibType.Get(ch);
    }

    TCalibrationData cdata;
    if(calibrationManager->GetData(GetName(), range.Start(), cdata)) {
        for(const TKeyValue<double>& kv : cdata.Data) {
            if(kv.Key>=GetNumberOfChannels()) {
                LOG(WARNING) << "Ignoring too large key " << kv.Key << " in TCalibrationData";
                continue;
            }
            values.at(kv.Key) = kv.Value;
        }
        LOG(INFO) << GetName() << ": Loaded previous values from database";

        // fill the map of fitparameters
        if(fitParameters.empty() || !UsePreviousSliceParams) {
            for(const TKeyValue<vector<double>>& kv : cdata.FitParameters) {
                if(kv.Key>=GetNumberOfChannels()) {
                    LOG(WARNING) << "Ignoring too large key " << kv.Key << " in TCalibrationData fit parameters";
                    continue;
                }
                // do not use at() as kv.Key might not yet exist in map
                fitParameters[kv.Key] = kv.Value;
            }
            LOG(INFO) << GetName() << ": Loaded previous fit parameter from database";
        }
        else if(!fitParameters.empty()) {
            LOG(INFO) << GetName() << ": Using fit parameters from previous slice";
        }
    }
    else {
        LOG(INFO) << GetName() << ": No previous values found, built from default value";
    }

    calibType.Values = values;

    // save a copy for comparison at finish stage
    previousValues = calibType.Values;

}

template<typename T>
bool hasKey(const std::vector<T>& v, unsigned key) {
    auto it = std::find_if(v.begin(), v.end(), [key] (const T& kv) {
        return kv.Key == key;
    });
    return it != v.end();
}


template<typename T>
const T& getByKey(const std::vector<T>& v, unsigned key) {
    auto it = std::find_if(v.begin(), v.end(), [key] (const T& kv) {
        return kv.Key == key;
    });
    if(it == v.end())
        throw std::out_of_range("Could not find entry with Key="+to_string(key));
    return *it;
}

void GUI_CalibType::StoreFinishSlice(const interval<TID>& range)
{
    TCalibrationData cdata(
                GetName(),
                range.Start(),
                range.Stop()
                );

    // check if there's an default for NoCalibUseDefault element flag
    // but only if we're actually running on DataRanges
    TCalibrationData cdata_default;
    const bool disableDataDefault = calibrationManager->GetOverrideToDefault() || range.Start().isSet(TID::Flags_t::MC);
    const bool haveDataDefault = !disableDataDefault && calibrationManager->GetData(GetName(), TID(0,0), cdata_default);

    // fill calibration data
    for(unsigned ch=0;ch<calibType.Values.size();ch++) {

        if(!disableDataDefault) {
            // not running on MC and not running DataDefault, so we run on ranges...
            if( detector->HasElementFlags(ch, Detector_t::ElementFlag_t::NoCalibUseDefault) &&
                !detector->HasElementFlags(ch, Detector_t::ElementFlag_t::NoCalibFill)) {
                if(haveDataDefault && hasKey(cdata_default.Data, ch)) {
                    // do special handling for NoCalibUseDefault
                    cdata.Data.emplace_back(getByKey(cdata_default.Data, ch));
                    if(hasKey(cdata_default.FitParameters, ch))
                        cdata.FitParameters.emplace_back(getByKey(cdata_default.FitParameters, ch));
                    VLOG(2) << "Channel " << ch << " stored with value " << cdata.Data.back().Value
                            << " from default calibration due to element flag NoCalibUseDefault";

                    continue;
                }
                else {
                    LOG(WARNING) << "Default calibrated value for channel=" << ch << " not found, "
                                 << "flag NoCalibUseDefault will not have any effect";
                }
            }
        }

        // normal handling of values/fit parameters,
        // ignore missing fit parameters silently (modules are not required to use this)
        cdata.Data.emplace_back(ch, calibType.Values[ch]);
        auto it_params = fitParameters.find(ch);
        if(it_params != fitParameters.end()) {
            cdata.FitParameters.emplace_back(ch, it_params->second);
        }
    }

    calibrationManager->Add(cdata, addMode);
}