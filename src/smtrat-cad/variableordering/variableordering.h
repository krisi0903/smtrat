#pragma once
#include <vector>
#include <smtrat-common/smtrat-common.h>

namespace smtrat::cad::variable_ordering {
    // A variable ordering is always described by a function that takes the vector of polynomials
    // and returns a vector of variables in the desired order

    typedef std::vector<carl::Variable> (*VariableOrdering)(const std::vector<Poly>& polys);
}