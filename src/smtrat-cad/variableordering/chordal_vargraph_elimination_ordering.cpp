#include <algorithm>
#include <numeric>
#include "graph_chordality.h"
#include "chordal_vargraph_elimination_ordering.h"


namespace smtrat::cad::variable_ordering {

    
    std::vector<carl::Variable> triangular_ordering(const std::vector<Poly>& polys) {
	SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Building order based on " << polys);

    // Properties that are saved for each vertex in the graph
    struct VariableVertexProperties {
        carl::Variable var;
    }

    typedef boost::adjacency_list<boost::setS, boost::setS, boost::undirectedS, VariableVertexProperties> ChordalStructure;

    std::map<carl::Variable, Vertex<ChordalStructure>> var_vertex_map;
    ChordalStructure chordal_structure;



	for (const Poly& p : polys) {
        // Make sure that we have a vertex for each variable in our graph
        for (auto var : carl::variables(p)) {
            if (var_vertex_map.find(var) == std::end()) {
                v = var_vertex_map[var] = boost::add_vertex({.var = var}, chordalStructure);
            }
        }

        // for each pair of variables in the polynomial, we add an edge to our graph
        for (auto var1 : carl::variables(p)) {
            for (auto var2 : carl::variables(p)) {
                if (var1 != var2) {
                    boost::add_edge(var_vertex_map[var1], var_vertex_map[var2]);
                }
            }
        }
	}

    auto [peo, fill] = mcs_m(chordalStructure);


	std::vector<carl::Variable> res;

    for (auto v : peo) {
        res.push_back(chordalStructure[v].var);
    }
	SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Sorted: " << res);
	return res;
}
}