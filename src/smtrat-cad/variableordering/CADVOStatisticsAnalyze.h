
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
        switch (CADVOStatistics::detail_level) {
            case CADVOStatistics::DetailLevel::HIGH:
            for (std::size_t level = 1; level <= cad.dim(); level++) {
                _add((std::stringstream() << "samples." << level << ".size").str(), std::distance(tree.begin_depth(level), tree.end_depth()));
            }
            break;
            case CADVOStatistics::DetailLevel::NORMAL:
                _add((std::stringstream() << "samples.size").str(), std::distance(tree.begin(), tree.end()));
        }
        
        
    }

    template <typename Settings>
    void CADVOStatistics::collectProjectionInformation(smtrat::cad::CAD<Settings> const& cad) {
        
        auto const& proj = cad.getProjection();
        DegreeCollector dc;

        std::size_t sum = 0;
        for (std::size_t level = 1; level <= proj.dim(); ++level) {
            sum += proj.size(level);
            for (std::size_t pid = 0; pid < proj.size(level); ++pid) {
                if (proj.hasPolynomialById(level, pid)) {
                    const auto& p = proj.getPolynomialById(level, pid);
                    dc(p);
                }
            }
            if constexpr (CADVOStatistics::detail_level >= CADVOStatistics::DetailLevel::HIGH) {
                this->_add((std::stringstream() << "projection." << level << ".size").str(), proj.size(level));
                this->_add((std::stringstream() << "projection." << level << ".deg_max").str(), dc.degree_max);
                this->_add((std::stringstream() << "projection." << level << ".deg_sum").str(), dc.degree_sum);
                this->_add((std::stringstream() << "projection." << level << ".tdeg_max").str(), dc.tdegree_max);
                this->_add((std::stringstream() << "projection." << level << ".tdeg_sum").str(), dc.tdegree_sum);
                dc = {};
            }
        }

        if constexpr (CADVOStatistics::detail_level <= CADVOStatistics::DetailLevel::NORMAL) {
            this->_add("projection.size", sum);
            this->_add("projection.deg_max", dc.degree_max);
            this->_add("projection.deg_sum", dc.degree_sum);
            this->_add("projection.tdeg_max", dc.tdegree_max);
            this->_add("projection.tdeg_sum", dc.tdegree_sum);
        }
    }
}
#endif