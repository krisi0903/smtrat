#pragma once

#include <smtrat-common/smtrat-common.h>
#include "variableordering.h"
#include "triangular_ordering.h"

#include <vector>

namespace smtrat::cad::variable_ordering {

    constexpr bool debugChordalVargraph = false;

    struct ChordalOrderingSettingsBase {
        static constexpr bool do_etree = true;
        /**
         * @brief If the ratio of fill edges to initial edges exceeds this threshold
         * and @p alternative_order is not null,
         * the determined elimination order will not be used. Instead, 
         * @p alternative_order is invoked.
         * 
         */
        static constexpr double alt_order_fill_threshold = 0;

        /**
         * @brief If not null, the variables with same rank within an elimination tree
         * will be ordered according to this order
         * 
         */
        static constexpr VariableOrdering etree_secondary_ordering = &triangular_ordering;

        /**
         * @brief If not null, this will be used as an ordering if the ratio of fill to initial edges
         * exceeds @p etree_fill_threshold, or if the graph is non-connected
         * 
         */
        static constexpr VariableOrdering alternative_ordering = &triangular_ordering;

        /**
         * @brief The ordering after which to choose variables in MCS-M as a tie breaker
         * 
         */
        static constexpr VariableOrdering mcs_m_secondary = &triangular_ordering;

    };

    struct ChordalOrderingSettingsNoETree : ChordalOrderingSettingsBase {
        static constexpr bool do_etree = false;
    };
    template<typename Settings>
    std::vector<carl::Variable> chordal_vargraph_elimination_ordering(const std::vector<Poly>& polys);


    std::vector<carl::Variable> degree_minimizing_fill(std::vector<Poly> const& polys);
}
