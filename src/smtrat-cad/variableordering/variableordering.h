#include <vector>
#include <smtrat-common/smtrat-common.h>
#include <smtrat-cad/variableordering/triangular_ordering.h>
#include <smtrat-cad/variableordering/pseudorandom_ordering.h>
#include <smtrat-cad/variableordering/chordal_vargraph_elimination_ordering.h>

namespace smtrat::cad::variable_ordering {
    typedef std::vector<carl::Variable> (*VariableOrdering)(const std::vector<Poly>& polys);
}