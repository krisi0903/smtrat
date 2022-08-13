#pragma once

#include <smtrat-common/smtrat-common.h>

#include <vector>

namespace smtrat::cad::variable_ordering {

std::vector<carl::Variable> chordal_vargraph_elimination_ordering(const std::vector<Poly>& polys);

}
