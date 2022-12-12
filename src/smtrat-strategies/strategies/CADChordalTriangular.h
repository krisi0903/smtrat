#pragma once

#include <smtrat-solver/Manager.h>

#include <smtrat-modules/NewCADModule/NewCADModule.h>
#include <smtrat-modules/SATModule/SATModule.h>

namespace smtrat
{
	class CADChordalTriangular: public Manager
	{
		public:
			CADChordalTriangular(): Manager() {
				setStrategy({
					addBackend<SATModule<SATSettings1>>({
						addBackend<NewCADModule<NewCADSettingsChordalWithTriangularSecondary>>()
					})
				});
			}
	};
}	// namespace smtrat
