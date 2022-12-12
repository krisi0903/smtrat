#pragma once
#include <vector>
#include <smtrat-common/smtrat-common.h>

namespace smtrat::cad::variable_ordering {
    // A variable ordering is always described by a function that takes the vector of polynomials
    // and returns a vector of variables in the desired order

    typedef std::vector<carl::Variable> (*VariableOrdering)(const std::vector<Poly>& polys);
    
    template<typename T>
    class VariableMap {
    private:
        std::vector<T> mData;
    public:
        T operator[](carl::Variable v) const {
            assert(v != carl::Variable::NO_VARIABLE);
            if (mData.size() <= v.id() - 1) return 0;
            return mData[v.id() - 1];
        }
        T& operator[](carl::Variable v) {
            assert(v != carl::Variable::NO_VARIABLE);
            if (mData.size() <= v.id() - 1) mData.resize(v.id());
            return mData[v.id() - 1];
        }
    };

    /**
     * @brief A total ordering on variables that is stable for a given input file.
     * This shall be used in all variable orderings as a tie-breaker whenever an ordering
     * relation is not total to avoid the influence of undefined state (e.g. the order of polynomials
     * in an iterator over the polynomial set)
     * 
     * @param v1 The first variable
     * @param v2 The second variable
     * @return true 
     * @return false 
     */
    inline bool stable_var_comp(carl::Variable v1, carl::Variable v2) {
        return v1.name() < v2.name();
    }
}