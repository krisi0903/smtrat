#pragma once
#include "graph_common.h"
#include "induced_subgraph.h"

namespace smtrat::cad::variable_ordering {
    template <typename Graph>
    struct ETreeVertexProperties {
        Vertex<Graph> v;
        int class_label;
    };

    template <typename Graph>
    struct ETreeProperties {
        Graph* base;
        unsigned int height;
    };

    template <typename Graph>
    using EliminationTree = boost::adjacency_list<boost::setS, boost::setS, boost::directedS, ETreeVertexProperties<Graph>, boost::no_property, ETreeProperties<Graph>>;


    /*!
     * A small utility function to print out the "chordal structure of the polynomial set" in a .doft
     * file for visualization and better debugging
     * a .dot file is automatically created in the current working directory
     */
    template <typename Graph>
    void print_graphviz_etree(EliminationTree<Graph> const& g) {
        boost::posix_time::ptime current_time(boost::posix_time::second_clock::local_time());
        std::string current_time_s = boost::posix_time::to_iso_string(current_time);
        std::string filename = "variable-graph-etree-" + current_time_s + ".dot";
        
        // If just the time does not work (when we are drawing multiple graphs in the same second)
        // we add an index and increment as long as the filename still exists
        if (std::filesystem::exists(filename)) {
            int i = 0;
            do {
                filename = "variable-graph-etree" + current_time_s + "-" + std::to_string(++i) + ".dot";
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
    } 


    template<typename Graph>
    bool deficiency_zero(Vertex<Graph> v, Graph const& g) {
        auto [adj_begin, adj_end] = adjacent_vertices(v, g);

        for (auto v1 = adj_begin; v1 != adj_end; v1++) {
            for (auto v2 = adj_begin; v2 != adj_end; v2++) {
                if (*v1 != *v2 && !boost::edge(*v1, *v2, g).second) return false;
            }
        }
        return true;
        
    }


    /**
     * @brief Generate the elimination tree of minimal height, given a chordal graph as input
     * This uses the algorithm by Jess andd Kees (1982), DOI: 10.1109/TC.1982.1675979
     * 
     * @tparam Graph Type of the input graph.
     * @param g0 The input graph, must be chordal
     * @return EliminationTree<Graph> An elimination tree of the input graph.
     */
    template <typename Graph>
    EliminationTree<Graph> min_height_etree(Graph const& g0) {
        unsigned int i = 1;
        // we want to be able to remove vertices successively without modifying our input.
        // If we copied the graph, the VertexDescriptors in the new graph would be different
        // which is undesirable for our algorithm. Therefore, we define an induced subgraph
        // on the set V, which is really just a filtered view on our original graph
        std::set <Vertex<Graph>> V;
        for (auto [v, v_end] = vertices(g0); v != v_end; v++) V.insert(*v);

        auto filter = [&](Vertex<Graph> const& _v) {return V.count(_v) > 0;};
        auto g = induce(g0, filter);

        std::map <Vertex<Graph>, int> labels;
        

        // Every iteration of this outer loop basically corresponds to one parallel computation
        // step of the elimination game: In each iteration, we simulate an arbitrary number of "processes"
        // to find a set of nodes that we could remove simultaneously without data access conflicts.
        while(V.size() > 0) {
            // U: The set of "removal candidates"
            // U := vertices(g)
            std::set <Vertex<Graph>> U = V;

            while(U.size() > 0) {
                Vertex<Graph> v = *U.begin();
                U.erase(v);

                // If v has deficiency zero (i.e. the adjacent vertices form a clique)
                // we can eliminate it without introducing fill.
                // Also remove the adjacent vertices from the candidate set U, since they cannot be eliminated
                // in parallel with v
                if (deficiency_zero(v, g)) {
                    labels[v] = i;
                    
                    for (auto [adj, adj_end] = adjacent_vertices(v,g); adj != adj_end; adj++) {
                        U.erase(*adj);
                    }
                    V.erase(v);
                }
                
            }
            i++;
        }

        // Now, construct the elimination tree, which we are just going to model as a bgl directed graph
        // every ETree vertex holds two properties:
        // - the corresponding vertex descriptor in the original graph
        // - The level, (also called "class label" in literatur) assigned above
        // This way, we can easily construct elimination orders later
        EliminationTree<Graph> t({.base = const_cast<Graph*>(&g0), .height = i - 1});


        // add all vertices from the original graph into the tree, and store a mapping
        // from input graph vertices to tree vertices
        std::map<Vertex<Graph>, Vertex<EliminationTree<Graph>>> gtmap;
        
        for(auto [v, v_end] = boost::vertices(g0); v != v_end; v++) {
            gtmap[*v] = boost::add_vertex({.v = *v, .class_label = labels[*v]}, t);
        }

        for(auto [v, v_end] = vertices(g0); v != v_end; v++) {
            std::set<int> l;

            for (auto [w, w_end] = adjacent_vertices(*v, g0); w != w_end; w++) {
                if (labels[*w] > labels[*v]) {
                    l.insert(labels[*w]);
                }
            }


            // this is a bit ugly, but the root node obviously has no parent, so we have to consider
            // this case.
            auto end = adjacent_vertices(*v, g0).second;
            auto min = end;
            for (auto [w, w_end] = adjacent_vertices(*v, g0); w != w_end; w++) {
                if (l.count(labels[*w]) == 1 && (min == end || labels[*w] <  labels[*min])) {
                    min = w;
                }
            }
            if (min != end) {
                add_edge(gtmap[*v], gtmap[*min], t);
            }
            
        }
        return t;
    }

    template <typename Graph>
    std::list<Vertex<Graph>> peo_from_etree(EliminationTree<Graph> const& t) {
        // since the vertices on any level can always be eliminated
        // in parallel without introducing fill, any elimination order
        // that eliminates the tree layers in order is perfect, regardless
        // of the order within a level
        // However, depending on the size of the levels, this leaves some additional
        // optimization potential, since we could now apply a different ordering heuristic
        // within the levels themselves
        std::vector<Vertex<EliminationTree<Graph>>> nodes;
        nodes.reserve(num_vertices(t));

        for(auto [v, v_end] = vertices(t); v != v_end; v++) {
            nodes.push_back(*v);
        }

        auto comp = [&](Vertex<EliminationTree<Graph>> v1, Vertex<EliminationTree<Graph>> v2) {
            return t[v1].class_label < t[v2].class_label;
        };

        std::sort(nodes.begin(), nodes.end(), comp);

        std::list<Vertex<Graph>> res;

        for (auto node : nodes) {
            res.push_back(t[node].v);
        }
        return res;

    }
}