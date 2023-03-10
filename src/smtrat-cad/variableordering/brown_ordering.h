#pragma once

#include <smtrat-common/smtrat-common.h>

#include <vector>

namespace smtrat::cad::variable_ordering {

std::vector<carl::Variable> brown_ordering(const std::vector<Poly>& polys);

}
