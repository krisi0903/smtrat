
#include <smtrat-common/smtrat-common.h>
#ifdef SMTRAT_DEVOPTION_Statistics
#include "CADVOStatistics.h"
#include <smtrat-cad/CAD.h>

namespace smtrat::cad::variable_ordering {
        template <typename Settings>
        void CADVOStatistics::collectLiftingInformation(smtrat::cad::CAD<Settings> const& cad) {
            auto const& tree = cad.getLifting().getTree();
            _add("fullDimSamples", std::distance(tree.begin_depth(cad.dim()), tree.end_depth()));
        }

        template <typename Settings>
        void CADVOStatistics::collectProjectionInformation(smtrat::cad::CAD<Settings> const& cad) {
            auto const& proj = cad.getProjection();
            for (std::size_t level = 1; level <= proj.dim(); ++level) {
                std::stringstream key;
                key << "projection." << level << ".size";
                this->_add(key.str(), proj.size(level));
            }
            /*
            std::size_t sum = 0;
            DegreeCollector dc;
            for (std::size_t level = 1; level <= projection.mProjection.dim(); ++level) {
                sum += proj.size(level);
                for (std::size_t pid = 0; pid < projection.mProjection.size(level); ++pid) {
                    if (projection.mProjection.hasPolynomialById(level, pid)) {
                        const auto& p = projection.mProjection.getPolynomialById(level, pid);
                        dc(p);
                    }
                }
            }
            stats.add(prefix + "_size", sum);
            stats.add(prefix + "_deg_max", dc.degree_max);
            stats.add(prefix + "_deg_sum", dc.degree_sum);
            stats.add(prefix + "_tdeg_max", dc.tdegree_max);
            stats.add(prefix + "_tdeg_sum", dc.tdegree_sum);
            */
        }
}
#endif