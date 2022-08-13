#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/visitors.hpp>

#include <numeric>
#include <algorithm>

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


    // Auxiliary filter for filtering a graph for edges
    // keeps only edges where both vertices are kept by VertexFilterPredicate
    template <typename VertexFilterPredicate, typename Graph>
    class keep_unfiltered {
        VertexFilterPredicate* pred;
        Graph* g;
        public:
        keep_unfiltered() {}
        keep_unfiltered(VertexFilterPredicate* pred, Graph* g) : pred(pred), g(g){}

        bool operator()(Edge<Graph> const & e) const{
            return (*pred)(boost::source(e, *g)) && (*pred)(boost::target(e, *g));
        }

    };

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

    // Type of a filtered graph filtered by vertex weights
    template <typename Graph>
    using MCSFilteredGraph = typename boost::filtered_graph<Graph, keep_unfiltered<VertexWeightFilter<Graph>, Graph>, VertexWeightFilter<Graph>>;
    
    // Computes the MCS-M Algorithm on the input graph
    // This computes a minimal chordal completion and the according perfect elimination ordering
    // (a minimal elimination ordering for the input graph)
    // Note that the input graph is modified during the procedure to add the fill edges
    // Currently, we pass by-value since we modify the graph
    
    template <typename Graph>
    std::pair<std::list<Vertex<Graph>>,
              std::list<std::pair<Vertex<Graph>, Vertex<Graph>>>> 
    mcs_m(Graph const& chordalStructure) {

        // Copy the graph so we can remove vertices from it
        Graph g = chordalStructure;


        std::map<Vertex<Graph>, Vertex<Graph>> vertex_backmap;

        for(VertexIterator<Graph> copy_vertex = vertices(g).first; copy_vertex != vertices(g).second; copy_vertex++) {
            for (VertexIterator<Graph> orig_vertex = vertices(chordalStructure).first; orig_vertex != vertices(chordalStructure).second; orig_vertex++) {
                if (chordalStructure[*orig_vertex].name  == g[*copy_vertex].name) {
                    vertex_backmap[*copy_vertex] = *orig_vertex;
                    break;
                }
            }
        }
        int n = boost::num_vertices(g);

        std::cout << n << std::endl;

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

            VertexIterator<Graph> max = start;
            for(auto iter = start; iter != end; iter++) {
                if (weights[*iter] > weights[*max]) {
                    max = iter;
                }
            }
            
            std::cout << "Max vertex is " << g[*max].name;

            /* A set that contains all vertices u
             * that are reachable from max over a path where
             * each intermediate vertex has a lower weight than u
             * (and, by extension, max)
             */
            std::list<Vertex<Graph>> S;
            for(VertexIterator<Graph> u = start; u != end; u++) {
                std::cout << "check path to " << g[*u].name;
                // undirected graph - max does not have a path to itself
                if (*u == *max) continue;
                
                // we need to find a path from max to u 
                // that is either a direct edge or only has intermediary vertices with weight < w(u)
                // step 1: create a "filtered graph" view that only contains v, u and vertices with lower weight than u
                // We keep all edges that still have a vertex on both ends

                VertexWeightFilter<Graph> filter(&weights, weights[*u], *max, *u);
                keep_unfiltered<VertexWeightFilter<Graph>, Graph> edgefilter(&filter, &g);

                MCSFilteredGraph<Graph> filtered(g, edgefilter, filter);

                std::map<Vertex<Graph>, boost::default_color_type> bfs_colormap;
                boost::associative_property_map<std::map<Vertex<Graph>, boost::default_color_type>> bfs_color_pmap(bfs_colormap);

                // step 2: do a breadth-first search starting from u
                // the reachability_visituor will change the value of hasPath as soon as the vertex u is found
                // starting from vertex v
                boost::breadth_first_search(filtered, *max, boost::color_map(bfs_color_pmap));

                if (bfs_colormap[*u] == boost::default_color_type::black_color) {
                    S.push_back(*u);
                }
            }

            std::cout << "Have vertices in S: ";
            for(auto u : S) {
                std::cout << g[u].name;
                weights[u]++;
                // return value of edge is a pair - first is always an edge descriptor, second indicates whether the edge is actually there
                if (!(boost::edge(*max, u, g).second)) {
                    std::cout << "adding fill edge";
                    fill.push_back(std::make_pair(vertex_backmap[u], vertex_backmap[*max]));
                }
            }

            std::cout << std::endl;

            peo.push_front(vertex_backmap[*max]);
            boost::clear_vertex(*max, g);
            boost::remove_vertex(*max, g);
        }


        return std::make_pair(peo, fill);
    }
}