
#include <smtrat-common/smtrat-common.h>
#ifdef SMTRAT_DEVOPTION_Statistics
#include "CADVOStatistics.h"
#include <smtrat-cad/CAD.h>
#include <smtrat-analyzer/analyzers/utils.h>

namespace smtrat::cad::variable_ordering {

    struct DegreeCollector {

        struct Statistics {
            std::size_t max;
            std::size_t min;
            std::size_t sum;
        };

        Statistics degree, tdegree;


        void operator()(const Poly& p) {
            degree.max = std::max(degree.max, carl::total_degree(p));
            degree.min = std::min(degree.min, carl::total_degree(p));
            degree.sum += carl::total_degree(p);
            tdegree.max = std::max(tdegree.max, carl::total_degree(p));
            tdegree.sum += carl::total_degree(p);
            tdegree.min = std::min(tdegree.min, carl::total_degree(p));
        }
        void operator()(const carl::UnivariatePolynomial<Poly>& p) {
            degree.max = std::max(degree.max, p.degree());
            degree.min = std::min(degree.min, p.degree());
            degree.sum += p.degree();
        
            tdegree.max = std::max(tdegree.max, carl::total_degree(p));
            tdegree.sum += carl::total_degree(p);
            tdegree.min = std::min(tdegree.min, carl::total_degree(p));
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
            Poly prod(1);
            for (std::size_t pid = 0; pid < proj.size(level); ++pid) {
                if (proj.hasPolynomialById(level, pid)) {
                    const auto& p = proj.getPolynomialById(level, pid);
                    if (!carl::is_zero(p))
                        prod *= Poly(p);
                    dc(p);
                }
            }

            
            if constexpr (CADVOStatistics::detail_level >= CADVOStatistics::DetailLevel::HIGH) {

                int combined_degree = 0;
                for (auto var : carl::variables(prod)) {
                    int degree = prod.degree(var);
                    if (degree > combined_degree) {
                        combined_degree = degree;
                    }
                }
                this->_add((std::stringstream() << "projection." << level << ".combined_degree").str(), combined_degree);
                this->_add((std::stringstream() << "projection." << level << ".size").str(), proj.size(level));
                this->_add((std::stringstream() << "projection." << level << ".deg_max").str(), dc.degree.max);
                this->_add((std::stringstream() << "projection." << level << ".deg_min").str(), dc.degree.min);
                this->_add((std::stringstream() << "projection." << level << ".deg_sum").str(), dc.degree.sum);
                this->_add((std::stringstream() << "projection." << level << ".tdeg_max").str(), dc.tdegree.max);
                this->_add((std::stringstream() << "projection." << level << ".tdeg_min").str(), dc.tdegree.min);
                this->_add((std::stringstream() << "projection." << level << ".tdeg_sum").str(), dc.tdegree.sum);
                dc = {};
            }
        }

        /*
        if constexpr (CADVOStatistics::detail_level <= CADVOStatistics::DetailLevel::NORMAL) {
            this->_add("projection.size", sum);
            this->_add("projection.deg_max", dc.degree_max);
            this->_add("projection.deg_sum", dc.degree_sum);
            this->_add("projection.tdeg_max", dc.tdegree_max);
            this->_add("projection.tdeg_sum", dc.tdegree_sum);
        }*/
    }
}
#endif