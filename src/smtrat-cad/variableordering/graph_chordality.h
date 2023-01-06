#pragma once
#include "graph_common.h"
#include "induced_subgraph.h"
#include "CADVOStatistics.h"
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/visitors.hpp>

#include <smtrat-common/smtrat-common.h>
#include <numeric>
#include <algorithm>

namespace smtrat::cad::variable_ordering {
    // A predicate to create a filtered graph that only has vertices
    // with a lower weight than a given number
    // This is elementary to the mcs algorithm
    template <typename Graph>
    struct VertexWeightFilter {
        VertexWeightFilter() {}
        VertexWeightFilter(
            std::map<Vertex<Graph>, unsigned int> * weights, 
            int limit, 
            Vertex<Graph> source, 
            Vertex<Graph> target) : weights(weights), limit(limit), source(source), target(target) {}

        bool operator()(Vertex<Graph> const & v) const {
            return weights->at(v) < limit || v == source || v == target;
        }

        // have to use a pointer here instead of a reference
        // since the filter is required to be default-constructible
        std::map<Vertex<Graph>, unsigned int> * weights;
        int limit;
        Vertex<Graph> source;
        Vertex<Graph> target;
    };


    // Implementation of the "Elimination Game" algorithm
    // This is mostly for sanity-checking our algorithms to ensure that they return a perfect elimination ordering
    template <typename Graph>
    std::list<std::pair<Vertex<Graph>, Vertex<Graph>>> elimination_game(Graph const& g, std::list<Vertex<Graph>> order) {
        typedef boost::adjacency_list<boost::setS, boost::setS, boost::undirectedS, Vertex<Graph>> EliminationGraph;
        std::map<Vertex<Graph>, Vertex<EliminationGraph>> vmap;
        std::list<std::pair<Vertex<Graph>, Vertex<Graph>>> fill;

        EliminationGraph eg;
        // meh
        for (auto [v, v_end] = boost::vertices(g); v != v_end; v++) {
            vmap[*v] = boost::add_vertex(*v, eg);
        }

        for (auto [e, e_end] = boost::edges(g); e != e_end; e++) {
            boost::add_edge(vmap[boost::source(*e, g)], vmap[boost::target(*e, g)], eg);
        }

        for(auto v_ : order) {
            auto v = vmap[v_];
            for (auto [a1, a1_end] = boost::adjacent_vertices(v, eg); a1 != a1_end; a1++) {
                for (auto [a2, a2_end] = boost::adjacent_vertices(v, eg); a2 != a2_end; a2++) {
                    if (*a1 != *a2 && !boost::edge(*a1, *a2, eg).second) {
                        boost::add_edge(*a1, *a2, eg);
                        fill.push_back(std::make_pair(eg[*a1], eg[*a2]));
                    }
                }
            }
            boost::clear_vertex(v, eg);
            boost::remove_vertex(v, eg);
        }
        return fill;
    }



    // Computes the MCS-M Algorithm on the input graph
    // This computes a minimal chordal completion and the according perfect elimination ordering
    // (a minimal elimination ordering for the input graph)
    // Note that the input graph is modified during the procedure to add the fill edges
    // Currently, we pass by-value since we modify the graph
    
    template <typename Graph, typename Compare>
    std::pair<std::list<Vertex<Graph>>,
              std::list<std::pair<Vertex<Graph>, Vertex<Graph>>>> 
    mcs_m(Graph const& chordalStructure, Compare comp) {

        // Copy the graph so we can remove vertices from it
        Graph g = chordalStructure;
        int n = boost::num_vertices(g);


        // Create a vector that maps vertex IDs to vertex descriptors of the original graph
        // This is used to map vertices descriptors of the copy graph to those of the original graph
        // since the vertex descriptors will be different 
        std::vector<Vertex<Graph>> vmap;
        vmap.resize(n);

        for (VertexIterator<Graph> v = vertices(chordalStructure).first; v != vertices(chordalStructure).second; v++) {
            vmap[chordalStructure[*v].id] = *v;
        }
        

        SMTRAT_LOG_TRACE("smtrat.cad.variableordering", "Graph vertex count: " << n) 

        // Each vertex requires a weight for the algorithm
        std::map<Vertex<Graph>, unsigned int> weights;

        // The list containing the vertices in a perfect elimination order
        std::list<Vertex<Graph>> peo;

        // The list of fill edges we have to introduce, given as pairs
        // of vertices
        std::list<std::pair<Vertex<Graph>,Vertex<Graph>>> fill;
        
        for (int i = n; i >= 1; i--) {
            // choose an unnumbered vertex v of maximum weight w(v)
            // we remove numbered vertices (those that have an index in the PEO)
            // from the graph structure
            // thus, just choose a max-weight vertex
            VertexIterator<Graph> start = vertices(g).first;
            VertexIterator<Graph> end = vertices(g).second;

            VertexIterator<Graph> max = std::max_element(vertices(g).first, vertices(g).second, [&](Vertex<Graph> const& v1, Vertex<Graph> const& v2) {
                return weights[v1] < weights[v2] || (weights[v1] == weights[v2] && comp(v2, v1));
            });

            #ifdef SMTRAT_DEVOPTION_Statistics
            cadVOStatistics.recordChoices(std::count_if(vertices(g).first, vertices(g).second, [&](Vertex<Graph> v) {return weights[v] == weights[*max];}));
            #endif

            SMTRAT_LOG_TRACE("smtrat.cad.variableordering", "Max vertex is " << g[*max]);

            /* A set that contains all vertices u
             * that are reachable from max over a path where
             * each intermediate vertex has a lower weight than u
             * (and, by extension, max)
             */
            std::list<Vertex<Graph>> S;
            for(VertexIterator<Graph> u = start; u != end; u++) {
                // undirected graph - max does not have a path to itself
                if (*u == *max) continue;

                SMTRAT_LOG_TRACE("smtrat.cad.variableordering", "check for path from " << g[*max] << "to" << g[*u]);
                
                // we need to find a path from max to u 
                // that is either a direct edge or only has intermediary vertices with weight < w(u)
                // step 1: create a "filtered graph" view that only contains v, u and vertices with lower weight than u
                // We keep all edges that still have a vertex on both ends
                
                VertexWeightFilter<Graph> filter(&weights, weights[*u], *max, *u);

                auto filtered = induce(g, filter);

                std::map<Vertex<Graph>, boost::default_color_type> bfs_colormap;
                boost::associative_property_map<std::map<Vertex<Graph>, boost::default_color_type>> bfs_color_pmap(bfs_colormap);

                // step 2: do a breadth-first search starting from u
                // the reachability_visituor will change the value of hasPath as soon as the vertex u is found
                // starting from vertex v

                // doing a breadth-first search here for every unnumbered vertex does not seem ideal
                // since BFS has complexity O(|V|+|E|), and we have two loops that iterate over some
                // part of the vertices, the best upper bound seems to b O(|V|^2*|E|) instead of the
                // often cited O(|V|*|E|) for MCS-M
                boost::breadth_first_search(filtered, *max, boost::color_map(bfs_color_pmap));

                if (bfs_colormap[*u] == boost::default_color_type::black_color) {
                    S.push_back(*u);
                }
            }

            for(auto u : S) {
                weights[u]++;
                // return value of edge is a pair - first is always an edge descriptor, second indicates whether the edge is actually there
                if (!(boost::edge(*max, u, g).second)) {
                    fill.push_back(std::make_pair(vmap[g[u].id], vmap[g[*max].id]));
                }
            }

            peo.push_front(vmap[g[*max].id]);
            boost::clear_vertex(*max, g);
            boost::remove_vertex(*max, g);
        }


        return std::make_pair(peo, fill);
    }

    
    template <typename Graph>
    std::pair<std::list<Vertex<Graph>>,
              std::list<std::pair<Vertex<Graph>, Vertex<Graph>>>> 
    mcs_m(Graph const& chordalStructure) {
        return mcs_m(chordalStructure, std::less<Vertex<Graph>>{});
    }

    template <typename Graph, typename Compare>
    std::list<Vertex<Graph>> mcs(Graph const& g, Compare comp) {
        std::map<Vertex<Graph>, int> w;

        std::list<Vertex<Graph>> res;
        std::set<Vertex<Graph>> unnumbered;

        for(auto [v, v_end] = boost::vertices(g); v != v_end; v++) {
            w[*v] = 0;
            unnumbered.insert(*v);
        }

        for (uint8_t i = boost::num_vertices(g); i > 0; i--) {
            Vertex<Graph> v = *std::max_element(unnumbered.begin(), unnumbered.end(), [&](auto _v1, auto _v2) { 
                return w[_v1] < w[_v2] || w[_v1] == w[_v2] && !comp(_v1, _v2);
            });
            for (auto [u, u_end] = boost::adjacent_vertices(v, g); u != u_end; u++) {
                if (unnumbered.count(*u) > 0) w[*u]++;
            }
            res.push_front(v);
            unnumbered.erase(v);
        }
        return res;
    }

    template <typename Graph>
    std::list<Vertex<Graph>> mcs(Graph const& g) {
        return mcs(g, std::less<Vertex<Graph>>{});
    }
}