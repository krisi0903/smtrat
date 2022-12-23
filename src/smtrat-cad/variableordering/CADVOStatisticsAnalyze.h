
#include <smtrat-common/smtrat-common.h>
#ifdef SMTRAT_DEVOPTION_Statistics
#include "CADVOStatistics.h"
#include <smtrat-cad/CAD.h>
#include <smtrat-analyzer/analyzers/utils.h>

namespace smtrat::cad::variable_ordering {

    struct DegreeCollector {
        std::size_t degree_max = 0;
        std::size_t degree_sum = 0;
        std::size_t tdegree_max = 0;
        std::size_t tdegree_sum = 0;

        void operator()(const Poly& p) {
            degree_max = std::max(degree_max, carl::total_degree(p));
            degree_sum += carl::total_degree(p);
            tdegree_max = std::max(tdegree_max, carl::total_degree(p));
            tdegree_sum += carl::total_degree(p);
        }
        void operator()(const carl::UnivariatePolynomial<Poly>& p) {
            degree_max = std::max(degree_max, p.degree());
            degree_sum += p.degree();
            tdegree_max = std::max(tdegree_max, carl::total_degree(p));
            tdegree_sum += carl::total_degree(p);
        }
        void operator()(const ConstraintT& c) {
            (*this)(c.lhs());
        }
    };

    template <typename Settings>
    void CADVOStatistics::collectLiftingInformation(smtrat::cad::CAD<Settings> const& cad) {
        auto const& tree = cad.getLifting().getTree();
        _add("fullDimSamples", std::distance(tree.begin_depth(cad.dim()), tree.end_depth()));
    }

    template <typename Settings>
    void CADVOStatistics::collectProjectionInformation(smtrat::cad::CAD<Settings> const& cad) {
        auto const& proj = cad.getProjection();
        for (std::size_t level = 1; level <= proj.dim(); ++level) {
            DegreeCollector dc;
            for (std::size_t pid = 0; pid < proj.size(level); ++pid) {
                if (proj.hasPolynomialById(level, pid)) {
                    const auto& p = proj.getPolynomialById(level, pid);
                    dc(p);
                }
            }
            this->_add((std::stringstream() << "projection." << level << ".size").str(), proj.size(level));
            this->_add((std::stringstream() << "projection." << level << ".deg_max").str(), dc.degree_max);
            this->_add((std::stringstream() << "projection." << level << ".deg_sum").str(), dc.degree_sum);
            this->_add((std::stringstream() << "projection." << level << ".tdeg_max").str(), dc.tdegree_max);
            this->_add((std::stringstream() << "projection." << level << ".tdeg_sum").str(), dc.tdegree_sum);
        }
    }
}
#endif