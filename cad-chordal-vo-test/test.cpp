#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <vector>
#include <iostream>
#include <smtrat-cad/variableordering/graph_chordality.h>


struct VariableVertexProperties {
    std::string name;
    unsigned int id;
};


typedef boost::adjacency_list<boost::setS, boost::setS, boost::undirectedS, VariableVertexProperties> Graph;
    

std::ostream& operator<<(std::ostream& os, VariableVertexProperties& props) {
    os << props.name;
    return os;
}



void print_graphviz(Graph const& g) {
    boost::write_graphviz(std::cout, 
    g, 
    boost::make_label_writer(boost::get(&VariableVertexProperties::name, g)),
    boost::default_writer(),
    boost::default_writer(),
    boost::get(&VariableVertexProperties::id, g));
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

    boost::add_edge(vertex_map["y1"], vertex_map["y2"], g);
    boost::add_edge(vertex_map["y1"], vertex_map["y3"], g);
    boost::add_edge(vertex_map["y1"], vertex_map["y4"], g);
    boost::add_edge(vertex_map["y2"], vertex_map["y3"], g);
    boost::add_edge(vertex_map["y3"], vertex_map["y4"], g);
    boost::add_edge(vertex_map["y4"], vertex_map["y2"], g);
    boost::add_edge(vertex_map["y5"], vertex_map["y2"], g);
    boost::add_edge(vertex_map["y5"], vertex_map["y3"], g);

    for (auto v = boost::vertices(g).first; v != boost::vertices(g).second; v++) {
        std::cout << *v << std::endl;
        std::cout << g[*v].name << std::endl;
    }

    print_graphviz(g);

    auto [peo, fill] = smtrat::cad::variable_ordering::mcs_m(g);

    std::cout << "PEO: ";

    for (auto const& item : peo) {
        std::cout << g[item].name << ", ";
    }

    std::cout << std::endl << "Fill Edges:";

    for (auto const& item : fill) {
        std::cout << "(" << g[item.first].name << ", " << g[item.second].name << "), ";
    }
    std::cout << std::endl;

}