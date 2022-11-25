#include <smtrat-cad/variableordering/CADVOStatistics.h>
#ifdef SMTRAT_DEVOPTION_Statistics
namespace smtrat::cad::variable_ordering {
	CADVOStatistics& cadVOStatistics = statistics_get<CADVOStatistics>("CADVOStatistics");
}
#endif