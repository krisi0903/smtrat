/**
 * @file NRASolver.h
 */
#pragma once

#include <smtrat-solver/Manager.h>

#include <smtrat-modules/FPPModule/FPPModule.h>
#include <smtrat-modules/SATModule/SATModule.h>
#include <smtrat-modules/NewCADModule/NewCADModule.h>

namespace smtrat
{
	/**
	 * Strategy description.
	 *
	 * @author
	 * @since
	 * @version
	 *
	 */
	class Null: public Manager {
	public:
		Null(): Manager() {
			setStrategy({});
		}
	};
} // namespace smtrat
