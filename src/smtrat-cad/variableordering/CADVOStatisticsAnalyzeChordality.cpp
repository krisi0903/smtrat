
#include <smtrat-common/smtrat-common.h>
#ifdef SMTRAT_DEVOPTION_Statistics
#include "CADVOStatistics.h"
#include <smtrat-cad/CAD.h>
#include <smtrat-analyzer/analyzers/utils.h>
#include <boost/graph/connected_components.hpp>
#include "graph_chordality.h"
#include "util.h"
#include "associated_graph.h"
#include "elimination_tree.h"
#include "graphviz_debug.h"

namespace smtrat::cad::variable_ordering {
    
    void CADVOStatistics::evaluateOrdering(std::vector<Poly> const& polys, std::vector<carl::Variable> const& _ordering) {
        

        struct VariableEdgeProperties {
            bool fillEdge;
        };

        typedef boost::adjacency_list<boost::setS, boost::setS, boost::undirectedS, VariableVertexPropertiesBase, VariableEdgeProperties> ChordalStructure;

        std::map<carl::Variable, Vertex<ChordalStructure>> var_vertex_map;
	    ChordalStructure chordal_structure;

        carl::carlVariables varset;

        for (const Poly& p : polys) {
            // Make sure that we have a vertex for each variable in our graph
            for (auto var : carl::variables(p)) {
                carl::variables(p, varset);
                if (var_vertex_map.find(var) == var_vertex_map.end()) {
                    var_vertex_map[var] = boost::add_vertex({.var = var}, chordal_structure);
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

        std::map<Vertex<ChordalStructure>, int> component_map;
        int connected_components;
        {
            std::map<Vertex<ChordalStructure>, boost::default_color_type> _colormap;
            connected_components = boost::connected_components(chordal_structure, boost::associative_property_map<std::map<Vertex<ChordalStructure>, int>>(component_map), boost::color_map(boost::associative_property_map<std::map<Vertex<ChordalStructure>, boost::default_color_type>>(_colormap)));
        }

        cadVOStatistics._add("connected_components", connected_components);

        if (connected_components != 1) {
            return;
        }

        // most of our algorithms expect std::list for performance reasons
        std::list<Vertex<ChordalStructure>> ordering;
        
        for (carl::Variable v : _ordering) {
            ordering.push_back(var_vertex_map[v]);
        }

        auto fill = elimination_game(chordal_structure, ordering);

        _add("ordering.vertices", boost::num_vertices(chordal_structure));
	    _add("ordering.edges", boost::num_edges(chordal_structure));
	    _add("ordering.filledges", fill.size());

        for (auto [v1, v2] : fill) {
            auto edge = boost::add_edge(v1, v2, {.fillEdge = true}, chordal_structure);
        }


        print_graphviz(chordal_structure);

        EliminationTree<ChordalStructure> t = etree(chordal_structure, ordering);

        print_graphviz_etree(t);

        _add("ordering.etree.height", t[boost::graph_bundle].height);

    }
}
#endif