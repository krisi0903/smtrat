#include "chordal_vargraph_elimination_ordering.h"
#include "CADVOStatistics.h"
#include "elimination_tree.h"
#include "graph_chordality.h"
#include "pseudorandom_ordering.h"
#include "util.h"
#include <algorithm>
#include "graphviz_debug.h"
#include <boost/graph/connected_components.hpp>
#include <numeric>

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


template <typename Graph, typename Compare>
class VariableVertexCompareAdapter {
	Graph const& g;
	Compare c;
	public:
	VariableVertexCompareAdapter(Graph const& g, Compare c) : g(g), c(c) {}

	bool operator()(Vertex<Graph> v1, Vertex<Graph> v2) {
		return c(g[v1].var, g[v2].var);
	}
};

template <bool find_peo>
std::vector<carl::Variable> degree_minimizing_fill(std::vector<Poly> const& polys) {

	vec_order_comp stable_var_comp(pseudorandom_ordering(polys));

	struct DMVertexProp {
		carl::Variable var;
	};

	struct DMEdgeProp {
		std::map<void*, unsigned long long> deg;
	};

	typedef boost::adjacency_list<boost::setS, boost::setS, boost::undirectedS, DMVertexProp, DMEdgeProp> DMGraph;

	std::map<carl::Variable, Vertex<DMGraph>> var_vertex_map;
	DMGraph g;

	carl::carlVariables varset;

	for (const Poly& p : polys) {
		// Make sure that we have a vertex for each variable in our graph
		for (auto var : carl::variables(p)) {
			carl::variables(p, varset);
			if (var_vertex_map.find(var) == var_vertex_map.end()) {
				var_vertex_map[var] = boost::add_vertex({.var = var}, g);
			}
		}

		// for each pair of variables in the polynomial, we add an edge to our graph
		for (auto var1 : carl::variables(p)) {
			for (auto var2 : carl::variables(p)) {
				if (var1 != var2) {
					auto [edge, add_success] = boost::add_edge(var_vertex_map[var1], var_vertex_map[var2], g);
					if (!add_success) {
						// the edge exists
						g[edge].deg[var_vertex_map[var1]] = std::max(g[edge].deg[var_vertex_map[var1]], static_cast<unsigned long long>(p.degree(var1)));
						g[edge].deg[var_vertex_map[var2]] = std::max(g[edge].deg[var_vertex_map[var2]], static_cast<unsigned long long>(p.degree(var2)));
					} else {
						g[edge].deg[var_vertex_map[var1]] = p.degree(var1);
						g[edge].deg[var_vertex_map[var2]] = p.degree(var2);
					}
				}
			}
		}
	}

	std::vector<carl::Variable> peo;
	
	for (int i = boost::num_vertices(g); i > 0; i--) {
		struct vedata_t {
			unsigned long long fill_count = 0;
			unsigned long long fill_degree = 0;
		};

		std::map<Vertex<DMGraph>, vedata_t> vedata_map;

		for (auto [v, v_end] = boost::vertices(g); v != v_end; v++) {
			vedata_map[*v] = {};
			for (auto [a1, a1_end] = boost::adjacent_vertices(*v, g); a1 != a1_end; a1++) {
				for (auto [a2, a2_end] = boost::adjacent_vertices(*v, g); a2 != a2_end; a2++) {
					if (*a1 != *a2) {
						auto [edge, exists] = boost::edge(*a1, *a2, g);
						if (!exists) {
							vedata_map[*v].fill_count++;
						}

						Edge<DMGraph> v_a1 = boost::edge(*v, *a1, g).first;
						Edge<DMGraph> v_a2 = boost::edge(*v, *a2, g).first;

						unsigned long long fill_degree_1 = (g[v_a1].deg[*v] + g[v_a2].deg[*v]) * g[v_a1].deg[*a1];
						unsigned long long fill_degree_2 = (g[v_a1].deg[*v] + g[v_a2].deg[*v]) * g[v_a2].deg[*a2];
						vedata_map[*v].fill_degree += fill_degree_1 + fill_degree_2;
					}
				}
			}
		}

		Vertex<DMGraph> v = std::min_element(vedata_map.begin(), vedata_map.end(), [&](auto const& p1, auto const& p2) {
			if constexpr (find_peo) {
				if (p1.second.fill_count != p2.second.fill_count) return p1.second.fill_count < p2.second.fill_count;
			}
			if (p1.second.fill_degree != p2.second.fill_degree) return p1.second.fill_degree < p2.second.fill_degree;
			return stable_var_comp(g[p1.first].var, g[p2.first].var);
		})->first;


		for (auto [a1, a1_end] = boost::adjacent_vertices(v, g); a1 != a1_end; a1++) {
			for (auto [a2, a2_end] = boost::adjacent_vertices(v, g); a2 != a2_end; a2++) {
				if (*a1 != *a2) {
					Edge<DMGraph> v_a1 = boost::edge(v, *a1, g).first;
					Edge<DMGraph> v_a2 = boost::edge(v, *a2, g).first;

					unsigned long long fill_degree_1 = (g[v_a1].deg[v] + g[v_a2].deg[v]) * g[v_a1].deg[*a1];
					unsigned long long fill_degree_2 = (g[v_a1].deg[v] + g[v_a2].deg[v]) * g[v_a2].deg[*a2];

					auto [edge, add_success] = boost::add_edge(*a1, *a2, g);
					if (!add_success) {
						g[edge].deg[*a1] = std::max(g[edge].deg[*a1], fill_degree_1);
						g[edge].deg[*a2] = std::max(g[edge].deg[*a2], fill_degree_2);
					} else {
						g[edge].deg[*a1] = fill_degree_1;
						g[edge].deg[*a2] = fill_degree_2;
					}
				}
			}
		}

		peo.push_back(g[v].var);

		boost::clear_vertex(v, g);
		boost::remove_vertex(v, g);
	}
	std::vector<carl::Variable> var_vector(std::move(varset.as_vector()));
	assert(std::is_permutation(peo.begin(), peo.end(), var_vector.begin(), var_vector.end()));
	return peo;
}


template std::vector<carl::Variable> degree_minimizing_fill<true>(const std::vector<Poly>& polys);
template std::vector<carl::Variable> degree_minimizing_fill<false>(const std::vector<Poly>& polys);

template<typename Settings>
std::vector<carl::Variable> chordal_vargraph_elimination_ordering(const std::vector<Poly>& polys) {
	SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Building order based on " << polys);

	typedef boost::adjacency_list<boost::setS, boost::setS, boost::undirectedS, VariableVertexProperties, VariableEdgeProperties> ChordalStructure;

	std::map<carl::Variable, Vertex<ChordalStructure>> var_vertex_map;
	ChordalStructure chordal_structure;

#ifdef SMTRAT_DEVOPTION_Statistics
	cadVOStatistics.startTimer("buildVariableGraph");
#endif

	vec_order_comp stable_var_comp(pseudorandom_ordering(polys));

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
				var_vertex_map[var] = boost::add_vertex({.id = id++, .var = var}, chordal_structure);
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


	cadVOStatistics.stopTimer("buildVariableGraph");


	cadVOStatistics._add("ordering.vertices", boost::num_vertices(chordal_structure));
	cadVOStatistics._add("ordering.edges", num_edges(chordal_structure));


	std::map<Vertex<ChordalStructure>, int> component_map;
	int connected_components;
	{
		std::map<Vertex<ChordalStructure>, boost::default_color_type> _colormap;
		connected_components = boost::connected_components(chordal_structure, boost::associative_property_map<std::map<Vertex<ChordalStructure>, int>>(component_map), boost::color_map(boost::associative_property_map<std::map<Vertex<ChordalStructure>, boost::default_color_type>>(_colormap)));
	}

	cadVOStatistics._add("connected_components", connected_components);

	if (connected_components != 1) {
		return Settings::alternative_ordering(polys);
	}

	cadVOStatistics.startTimer("tryBuildPEO");


	auto peo = mcs(chordal_structure);
	auto fill = elimination_game(chordal_structure, peo);


	cadVOStatistics.stopTimer("tryBuildPEO");


	if (fill.size()) {
		cadVOStatistics.startTimer("buildMEO");

		if constexpr (Settings::secondary_ordering) {
			vec_order_comp secondary(Settings::secondary_ordering(polys));
			std::tie(peo, fill) = mcs_m(chordal_structure, VariableVertexCompareAdapter(chordal_structure, vec_order_comp(Settings::secondary_ordering(polys))));
		} else {
			std::tie(peo, fill) = mcs_m(chordal_structure, VariableVertexCompareAdapter(chordal_structure, stable_var_comp));
		}

		// Add the fill edges to the graph structure so that we can pass it to the drawing function
		for (auto const& pair : fill) {
			add_edge(pair.first, pair.second, {.poly = nullptr, .fillEdge = true}, chordal_structure);
		}

		cadVOStatistics.stopTimer("buildMEO");

		SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", "Have " << num_vertices(chordal_structure) << " vertices with " << num_edges(chordal_structure) << " edges. " << fill.size() << " fill-in edges to make graph chordal with given PEO, a percentage of " << 100 * (fill.size() / (double)num_edges(chordal_structure) + fill.size()) << "%'.");

	} 

	cadVOStatistics._add("ordering.filledges", fill.size());
	
	if constexpr (Settings::do_etree) {
		// if the graph is chordal, we can compute the minimum height etree
		// and possibly get a better ordering
		SMTRAT_LOG_DEBUG("smtrat.cad.variableordering", " - computing shortest e-tree");

		cadVOStatistics.startTimer("buildETree");

		EliminationTree<ChordalStructure> t;
		if constexpr (Settings::secondary_ordering) {
			std::tie(t, peo) = min_height_etree(chordal_structure, VariableVertexCompareAdapter(chordal_structure, vec_order_comp(Settings::secondary_ordering(polys))));
		} else {
			std::tie(t, peo) = min_height_etree(chordal_structure, VariableVertexCompareAdapter(chordal_structure, stable_var_comp));
		}
		

		cadVOStatistics.stopTimer("buildETree");

		if constexpr (debugChordalVargraph) {
			cadVOStatistics._add("ordering.etree.graph_dot", print_graphviz_etree<ChordalStructure>(t));
		}

		cadVOStatistics._add("ordering.etree.height", t[boost::graph_bundle].height);
	} else {
		EliminationTree<ChordalStructure> t = etree(chordal_structure, peo);
		cadVOStatistics._add("ordering.etree.height", t[boost::graph_bundle].height);
	}



	if constexpr (debugChordalVargraph) {
		cadVOStatistics._add("ordering.graph_dot", print_graphviz<ChordalStructure>(chordal_structure));
	}


	assert(elimination_game(chordal_structure, peo).size() == 0);

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




template std::vector<carl::Variable> chordal_vargraph_elimination_ordering<ChordalOrderingSettingsBase>(const std::vector<Poly>& polys);
template std::vector<carl::Variable> chordal_vargraph_elimination_ordering<ChordalOrderingSettingsNoETree>(const std::vector<Poly>& polys);
template std::vector<carl::Variable> chordal_vargraph_elimination_ordering<ChordalOrderingSettingsTriangular>(const std::vector<Poly>& polys);

} // namespace smtrat::cad::variable_ordering
