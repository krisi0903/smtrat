#pragma once

#include <smtrat-solver/Manager.h>

#include <smtrat-modules/NewCADModule/NewCADModule.h>
#include <smtrat-modules/SATModule/SATModule.h>

namespace smtrat
{
	class CADPseudorandom: public Manager
	{
		public:
			CADPseudorandom(): Manager() {
				setStrategy({
					addBackend<SATModule<SATSettings1>>({
						addBackend<NewCADModule<NewCADSettingsPseudorandom>>()
					})
				});
			}
	};
}	// namespace smtrat
