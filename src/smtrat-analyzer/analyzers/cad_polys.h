#pragma once

#include "../statistics.h"

#include <smtrat-common/smtrat-common.h>

namespace smtrat::analyzer {

void analyze_cad_polys(const FormulaT& f, AnalyzerStatistics& stats);

}