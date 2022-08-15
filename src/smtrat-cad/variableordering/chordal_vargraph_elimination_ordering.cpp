#include <algorithm>
#include <numeric>
#include <iostream>
#include "graph_chordality.h"
#include "chordal_vargraph_elimination_ordering.h"


namespace smtrat::cad::variable_ordering {

    // Properties that are saved for each vertex in the graph
    struct VariableVertexProperties {
        unsigned int id;
        carl::Variable var;
    };

    std::ostream& operator<<(std::ostream& os, VariableVertexProperties const& props) {
        os << props.var;
        return os;
    }

    std::vector<carl::Variable> chordal_vargraph_elimination_ordering(const std::vector<Poly>& polys) {
        SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Building order based on " << polys);

        typedef boost::adjacency_list<boost::setS, boost::setS, boost::undirectedS, VariableVertexProperties> ChordalStructure;

        std::map<carl::Variable, Vertex<ChordalStructure>> var_vertex_map;
        ChordalStructure chordal_structure;


        // Counter for the ID number we assign to vertices
        // Our implementation of the MCS algorithm requires each vertex to have a unique index
        // in the range [0; |V|)
        unsigned int id = 0;

        for (const Poly& p : polys) {
            // Make sure that we have a vertex for each variable in our graph
            for (auto var : carl::variables(p)) {
                if (var_vertex_map.find(var) == var_vertex_map.end()) {
                    var_vertex_map[var] = boost::add_vertex({ .id=id++, .var = var}, chordal_structure);
                }
            }

            // for each pair of variables in the polynomial, we add an edge to our graph
            for (auto var1 : carl::variables(p)) {
                for (auto var2 : carl::variables(p)) {
                    if (var1 != var2) {
                        boost::add_edge(var_vertex_map[var1], var_vertex_map[var2], chordal_structure);
                    }
                }
            }
        }

        auto [peo, fill] = mcs_m(chordal_structure);


        std::vector<carl::Variable> res;

        for (auto v : peo) {
            res.push_back(chordal_structure[v].var);
        }
        SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Sorted: " << res);
        return res;
    }
}