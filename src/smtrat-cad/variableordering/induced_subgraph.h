#pragma once
#include "graph_common.h"
#include <boost/graph/filtered_graph.hpp>

namespace smtrat::cad::variable_ordering {

    template <typename Graph, typename VertexPredicate>
    class InducedSubgraphFilter {
        VertexPredicate const* pred;
        Graph const* g;
       
        public:
        InducedSubgraphFilter() {}
        InducedSubgraphFilter(Graph const* g, VertexPredicate const* pred) : g(g), pred(pred) {}

        bool operator()(Edge<Graph> const& e) const {
            return (*pred)(boost::source(e, *g)) && (*pred)(boost::target(e, *g));
        }

        bool operator()(Vertex<Graph> const& g) const
         {
            return (*pred)(g);
        }


    };

    // The induced subgraph of a vertex set on another graph, implemented using the boost::filtered_graph
    template <typename Graph, typename VertexPredicate>
    using InducedSubgraph = typename boost::filtered_graph<Graph, InducedSubgraphFilter<Graph, VertexPredicate>, InducedSubgraphFilter<Graph, VertexPredicate>>;

    template <typename Graph, typename VertexPredicate>
    InducedSubgraph<Graph, VertexPredicate> induce(Graph const& g, VertexPredicate const& v) {
        InducedSubgraphFilter<Graph, VertexPredicate> f(&g, &v);
        return InducedSubgraph<Graph, VertexPredicate>(g, f, f);
    }


}