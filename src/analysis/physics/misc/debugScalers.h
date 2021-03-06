#pragma once

#include "analysis/physics/Physics.h"
#include "base/WrapTTree.h"

class TH1D;

namespace ant {
namespace analysis {
namespace physics {


struct debugScaler: public Physics {

    unsigned seenEvents = 0;

    struct TreeScalarReads : WrapTTree {
        ADD_BRANCH_T(TID,   LastID)
        ADD_BRANCH_T(int,   nEvtsPerRead)

        ADD_BRANCH_T(double,    ExpLivetime)
        ADD_BRANCH_T(double,    ExpTriggerRate)
        ADD_BRANCH_T(double,    L1TriggerRate)
        ADD_BRANCH_T(double,       Exp1MHz)

        ADD_BRANCH_T(double,   BeamPolMon1MHz)

        ADD_BRANCH_T(double,              PbRate)
        ADD_BRANCH_T(std::vector<double>, TaggRates)

        ADD_BRANCH_T(std::vector<int>,                  TDCHits)
        ADD_BRANCH_T(std::vector<int>,                  CoincidentTDCHits)
        ADD_BRANCH_T(std::vector<std::vector<double>>,  TaggTimings)

    };

    TreeScalarReads scalerReads;

    debugScaler(const std::string& name, OptionsPtr opts=nullptr);
    virtual ~debugScaler();

    virtual void ProcessEvent(const TEvent& ev, manager_t&) override;
    virtual void Finish() override;
    virtual void ShowResult() override;

    void processBlock(const TEvent& ev);
    void processTaggerHits(const TEvent& ev);
};

}}} // namespace ant::analysis::physics
