#pragma once
#include <smtrat-common/smtrat-common.h>

namespace smtrat::cad::variable_ordering {
    struct VariableVertexPropertiesBase {
        carl::Variable var;
    };

    std::ostream& operator<<(std::ostream& os, VariableVertexPropertiesBase prop) {
        return os << prop.var;
    }
}