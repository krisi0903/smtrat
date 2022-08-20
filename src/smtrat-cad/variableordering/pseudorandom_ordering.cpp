#include "pseudorandom_ordering.h"

#include <algorithm>
#include <numeric>
#include <random>

namespace smtrat::cad::variable_ordering {

std::vector<carl::Variable> pseudorandom_ordering(const std::vector<Poly>& polys) {
	SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Building order based on " << polys);

    // construct a random seed from the polynomial hashes
    // Starting value has no special meaning, just choose something non-zero
    // otherwise we will always get 0
    size_t seed = 0xcafecafecafecafe;
    for (Poly const& p : polys) {
        seed += std::hash<Poly>()(p) * seed;
    }


    carl::carlVariables vars;
    for (Poly const& p : polys) {
        carl::variables(p, vars);
    }
    SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Seed value for RNG is " << seed);

    std::default_random_engine rng(seed);

    std::vector<carl::Variable> res = vars.as_vector();
    std::shuffle(res.begin(), res.end(), rng);

	SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Sorted: " << res);
	return res;
}

}