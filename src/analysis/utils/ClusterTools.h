#pragma once

#include "base/Detector_t.h"

#include <memory>
#include <map>

namespace ant {

class TCluster;

namespace analysis {
namespace utils {

struct ClusterTools {

    ClusterTools();

    /**
     * @brief Calculate the Lateral Moment of a cluster
     * @param cluster
     * @return
     *
     * \f[
     *      lat = \frac{\sum_{i=2}^{n} E_i r_i^2 }{ (\sum_{i=2}^{n} E_i r_i^2 + r_0^0 E_0 + r_0^2 E_1) }
     * \f]
     * where \f$ E_{0} \f$ and \f$ E_{1} \f$ are the two hits highest energies.
     *
     * Reimplemented after BaBar
     */
     double LateralMoment(const ant::TCluster& cluster) const;

protected:
     struct det_t {
         using det_ptr_t = std::shared_ptr<ClusterDetector_t>;
         det_t(const det_ptr_t& det);
         det_ptr_t Detector;
         std::vector<double> R0; // element-wise R0
     };

     std::map<Detector_t::Type_t, det_t> cluster_detectors;
};

}}}
