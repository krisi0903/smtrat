#pragma once

#include <smtrat-common/smtrat-common.h>
#include "variableordering.h"
#include "triangular_ordering.h"

#include <vector>

namespace smtrat::cad::variable_ordering {

    constexpr bool debugChordalVargraph = false;

    struct ChordalOrderingSettingsBase {
        /**
         * @brief If the ratio of fill edges to initial edges exceeds this threshold,
         * the construction of an elimination tree is not attempted.
         * 
         */
        static constexpr double etree_fill_threshold = 0;

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
        static constexpr VariableOrdering etree_secondary_ordering = nullptr;

        /**
         * @brief If not null, this will be used as an ordering if the ratio of fill to initial edges
         * exceeds @p etree_fill_threshold
         * 
         */
        static constexpr VariableOrdering alternative_ordering = nullptr;

    };

    struct ChordalOrderingSettingsTriangularETree : ChordalOrderingSettingsBase {
        static constexpr VariableOrdering etree_secondary_ordering = &triangular_ordering;
    };

    template<typename Settings>
    std::vector<carl::Variable> chordal_vargraph_elimination_ordering(const std::vector<Poly>& polys);

}
