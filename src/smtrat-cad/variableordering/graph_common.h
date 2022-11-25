#pragma once
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/visitors.hpp>


namespace smtrat::cad::variable_ordering {

    /* We introduce some type aliases for commonly used graph elements
     * since the boost types just get too long for comfortable use */
    template <typename Graph>
    using Edge = typename boost::graph_traits<Graph>::edge_descriptor;

    template <typename Graph>
    using Vertex = typename boost::graph_traits<Graph>::vertex_descriptor;

    template <typename Graph>
    using EdgeIterator = typename boost::graph_traits<Graph>::edge_iterator;
    
    template <typename Graph>
    using VertexIterator = typename boost::graph_traits<Graph>::vertex_iterator;    
}