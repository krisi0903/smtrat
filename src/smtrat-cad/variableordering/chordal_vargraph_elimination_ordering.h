#pragma once

#include <smtrat-common/smtrat-common.h>
#include "variableordering.h"
#include "triangular_ordering.h"

#include <vector>

namespace smtrat::cad::variable_ordering {

    constexpr bool debugChordalVargraph = false;

    struct ChordalOrderingSettingsBase {
        static constexpr bool do_etree = true;



        static constexpr VariableOrdering alternative_ordering = &triangular_ordering;

        /**
         * @brief If not null, this will be used as an ordering whenever there are multiple choices for the next variable
         * under the regular chordal ordering. Otherwise, that is undefined
         * 
         */
        static constexpr VariableOrdering secondary_ordering = nullptr;
    };


    struct ChordalOrderingSettingsTriangular : ChordalOrderingSettingsBase {
        static constexpr VariableOrdering secondary_ordering = &triangular_ordering;
    };

    struct ChordalOrderingSettingsNoETree : ChordalOrderingSettingsBase {
        static constexpr bool do_etree = false;
    };
    template<typename Settings>
    std::vector<carl::Variable> chordal_vargraph_elimination_ordering(const std::vector<Poly>& polys);

    template<bool find_peo>
    std::vector<carl::Variable> degree_minimizing_fill(std::vector<Poly> const& polys);
}
