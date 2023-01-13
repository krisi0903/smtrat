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
}