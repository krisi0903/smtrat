#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/time_clock.hpp>
#include <filesystem>
#include <vector>
#include <iostream>
#include <chrono>
#include <smtrat-cad/variableordering/graph_chordality.h>


struct VariableVertexProperties {
    std::string name;
    unsigned int id;
};

struct PolynomialEdgeProperties {
    bool fillEdge;
};


typedef boost::adjacency_list<boost::setS, boost::setS, boost::undirectedS, VariableVertexProperties, PolynomialEdgeProperties> Graph;
    

std::ostream& operator<<(std::ostream& os, VariableVertexProperties& props) {
    os << props.name;
    return os;
}



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
    boost::make_label_writer(boost::get(&VariableVertexProperties::name, g)),

    // Edge property writer: Color the edge red if it is a fill-edge
    [&](std::ostream& out, boost::graph_traits<Graph>::edge_descriptor e) {
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

int main(void) {
    //std::map<boost::graph_traits<Graph>::vertex_descriptor, std::string> varmap;

    std::map<std::string, boost::graph_traits<Graph>::vertex_descriptor> vertex_map;
    
    Graph g;
    vertex_map["y1"] = boost::add_vertex({.name = "y1", .id=0}, g);
    vertex_map["y2"] = boost::add_vertex({.name = "y2", .id=1}, g);
    vertex_map["y3"] = boost::add_vertex({.name = "y3", .id=2}, g);
    vertex_map["y4"] = boost::add_vertex({.name = "y4", .id=3}, g);
    vertex_map["y5"] = boost::add_vertex({.name = "y5", .id=4}, g);

    /*
    boost::add_edge(vertex_map["y1"], vertex_map["y2"], g);
    boost::add_edge(vertex_map["y1"], vertex_map["y3"], g);
    boost::add_edge(vertex_map["y1"], vertex_map["y4"], g);
    boost::add_edge(vertex_map["y2"], vertex_map["y3"], g);
    boost::add_edge(vertex_map["y3"], vertex_map["y4"], g);
    boost::add_edge(vertex_map["y4"], vertex_map["y2"], g);
    boost::add_edge(vertex_map["y5"], vertex_map["y2"], g);
    boost::add_edge(vertex_map["y5"], vertex_map["y3"], g);
    */
    #define edge(u,v) boost::add_edge(vertex_map[#u], vertex_map[#v], g);
    edge(y1, y2);
    edge(y2, y3);
    edge(y3, y4);
    edge(y4, y5);
    edge(y5, y1);
    #undef edge
    for (auto v = boost::vertices(g).first; v != boost::vertices(g).second; v++) {
        std::cout << *v << std::endl;
        std::cout << g[*v].name << std::endl;
    }

    auto [peo, fill] = smtrat::cad::variable_ordering::mcs_m(g);

    std::cout << "PEO: ";

    for (auto const& item : peo) {
        std::cout << g[item].name << ", ";
    }

    std::cout << std::endl << "Fill Edges:";

    for (auto const& item : fill) {
        std::cout << "(" << g[item.first].name << ", " << g[item.second].name << "), ";
    }

    for (auto const& vpair : fill) {
        boost::add_edge(vpair.first, vpair.second, {.fillEdge = true}, g);
    }

    print_graphviz(g);
    std::cout << std::endl;

}