#include "variables.h"
#include "utils.h"
#include "../settings.h"

#include <smtrat-cad/common.h>
#include <smtrat-cad/Settings.h>
#include <smtrat-cad/projection/Projection.h>
#include <smtrat-cad/projectionoperator/utils.h>
#include <smtrat-cad/utils/CADConstraints.h>
#include <smtrat-cad/variableordering/triangular_ordering.h>
#include <smtrat-cad/variableordering/chordal_vargraph_elimination_ordering.h>

namespace smtrat::analyzer {


void analyze_cad_polys(const FormulaT& f, AnalyzerStatistics& stats) {

	std::vector<Poly> polys;

	carl::visit(f, [&](const FormulaT& f){
		if (f.type() == carl::FormulaType::CONSTRAINT) {
			polys.push_back(f.constraint().lhs());
		}
	});

	DegreeCollector dc;
	for (Poly p : polys ) {
        dc(p);
    }

    std::string prefix = "polys";
	stats.add(prefix + "_size", polys.size());
	stats.add(prefix + "_deg_max", dc.degree_max);
	stats.add(prefix + "_deg_sum", dc.degree_sum);
	stats.add(prefix + "_tdeg_max", dc.tdegree_max);
	stats.add(prefix + "_tdeg_sum", dc.tdegree_sum);

	
}

}