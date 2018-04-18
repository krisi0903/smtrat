/**
 * @file CSplitModule.h
 * @author Ömer Sali <oemer.sali@rwth-aachen.de>
 *
 * @version 2018-04-04
 * Created on 2017-11-01.
 */

#pragma once

#include "../../datastructures/VariableBounds.h"
#include "../../solver/Module.h"
#include "../../solver/Manager.h"
#include "CSplitStatistics.h"
#include "CSplitSettings.h"
#include "../SATModule/SATModule.h"
#include "../LRAModule/LRAModule.h"

namespace smtrat
{
	template<typename Settings>
	class CSplitModule : public Module
	{
		private:
#ifdef SMTRAT_DEVOPTION_Statistics
			CSplitStatistics mStatistics;
#endif

			struct Purification
			{
				std::vector<carl::Variable> mSubstitutions;
				carl::Variable mReduction;
				size_t mUsage;
				
				Purification()
					: mReduction(carl::Variable::NO_VARIABLE)
					, mUsage(0)
				{
					mSubstitutions.emplace_back(carl::freshIntegerVariable());
				}
			};
			
			std::map<carl::Monomial::Arg, Purification, std::greater<carl::Monomial::Arg>> mPurifications;
			
			struct Expansion
			{
				Rational mNucleus;
				RationalInterval mMaximalDomain, mActiveDomain;
				std::vector<carl::Variable> mQuotients, mRemainders;
				std::unordered_set<Purification *> mPurifications;
				bool mBoundsChanged;
				
				Expansion()
					: mNucleus(ZERO_RATIONAL)
					, mMaximalDomain(RationalInterval::unboundedInterval())
					, mActiveDomain(RationalInterval::emptyInterval())
					, mBoundsChanged(false)
				{
					mQuotients.emplace_back(carl::freshIntegerVariable());
				}
			};
			
			std::unordered_map<carl::Variable, Expansion> mExpansions;
			
			struct Linearization
			{
				Poly mLinearization;
				std::vector<Purification *> mPurifications;
				std::set<carl::Relation> mRelations;
				bool mHasRealVariables;
				boost::optional<carl::Relation> mActiveRelation;
			};
			
			std::unordered_map<Poly, Linearization> mLinearizations;
			
			std::unordered_set<Linearization *> mChangedLinearizations;
			
			vb::VariableBounds<FormulaT> mVariableBounds;
			
			// Stores whether the last consistency check was done by the backends
			bool mCheckedWithBackends;
			
			/**
			 * Linear arithmetic module to call for the linearized formula
			 */
			struct LAModule : public Manager
			{
				LAModule() : Manager()
				{
					setStrategy({
						addBackend<SATModule<SATSettings1>>({
							addBackend<LRAModule<LRASettings1>>()
						})
					});
				}
			};
			
			/// Handle to the linear arithmetic module
			LAModule mLRAModule;
			
		public:
			typedef Settings SettingsType;
			
			std::string moduleName() const
			{
				return SettingsType::moduleName;
			}
			
			CSplitModule(const ModuleInput* _formula, RuntimeSettings* _settings, Conditionals& _conditionals, Manager* _manager = nullptr);
			
			/**
			 * The module has to take the given sub-formula of the received formula into account.
			 * @param _subformula The sub-formula to take additionally into account.
			 * @return False, if it can be easily decided that this sub-formula causes a conflict with
			 *				  the already considered sub-formulas;
			 *		   True, otherwise.
			 */
			bool addCore( ModuleInput::const_iterator _subformula );
			
			/**
			 * Removes the subformula of the received formula at the given position to the considered ones of this module.
			 * Note that this includes every stored calculation which depended on this subformula, but should keep the other
			 * stored calculation, if possible, untouched.
			 * @param _subformula The position of the subformula to remove.
			 */
			void removeCore( ModuleInput::const_iterator _subformula );
			
			/**
			 * Updates the current assignment into the model.
			 * Note, that this is a unique but possibly symbolic assignment maybe containing newly introduced variables.
			 */
			void updateModel() const;
			
			/**
			 * Checks the received formula for consistency.
			 * @return True,	if the received formula is satisfiable;
			 *		   False,   if the received formula is not satisfiable;
			 *		   Unknown, otherwise.
			 */
			Answer checkCore();
		
		private:
			bool resetExpansions();
			bool bloatDomains(const FormulaSetT& conflict);
			Answer analyzeConflict(const FormulaSetT& conflict);
			
			void changeActiveDomain(Expansion& expansion, RationalInterval domain);
			
			inline void propagateLinearCaseSplits(const Purification& purification, const RationalInterval& interval, size_t i, bool assert);
			inline void propagateLogarithmicCaseSplits(const Purification& purification, size_t i, bool assert);
			inline void propagateFormula(const FormulaT& formula, bool assert);
	};
}
