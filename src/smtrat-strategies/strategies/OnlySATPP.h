#pragma once

#include <smtrat-solver/Manager.h>

#include <smtrat-modules/FPPModule/FPPModule.h>
#include <smtrat-modules/SATModule/SATModule.h>

namespace smtrat
{
    /**
     * A pure SAT solver with preprocessing.
     */
    class OnlySAT:
        public Manager
    {
        public:
            OnlySAT(): Manager() {
				setStrategy({
                    addBackend<FPPModule<FPPSettings1>>({
					    addBackend<SATModule<SATSettings1>>()
                    })
				});
			}

    };

}    // namespace smtrat
