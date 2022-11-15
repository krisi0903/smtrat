#include <algorithm>
#include <numeric>
#include <iostream>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/graph/graphviz.hpp>
#include <filesystem>
#include "graph_chordality.h"
#include "chordal_vargraph_elimination_ordering.h"
#include <smtrat-cad/CADVOStatistics.h>


namespace smtrat::cad::variable_ordering {

    // Properties that are saved for each vertex in the graph
    struct VariableVertexProperties {
        unsigned int id;
        carl::Variable var;
    };

    struct VariableEdgeProperties {
        Poly const* poly;
        bool fillEdge;
    };

    std::ostream& operator<<(std::ostream& os, VariableVertexProperties const& props) {
        os << props.var;
        return os;
    }


    // a little hack because I do not know where to put this setting
    // enables writing the graph with colored fill-in edges to a file
    constexpr bool debugChordalVargraph = true;


    /*!
     * A small utility function to print out the "chordal structure of the polynomial set" in a .doft
     * file for visualization and better debugging
     * a .dot file is automatically created in the current working directory
     */
    template <typename Graph>
    void print_graphviz(Graph const& g) {
        boost::posix_time::ptime current_time(boost::posix_time::second_clock::local_time());
        std::string current_time_s = boost::posix_time::to_iso_string(current_time);
        std::string filename = "variable-graph-" + current_time_s + ".dot";

        // If just the time does not work (when we are drawing multiple graphs in the same second)
        // we add an index and increment as long as the filename still exists
        if (std::filesystem::exists(filename)) {
            int i = 0;
            do {
                filename = "variable-graph-" + current_time_s + "-" + std::to_string(++i) + ".dot";
            } while(std::filesystem::exists(filename));
        }
        std::ofstream filestream(filename);
        boost::write_graphviz(filestream, 
        g, 
        // Node property writer: default label_writer, which writes the name as the label property
        [&](std::ostream& out, typename boost::graph_traits<Graph>::vertex_descriptor v) {
            out << "[label=\"" << g[v].var << "\"]";
        },

        // Edge property writer: Color the edge red if it is a fill-edge
        [&](std::ostream& out, typename boost::graph_traits<Graph>::edge_descriptor e) {
            if(g[e].fillEdge) {
                out << "[color=red]";
            }
        },

        // Graph property writer: defines some useful properties for the whole graph
        [](std::ostream& out) {
            // out << "graph [bgcolor=lightgrey]" << std::endl;
            out << "node [shape=plaintext]" << std::endl;
            // out << "edge [style=dashed]" << std::endl;
        },
        boost::get(&VariableVertexProperties::id, g));
        filestream.close();
    } 

    std::vector<carl::Variable> chordal_vargraph_elimination_ordering(const std::vector<Poly>& polys) {
        SMTRAT_STATISTICS_INIT(CADVOStatistics, statistics, "CADVOStatistics");
        SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Building order based on " << polys);

        typedef boost::adjacency_list<boost::setS, boost::setS, boost::undirectedS, VariableVertexProperties, VariableEdgeProperties> ChordalStructure;

        std::map<carl::Variable, Vertex<ChordalStructure>> var_vertex_map;
        ChordalStructure chordal_structure;


        // Counter for the ID number we assign to vertices
        // Our implementation of the MCS algorithm requires each vertex to have a unique index
        // in the range [0; |V|)
        unsigned int id = 0;

        carl::carlVariables varset;

        for (const Poly& p : polys) {
            // Make sure that we have a vertex for each variable in our graph
            for (auto var : carl::variables(p)) {
                carl::variables(p, varset);
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
        SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Have " << num_vertices(chordal_structure) << " vertices with " << num_edges(chordal_structure) << " edges. " << fill.size() << " fill-in edges to make graph chordal with given PEO, a percentage of " <<  100 * (fill.size() / (double) num_edges(chordal_structure) + fill.size()) << "%'.");
        
        statistics._add("ordering.edges", num_edges(chordal_structure));
        statistics._add("ordering.filledges", fill.size());
        if constexpr (debugChordalVargraph) {
            // Add the fill edges to the graph structure so that we can pass it to the drawing function
            for (auto const& pair : fill) {
                add_edge(pair.first, pair.second, {.poly = nullptr, .fillEdge = true}, chordal_structure);
            }

            print_graphviz<ChordalStructure>(chordal_structure);
        }



        std::vector<carl::Variable> res;
        std::vector<carl::Variable> var_vector(std::move(varset.as_vector()));

        for (auto v : peo) {
            res.push_back(chordal_structure[v].var);
        }

        SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Sorted: " << res);
        // Check that the new variable ordering is indeed a permutation of the polynomial set
        // determined before re-ordering
        // if not, this will impact the correctness of our result, so we abort
        SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Originale: " << var_vector);
        assert(std::is_permutation(res.begin(), res.end(), var_vector.begin(), var_vector.end()));
        return res;
    }
}