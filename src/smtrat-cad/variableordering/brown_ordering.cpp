#include "brown_ordering.h"
#include "variableordering.h"
#include "pseudorandom_ordering.h"
#include "util.h"
#include <algorithm>
#include <numeric>

namespace smtrat::cad::variable_ordering {

template <typename Compare>
struct brown_data {
	VariableMap<std::size_t> max_deg;
	VariableMap<std::size_t> max_term_tdeg;
	VariableMap<std::size_t> term_count;
	
	Compare stable_var_comp;

	brown_data(Compare stable_var_comp) : stable_var_comp(stable_var_comp) {}

	bool operator()(carl::Variable lhs, carl::Variable rhs) const {
		SMTRAT_LOG_TRACE("smtrat.cad.variableordering", "Comparing maxdeg " << lhs << " < " << rhs << "? " << max_deg[lhs] << " > " << max_deg[rhs]);
		if (max_deg[lhs] != max_deg[rhs]) return max_deg[lhs] < max_deg[rhs];
		SMTRAT_LOG_TRACE("smtrat.cad.variableordering", "Comparing max_term_tdeg " << lhs << " < " << rhs << "? " << max_term_tdeg[lhs] << " > " << max_term_tdeg[rhs]);
		if (max_term_tdeg[lhs] != max_term_tdeg[rhs]) return max_term_tdeg[lhs] < max_term_tdeg[rhs];
		SMTRAT_LOG_TRACE("smtrat.cad.variableordering", "Comparing term_count " << lhs << " < " << rhs << "? " << term_count[lhs] << " > " << term_count[rhs]);
		if (term_count[lhs] != term_count[rhs]) return term_count[lhs] < term_count[rhs];
		return stable_var_comp(lhs, rhs);
	}
};

std::vector<carl::Variable> brown_ordering(const std::vector<Poly>& polys) {
	SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Building order based on " << polys);
	carl::carlVariables vars;
	brown_data data(vec_order_comp(pseudorandom_ordering(polys)));

	for (std::size_t i = 0; i < polys.size(); ++i) {
		for (Poly::TermType const& term : polys[i].terms()) {
			std::shared_ptr<const carl::Monomial> monomial = term.monomial();
			if (monomial != nullptr) {
				for (auto var : carl::variables(*monomial)) {
					data.term_count[var]++;
					data.max_term_tdeg[var] = std::max(data.max_term_tdeg[var], monomial->tdeg());
				}
			}
			
		}
		carl::variables(polys[i], vars);
		for (auto var: carl::variables(polys[i])) { 
			data.max_deg[var] = std::max(data.max_deg[var], polys[i].degree(var));
		}
	}
	SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Collected variables: " << vars);
	std::vector<carl::Variable> res(std::move(vars.as_vector()));
	std::sort(res.begin(), res.end(), data);
	SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Sorted: " << res);
	return res;
}

}