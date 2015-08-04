#include "PID_Energy.h"
#include "analysis/plot/HistogramFactories.h"
#include "analysis/data/Event.h"
#include "analysis/utils/combinatorics.h"

#include "expconfig/detectors/PID.h"

#include "tree/TDataRecord.h"

#include <list>


using namespace std;
using namespace ant;
using namespace ant::calibration;

PID_Energy::PID_Energy(
        std::shared_ptr<expconfig::detector::PID> pid,
        std::shared_ptr<CalibrationDataManager> calmgr,
        Calibration::Converter::ptr_t converter,
        double defaultPedestal,
        double defaultGain,
        double defaultThreshold,
        double defaultRelativeGain
        ) :
    Energy(pid->Type,
           calmgr,
           converter,
           defaultPedestal,
           defaultGain,
           defaultThreshold,
           defaultRelativeGain),
    pid_detector(pid)
{

}

PID_Energy::ThePhysics::ThePhysics(const string& name, unsigned nChannels):
    Physics(name)
{
    const BinSettings pid_channels(nChannels);
    const BinSettings rawbins(1000);

    pedestals = HistFac.makeTH2D("PID Pedestals", "Raw ADC value", "#", rawbins, pid_channels, "pedestals");
}

void PID_Energy::ThePhysics::ProcessEvent(const Event& event)
{

}

void PID_Energy::ThePhysics::Finish()
{
}

void PID_Energy::ThePhysics::ShowResult()
{
    canvas(GetName()) << drawoption("colz") << pedestals << endc;
}

PID_Energy::~PID_Energy()
{

}

unique_ptr<Physics> PID_Energy::GetPhysicsModule()
{
    return std_ext::make_unique<ThePhysics>(GetName(), pid_detector->GetNChannels());
}
