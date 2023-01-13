#pragma once
#include "graph_common.h"
#include "induced_subgraph.h"
#include "CADVOStatistics.h"
#include "util.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include "boost/date_time/posix_time/posix_time.hpp"


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
    using EliminationTree = boost::adjacency_list<boost::setS, boost::setS, boost::bidirectionalS, ETreeVertexProperties<Graph>, boost::no_property, ETreeProperties<Graph>>;

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

    template <typename Graph>
    EliminationTree<Graph> etree(Graph const& g, std::list<Vertex<Graph>> const& peo) {
        EliminationTree<Graph> t;
        // add all vertices from the original graph into the tree, and store a mapping
        // from input graph vertices to tree vertices
        std::map<Vertex<Graph>, Vertex<EliminationTree<Graph>>> gtmap;
        
        for(auto [v, v_end] = boost::vertices(g); v != v_end; v++) {
            gtmap[*v] = boost::add_vertex({.v = *v}, t);
        }

        vec_order_comp comp(peo);

        for(auto [v, v_end] = boost::vertices(g); v != v_end; v++) {
            if (*v != peo.back()) {
                
                Vertex<Graph> parent = nullptr;
                for(auto [a, a_end] = boost::adjacent_vertices(*v, g); a != a_end; a++) {
                    if (comp(*v, *a) && (parent == nullptr || comp(*a, parent))) parent = *a;
                }
                if (parent != nullptr) {
                    boost::add_edge(gtmap[*v], gtmap[parent], t);
                }
                
            }
        }

        std::set<Vertex<EliminationTree<Graph>>> curr;
        std::set<Vertex<EliminationTree<Graph>>> next;

        for (auto [v, v_end] = boost::vertices(t); v != v_end; v++) {
            if (!boost::in_degree(*v, t)) {
                curr.insert(*v);
            }
        }



        uint height = 0;

        while(curr.size() > 0) {
            height++;
            for (auto v : curr) {
                t[v].class_label = height;
                auto [p, p_end] = boost::adjacent_vertices(v, t);
                if (p != p_end) next.insert(*p);
            }
            curr = next;
            next.clear();
        }

        t[boost::graph_bundle].height = height;
        t[boost::graph_bundle].base = const_cast<Graph*>(&g);

        return t;
    }

    /**
     * @brief Generate the elimination tree of minimal height, given a chordal graph as input
     * This uses the algorithm by Jess andd Kees (1982), DOI: 10.1109/TC.1982.1675979
     * 
     * @tparam Graph Type of the input graph.
     * @param g0 The input graph, must be chordal
     * @return EliminationTree<Graph> An elimination tree of the input graph.
     */
    template <typename Graph, typename Compare>
    std::pair<EliminationTree<Graph>, std::list<Vertex<Graph>>> min_height_etree(Graph const& g0, Compare comp) {

        long double dor = std::numeric_limits<long double>::quiet_NaN();
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
        std::list <Vertex<Graph>> peo;
        

        // Every iteration of this outer loop basically corresponds to one parallel computation
        // step of the elimination game: In each iteration, we simulate an arbitrary number of "processes"
        // to find a set of nodes that we could remove simultaneously without data access conflicts.
        while(V.size() > 0) {
            // U: The set of "removal candidates"
            // U := vertices(g)
            std::set <Vertex<Graph>> U = V;

            while(U.size() > 0) {
                
                // The loop below is not part of the original algorithm, but we decide
                // to throw out all vertices with non-zero deficiency first, and then
                // select one vertex (which has a guarenteed zero deficiency) to eliminate
                // Since the vertices with nonzero deficiencly just get removed,
                // their removal order does not have any impact on the algorithm (and with that, the final peo)
                // and hence, we do not want to count these to our 'degrees of freedom' metric
                
                { 
                    std::list<Vertex<Graph>> to_remove;
                    for (Vertex<Graph> v : U) {
                        if (!deficiency_zero(v, g)) {
                            to_remove.push_back(v);
                        }
                    }

                    // we cannot remove in the upper loop, doing so would invalidate our iterators :/
                    for (Vertex<Graph> v : to_remove) {
                        U.erase(v);
                    }
                }
                
                

                if (U.size() == 0) break;

                if (std::isnan(dor)) {dor = 1;}

                dor *= U.size();
                dor /= V.size();

                Vertex<Graph> v = *std::min_element(U.begin(), U.end(), comp);
                U.erase(v);

                // If v has deficiency zero (i.e. the adjacent vertices form a clique)
                // we can eliminate it without introducing fill.
                // Also remove the adjacent vertices from the candidate set U, since they cannot be eliminated
                // in parallel with v
        
                // because of the loop above, we know that deficiency is zero

                labels[v] = i;
                
                for (auto [adj, adj_end] = adjacent_vertices(v,g); adj != adj_end; adj++) {
                    U.erase(*adj);
                }
                V.erase(v);
                peo.push_back(v);

                
            }
            i++;
        }

        cadVOStatistics._add("choices_etree", dor);

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
        return std::make_pair(t, peo);
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