#pragma once

#include <smtrat-common/smtrat-common.h>

#include <vector>

namespace smtrat::cad::variable_ordering {

/**
 * @brief Pseudo-random variable ordering solely based on input polynomials
 * Seeds a PRNG with a value computed only from the input polynomials which is used
 * to shuffle the variables.
 * The rationale behind this is to achieve an ordering that performs very similar
 * to a complete random shuffle of the variables while always returning the same ordering
 * for the same polynomial set, which aids in giving more reproducible results
 * 
 * @param polys 
 * @return std::vector<carl::Variable> 
 */
std::vector<carl::Variable> pseudorandom_ordering(const std::vector<Poly>& polys);

}
