#pragma once

#include <smtrat-common/smtrat-common.h>
#include "variableordering.h"

#include <vector>

namespace smtrat::cad::variable_ordering {


    struct ChordalOrderingSettings {
        /**
         * @brief If the ratio of fill edges to initial edges exceeds this threshold,
         * the construction of an elimination tree is not attempted.
         * 
         */
        double etree_fill_threshold;

        /**
         * @brief If the ratio of fill edges to initial edges exceeds this threshold
         * and @p alternative_order is not null,
         * the determined elimination order will not be used. Instead, 
         * @p alternative_order is invoked.
         * 
         */
        double alt_order_fill_threshold;

        /**
         * @brief If not null, the variables with same rank within an elimination tree
         * will be ordered according to this order
         * 
         */
        VariableOrdering etree_secondary_order;

        /**
         * @brief If not null, this will be used as an ordering if the ratio of fill to initial edges
         * exceeds @p etree_fill_threshold
         * 
         */
        VariableOrdering alternative_order;

    };

    constexpr ChordalOrderingSettings paper_settings = {
        .etree_fill_threshold = 0,
        .alt_order_fill_threshold = 0,
        .etree_secondary_order = nullptr,
        .alternative_order = nullptr,
    };

std::vector<carl::Variable> chordal_vargraph_elimination_ordering(const std::vector<Poly>& polys);

}
