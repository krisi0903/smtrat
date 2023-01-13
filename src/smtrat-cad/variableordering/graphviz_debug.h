#include <boost/date_time/local_time/local_time.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <filesystem>
#include <iostream>
#include "elimination_tree.h"


namespace smtrat::cad::variable_ordering {

/*!
 * A small utility function to print out the "chordal structure of the polynomial set" in a .dot
 * file for visualization and better debugging
 * a .dot file is automatically created in the current working directory
 */
template<typename Graph>
std::string print_graphviz(Graph const& g) {

	std::filesystem::path basedir("/tmp/smtrat-debug-graphs");
	boost::posix_time::ptime current_time(boost::posix_time::second_clock::local_time());
	std::string current_time_s = boost::posix_time::to_iso_string(current_time);
	std::filesystem::path filename = basedir / ("variable-graph-" + current_time_s + ".dot");

    std::map<Vertex<Graph>, int> idmap;
    int id = 0;

    for (auto [v, v_end] = boost::vertices(g); v != v_end; v++) {
        idmap[*v] = id++;
    }


    boost::associative_property_map _idmap(idmap);
	// If just the time does not work (when we are drawing multiple graphs in the same second)
	// we add an index and increment as long as the filename still exists
	if (std::filesystem::exists(filename)) {
		int i = 0;
		do {
			filename = basedir / ("variable-graph-" + current_time_s + "-" + std::to_string(++i) + ".dot");
		} while (std::filesystem::exists(filename));
	}
	std::ofstream filestream(filename);
	boost::write_graphviz(
		filestream,
		g,
		// Node property writer: default label_writer, which writes the name as the label property
		[&](std::ostream& out, typename boost::graph_traits<Graph>::vertex_descriptor v) {
			out << "[label=\"" << g[v].var << "\"]";
		},

		// Edge property writer: Color the edge red if it is a fill-edge
		[&](std::ostream& out, typename boost::graph_traits<Graph>::edge_descriptor e) {
			if (g[e].fillEdge) {
				out << "[color=red]";
			}
		},

		// Graph property writer: defines some useful properties for the whole graph
		[](std::ostream& out) {
			// out << "graph [bgcolor=lightgrey]" << std::endl;
			out << "node [shape=plaintext]" << std::endl;
			// out << "edge [style=dashed]" << std::endl;
		},
		_idmap);
	filestream.close();
	return filename;
}


/*!
     * A small utility function to print out the "chordal structure of the polynomial set" in a .doft
     * file for visualization and better debugging
     * a .dot file is automatically created in the current working directory
     */
    template <typename Graph>
    std::string print_graphviz_etree(EliminationTree<Graph> const& g) {
        std::filesystem::path basedir("/tmp/smtrat-debug-graphs");
        std::filesystem::create_directories(basedir);
        boost::posix_time::ptime current_time(boost::posix_time::second_clock::local_time());
        std::string current_time_s = boost::posix_time::to_iso_string(current_time);
        std::filesystem::path filename = basedir / ("variable-graph-etree-" + current_time_s + ".dot");
        
        // If just the time does not work (when we are drawing multiple graphs in the same second)
        // we add an index and increment as long as the filename still exists
        if (std::filesystem::exists(filename)) {
            int i = 0;
            do {
                filename = basedir / ("variable-graph-etree" + current_time_s + "-" + std::to_string(++i) + ".dot");
            } while(std::filesystem::exists(filename));
        }
        std::ofstream filestream(filename);

        filestream << "digraph G {\n";

        std::vector<Vertex<EliminationTree<Graph>>> vertices;
        vertices.reserve(num_vertices(g));
        for (auto [v, v_end] = boost::vertices(g); v != v_end; v++) {
            vertices.push_back(*v);
        }

        auto comp = [&](Vertex<EliminationTree<Graph>> v1, Vertex<EliminationTree<Graph>> v2) -> bool {
            return g[v1].class_label < g[v2].class_label;
        };
        std::sort(vertices.begin(), vertices.end(), comp);

        auto range_begin = vertices.begin();
        auto range_end = vertices.begin();

        while(range_begin != vertices.end()) {
            for(; range_end != vertices.end() && g[*range_end].class_label == g[*range_begin].class_label; range_end++);
            int level = g[*range_begin].class_label;
            filestream << "\tsubgraph level_" << level << " {\n\t\trank=same\n";
            for (auto curr = range_begin; curr != range_end; curr++) {
                // 
                filestream << "\t\t" << (*g[boost::graph_bundle].base)[g[*curr].v] << ";\n";
            }
            filestream << "\t}\n\n";
            range_begin = range_end;
        } 

        for(auto [v, v_end] = boost::vertices(g); v != v_end; v++) {
            auto [p, p_end] = adjacent_vertices(*v,g);
            if (p != p_end) {
                filestream << "\t" << (*g[boost::graph_bundle].base)[g[*v].v] << " -> " << (*g[boost::graph_bundle].base)[g[*p].v] << ";\n";
            }

        }
        filestream << "}\n";
        filestream.close();
        return filename;
    } 
}