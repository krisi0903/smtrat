#include <vector>
#include <smtrat-common/smtrat-common.h>
#include <smtrat-cad/variableordering/triangular_ordering.h>
#include <smtrat-cad/variableordering/pseudorandom_ordering.h>
#include <smtrat-cad/variableordering/chordal_vargraph_elimination_ordering.h>
#include <smtrat-modules/NewCADModule/NewCADStatistics.h>

namespace smtrat::cad::variable_ordering {

    // A variable ordering is always described by a function that takes the vector of polynomials
    // and returns a vector of variables in the desired order

    typedef std::vector<carl::Variable> (*VariableOrdering)(const std::vector<Poly>& polys);
}