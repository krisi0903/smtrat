#include "triangular_ordering.h"

#include <algorithm>
#include <numeric>
#include <random>

namespace smtrat::cad::variable_ordering {

std::vector<carl::Variable> pseudorandom_ordering(const std::vector<Poly>& polys) {
	SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Building order based on " << polys);
    size_t seed = 0;
    for (Poly const& p : polys) {
        seed ^= std::hash<Poly>()(p);
    }


    carl::carlVariables vars;
    for (Poly const& p : polys) {
        carl::variables(p, vars);
    }
    SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Seed value for RNG is " << seed);

    std::default_random_engine rng(seed);
    std::uniform_int_distribution dist;

    std::vector<carl::Variable> res = vars.as_vector();
    std::shuffle(res.begin(), res.end(), dist);

	SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Sorted: " << res);
	return res;
}

}