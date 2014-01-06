/*
 *  SMT-RAT - Satisfiability-Modulo-Theories Real Algebra Toolbox
 * Copyright (C) 2012 Florian Corzilius, Ulrich Loup, Erika Abraham, Sebastian Junges
 *
 * This file is part of SMT-RAT.
 *
 * SMT-RAT is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SMT-RAT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SMT-RAT.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * File:   VSModule.cpp
 * @author Florian Corzilius <corzilius@cs.rwth-aachen.de>
 * @since 2010-05-11
 * @version 2013-06-20
 */

#include <set>

#include "State.h"
#include "VSModule.h"

using namespace std;
using namespace vs;

//#define VS_DEBUG

namespace smtrat
{
    template<class Settings>
    VSModule<Settings>::VSModule( ModuleType _type, const Formula* const _formula, RuntimeSettings* settings, Conditionals& _conditionals, Manager* const _manager ):
        Module( _type, _formula, _conditionals, _manager ),
        mConditionsChanged( false ),
        mInconsistentConstraintAdded( false ),
        mIDCounter( 0 ),
        #ifdef VS_STATISTICS
        mStepCounter( 0 ),
        #endif
        mpStateTree( new State( Settings::use_variable_bounds ) ),
        mAllVariables(),
        mFormulaConditionMap(),
        mRanking(),
        mVariableVector()
    {}

    template<class Settings>
    VSModule<Settings>::~VSModule()
    {
        while( !mFormulaConditionMap.empty() )
        {
            const vs::Condition* pRecCond = mFormulaConditionMap.begin()->second;
            mFormulaConditionMap.erase( mFormulaConditionMap.begin() );
            delete pRecCond;
            pRecCond = NULL;
        }
        delete mpStateTree;
    }

    /**
     * Adds a constraint to the so far received constraints.
     *
     * @param _subformula The position of the constraint within the received constraints.
     * @return false, if a conflict is detected;
     *          true,  otherwise.
     */
    template<class Settings>
    bool VSModule<Settings>::assertSubformula( Formula::const_iterator _subformula )
    {
        Module::assertSubformula( _subformula );
        if( (*_subformula)->getType() == CONSTRAINT )
        {
            const Constraint* constraint = (*_subformula)->pConstraint();
            const vs::Condition* condition  = new vs::Condition( constraint );
            mFormulaConditionMap[*_subformula] = condition;
            // Clear the ranking.
            switch( constraint->isConsistent() )
            {
                case 0:
                {
                    removeStatesFromRanking( *mpStateTree );
                    mIDCounter = 0;
                    mInfeasibleSubsets.clear();
                    mInfeasibleSubsets.push_back( set<const Formula*>() );
                    mInfeasibleSubsets.back().insert( *_subformula );
                    mInconsistentConstraintAdded = true;
                    foundAnswer( False );
                    return false;
                }
                case 1:
                {
                    return true;
                }
                case 2:
                {
                    mIDCounter = 0;
                    for( auto var = constraint->variables().begin(); var != constraint->variables().end(); ++var )
                        mAllVariables.insert( *var );
                    if( Settings::incremental_solving )
                    {
                        removeStatesFromRanking( *mpStateTree );
                        vs::Condition::Set oConds = vs::Condition::Set();
                        oConds.insert( condition );
                        vector<DisjunctionOfConditionConjunctions> subResults = vector<DisjunctionOfConditionConjunctions>();
                        DisjunctionOfConditionConjunctions subResult = DisjunctionOfConditionConjunctions();
                        ConditionList condVector = ConditionList();
                        condVector.push_back( new vs::Condition( constraint, 0, false, oConds ) );
                        subResult.push_back( condVector );
                        subResults.push_back( subResult );
                        mpStateTree->addSubstitutionResults( subResults );
                        addStateToRanking( mpStateTree );
                        insertTooHighDegreeStatesInRanking( mpStateTree );
                    }
                    mConditionsChanged = true;
                    return true;
                }
                default:
                {
                    assert( false );
                    return true;
                }
            }
        }
        return true;
    }

    /**
     * Removes a constraint of the so far received constraints.
     *
     * @param _subformula The position of the constraint within the received constraints.
     */
    template<class Settings>
    void VSModule<Settings>::removeSubformula( Formula::const_iterator _subformula )
    {
        if( (*_subformula)->getType() == CONSTRAINT )
        {
            mInconsistentConstraintAdded = false;
            auto formulaConditionPair = mFormulaConditionMap.find( *_subformula );
            assert( formulaConditionPair != mFormulaConditionMap.end() );
            const vs::Condition* condToDelete = formulaConditionPair->second;
            if( Settings::incremental_solving )
            {
                removeStatesFromRanking( *mpStateTree );
                mpStateTree->rSubResultsSimplified() = false;
                set<const vs::Condition*> condsToDelete = set<const vs::Condition*>();
                condsToDelete.insert( condToDelete );
                mpStateTree->deleteOrigins( condsToDelete );
                mpStateTree->rType() = State::COMBINE_SUBRESULTS;
                mpStateTree->rTakeSubResultCombAgain() = true;
                addStateToRanking( mpStateTree );
                insertTooHighDegreeStatesInRanking( mpStateTree );
            }
            mFormulaConditionMap.erase( formulaConditionPair );
            delete condToDelete;
            condToDelete = NULL;
            mConditionsChanged = true;
        }
        Module::removeSubformula( _subformula );
    }

    /**
     * Checks the consistency of the so far received constraints.
     *
     * @return True,    if the so far received constraints are consistent;
     *          False,   if the so far received constraints are inconsistent;
     *          Unknown, if this module cannot determine whether the so far received constraints are consistent or not.
     */
    template<class Settings>
    Answer VSModule<Settings>::isConsistent()
    {
        #ifdef VS_STATISTICS
        mStepCounter = 0;
        #endif
        if( !Settings::incremental_solving )
        {
            removeStatesFromRanking( *mpStateTree );
            delete mpStateTree;
            mpStateTree = new State( Settings::use_variable_bounds );
            for( auto iter = mFormulaConditionMap.begin(); iter != mFormulaConditionMap.end(); ++iter )
            {
                vs::Condition::Set oConds = vs::Condition::Set();
                oConds.insert( iter->second );
                vector<DisjunctionOfConditionConjunctions> subResults = vector<DisjunctionOfConditionConjunctions>();
                DisjunctionOfConditionConjunctions subResult = DisjunctionOfConditionConjunctions();
                ConditionList condVector = ConditionList();
                condVector.push_back( new vs::Condition( iter->first->pConstraint(), 0, false, oConds ) );
                subResult.push_back( condVector );
                subResults.push_back( subResult );
                mpStateTree->addSubstitutionResults( subResults );
            }
            addStateToRanking( mpStateTree );
        }
        assert( Settings::real_variables || Settings::integer_variables );
        if( Settings::real_variables && !Settings::integer_variables && !mpReceivedFormula->isRealConstraintConjunction() )
            return foundAnswer( Unknown );
        if( !Settings::real_variables && Settings::integer_variables && !mpReceivedFormula->isIntegerConstraintConjunction() )
            return foundAnswer( Unknown );
        if( Settings::real_variables && Settings::integer_variables && !mpReceivedFormula->isConstraintConjunction() )
            return foundAnswer( Unknown );
        assert( mpReceivedFormula->size() == mFormulaConditionMap.size() );
        if( !mConditionsChanged )
        {
            if( mInfeasibleSubsets.empty() )
            {
                if( solverState() == True )
                    return consistencyTrue();
                else
                {
                    return foundAnswer( Unknown );
                }
            }
            else
                return foundAnswer( False );
        }
        mConditionsChanged = false;
        if( mpReceivedFormula->empty() )
            return consistencyTrue();
        if( mInconsistentConstraintAdded )
        {
            assert( !mInfeasibleSubsets.empty() );
            assert( !mInfeasibleSubsets.back().empty() );
            return foundAnswer( False );
        }
        while( !mRanking.empty() )
        {
//                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            if( anAnswerFound() )
                return foundAnswer( Unknown );
//            else
//                cout << "VSModule iteration" << endl;
            #ifdef VS_STATISTICS
            ++mStepCounter;
            #endif
//            if( mStepCounter >= 21520 )
//            {
//                assert(false);
//            }
//            cout << "Iteration:  " << mStepCounter << endl;
            State* currentState = mRanking.begin()->second;
            #ifdef VS_DEBUG
            cout << "Ranking:" << endl;
            for( auto valDTPair = mRanking.begin(); valDTPair != mRanking.end(); ++valDTPair )
            {
                stringstream stream;
                stream << "(" << valDTPair->first.first << ", " << valDTPair->first.second.first << ", " << valDTPair->first.second.second << ")";
                cout << setw(15) << stream.str();
                cout << ":  " << valDTPair->second << endl;
            }
            cout << "*** Considered state:" << endl;
            currentState->printAlone( "*** ", cout );
            #endif
            currentState->simplify();
            #ifdef VS_DEBUG
            cout << "Simplifing results in " << endl;
            currentState->printAlone( "*** ", cout );
            #endif
            if( currentState->hasChildrenToInsert() )
            {
                currentState->rHasChildrenToInsert() = false;
                addStatesToRanking( currentState );
            }
            else
            {
                if( currentState->isInconsistent() )
                {
                    #ifdef VS_LOG_INTERMEDIATE_STEPS
                    logConditions( *currentState, false, "Intermediate_conflict_of_VSModule" );
                    #endif
                    removeStatesFromRanking( *currentState );
                    if( currentState->isRoot() )
                    {
                        updateInfeasibleSubset();
                        return foundAnswer( False );
                    }
                    else
                    {
                        currentState->passConflictToFather( Settings::check_conflict_for_side_conditions );
                        removeStateFromRanking( currentState->rFather() );
                        addStateToRanking( currentState->pFather() );
                    }
                }
                else if( currentState->takeSubResultCombAgain() && currentState->type() == State::COMBINE_SUBRESULTS )
                {
                    #ifdef VS_DEBUG
                    cout << "*** Refresh conditons:" << endl;
                    #endif
                    currentState->refreshConditions();
                    currentState->rTakeSubResultCombAgain() = false;
                    #ifdef VS_DEBUG
                    currentState->printAlone( "   ", cout );
                    cout << "*** Conditions refreshed." << endl;
                    #endif
                }
                else if( currentState->hasRecentlyAddedConditions() )//&& !(currentState->takeSubResultCombAgain() && currentState->isRoot() ) )
                {
                    #ifdef VS_DEBUG
                    cout << "*** Propagate new conditions :" << endl;
                    #endif
                    propagateNewConditions(currentState);
                    #ifdef VS_DEBUG
                    cout << "*** Propagate new conditions ready." << endl;
                    #endif
                }
                else
                {
                    #ifdef SMTRAT_VS_VARIABLEBOUNDS
                    if( !currentState->checkTestCandidatesForBounds() )
                    {
                        currentState->rInconsistent() = true;
                        removeStatesFromRanking( *currentState );
                    }
                    else
                    {
                        #endif
                        switch( currentState->type() )
                        {
                            case State::SUBSTITUTION_TO_APPLY:
                            {
                                #ifdef VS_DEBUG
                                cout << "*** SubstituteAll changes it to:" << endl;
                                #endif
                                if( !substituteAll( currentState, currentState->rFather().rConditions() ) )
                                {
                                    // Delete the currently considered state.
                                    currentState->rInconsistent() = true;
                                    removeStateFromRanking( *currentState );
                                }
                                #ifdef VS_DEBUG
                                cout << "*** SubstituteAll ready." << endl;
                                #endif
                                break;
                            }
                            case State::COMBINE_SUBRESULTS:
                            {
                                #ifdef VS_DEBUG
                                cout << "*** Refresh conditons:" << endl;
                                #endif
                                if( currentState->nextSubResultCombination() )
                                {
                                    if( currentState->refreshConditions() )
                                        addStateToRanking( currentState );
                                    else 
                                        addStatesToRanking( currentState );
                                    #ifdef VS_DEBUG
                                    currentState->printAlone( "   ", cout );
                                    #endif
                                }
                                else
                                {
                                    // If it was the last combination, delete the state.
                                    currentState->rInconsistent() = true;
                                    removeStatesFromRanking( *currentState );
                                    currentState->rFather().rMarkedAsDeleted() = false;
                                    addStateToRanking( currentState->pFather() );

                                }
                                #ifdef VS_DEBUG
                                cout << "*** Conditions refreshed." << endl;
                                #endif
                                break;
                            }
                            case State::TEST_CANDIDATE_TO_GENERATE:
                            {
                                // Set the index, if not already done, to the best variable to eliminate next.
                                if( currentState->index() == carl::Variable::NO_VARIABLE ) 
                                    currentState->initIndex( mAllVariables, Settings::prefer_equation_over_all );
                                else if( currentState->tryToRefreshIndex() )
                                {
                                    if( currentState->initIndex( mAllVariables, Settings::prefer_equation_over_all ) )
                                    {
                                        currentState->initConditionFlags();
                                        currentState->resetConflictSets();
                                        while( !currentState->children().empty() )
                                        {
                                            State* toDelete = currentState->rChildren().back();
                                            removeStatesFromRanking( *toDelete );
                                            currentState->rChildren().pop_back();
                                            delete toDelete;
                                        }
                                    }
                                }
                                // Find the most adequate conditions to continue.
                                const vs::Condition* currentCondition;
                                if( !currentState->bestCondition( currentCondition, mAllVariables.size(), Settings::prefer_equation_over_all ) )
                                {
                                    if( currentState->tooHighDegreeConditions().empty() )
                                    {
                                        // It is a state, where no more elimination could be applied to the conditions.
                                        if( currentState->conditions().empty() )
                                        {
                                            #ifdef VS_DEBUG
                                            cout << "*** Check ancestors!" << endl;
                                            #endif
                                            // Check if there are still conditions in any ancestor, which have not been considered.
                                            State * unfinishedAncestor;
                                            if( currentState->unfinishedAncestor( unfinishedAncestor ) )
                                            {
                                                // Go back to this ancestor and refine.
                                                removeStatesFromRanking( *unfinishedAncestor );
                                                unfinishedAncestor->extendSubResultCombination();
                                                unfinishedAncestor->rType() = State::COMBINE_SUBRESULTS;
                                                if( unfinishedAncestor->refreshConditions() ) 
                                                    addStateToRanking( unfinishedAncestor );
                                                else 
                                                    addStatesToRanking( unfinishedAncestor );
                                                #ifdef VS_DEBUG
                                                cout << "*** Found an unfinished ancestor:" << endl;
                                                unfinishedAncestor->printAlone();
                                                #endif
                                            }
                                            else // Solution.
                                                return consistencyTrue();
                                        }
                                        // It is a state, where all conditions have been used for test candidate generation.
                                        else
                                        {
                                            // Check whether there are still test candidates in form of children left.
                                            bool currentStateHasChildrenToConsider = false;
                                            bool currentStateHasChildrenWithToHighDegree = false;
                                            auto child = currentState->rChildren().begin();
                                            while( child != currentState->children().end() )
                                            {
                                                if( !(**child).isInconsistent() )
                                                {
                                                    if( !(**child).markedAsDeleted() )
                                                        addStateToRanking( *child );
                                                    if( !(**child).tooHighDegree() && !(**child).markedAsDeleted() )
                                                        currentStateHasChildrenToConsider = true;
                                                    else 
                                                        currentStateHasChildrenWithToHighDegree = true;
                                                }
                                                child++;
                                            }

                                            if( !currentStateHasChildrenToConsider )
                                            {
                                                if( !currentStateHasChildrenWithToHighDegree )
                                                {
                                                    currentState->rInconsistent() = true;
                                                    #ifdef VS_LOG_INTERMEDIATE_STEPS
                                                    logConditions( *currentState, false, "Intermediate_conflict_of_VSModule" );
                                                    #endif
                                                    removeStatesFromRanking( *currentState );
                                                    if( currentState->isRoot() )
                                                        updateInfeasibleSubset();
                                                    else
                                                    {
                                                        currentState->passConflictToFather( Settings::check_conflict_for_side_conditions );
                                                        removeStateFromRanking( currentState->rFather() );
                                                        addStateToRanking( currentState->pFather() );
                                                    }
                                                }
                                                else
                                                {
                                                    currentState->rMarkedAsDeleted() = true;
                                                    removeStateFromRanking( *currentState );
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if( (*currentState).tooHighDegree() )
                                        {
                                            // If we need to involve another approach.
                                            Answer result = runBackendSolvers( currentState );
                                            switch( result )
                                            {
                                                case True:
                                                {
                                                    currentState->rToHighDegree() = true;
                                                    State * unfinishedAncestor;
                                                    if( currentState->unfinishedAncestor( unfinishedAncestor ) )
                                                    {
                                                        // Go back to this ancestor and refine.
                                                        removeStatesFromRanking( *unfinishedAncestor );
                                                        unfinishedAncestor->extendSubResultCombination();
                                                        unfinishedAncestor->rType() = State::COMBINE_SUBRESULTS;
                                                        if( unfinishedAncestor->refreshConditions() )
                                                            addStateToRanking( unfinishedAncestor );
                                                        else
                                                            addStatesToRanking( unfinishedAncestor );
                                                    }
                                                    else // Solution.
                                                        return consistencyTrue();
                                                    break;
                                                }
                                                case False:
                                                {
                                                    break;
                                                }
                                                case Unknown:
                                                {
                                                    return foundAnswer( Unknown );
                                                }
                                                default:
                                                {
                                                    cout << "Error: Unknown answer in method " << __func__ << " line " << __LINE__ << endl;
                                                    return foundAnswer( Unknown );
                                                }
                                            }
                                        }
                                        else
                                        {
                                            currentState->rToHighDegree() = true;
                                            addStateToRanking( currentState );
                                        }
                                    }
                                }
                                // Generate test candidates for the chosen variable and the chosen condition.
                                else
                                {
                                    if( Settings::local_conflict_search && currentState->hasLocalConflict() )
                                    {
                                        removeStatesFromRanking( *currentState );
                                        addStateToRanking( currentState );
                                    }
                                    else
                                    {
                                        #ifdef VS_DEBUG
                                        cout << "*** Eliminate " << currentState->index() << " in ";
                                        cout << currentCondition->constraint().toString( 0, true, true );
                                        cout << " creates:" << endl;
                                        #endif
                                        eliminate( currentState, currentState->index(), currentCondition );
                                        #ifdef VS_DEBUG
                                        cout << "*** Eliminate ready." << endl;
                                        #endif
                                    }
                                }
                                break;
                            }
                            default:
                                assert( false );
                        }
                        #ifdef SMTRAT_VS_VARIABLEBOUNDS
                    }
                    #endif
                }
            }
        }
        #ifdef VS_LOG_INTERMEDIATE_STEPS
        if( mpStateTree->conflictSets().empty() ) logConditions( *mpStateTree, false, "Intermediate_conflict_of_VSModule" );
        #endif
        assert( !mpStateTree->conflictSets().empty() );
        updateInfeasibleSubset();
        #ifdef VS_DEBUG
        printAll();
        #endif
        return foundAnswer( False );
    }

    /**
     * Updates the model, if the received formula was found to be satisfiable by this module.
     */
    template<class Settings>
    void VSModule<Settings>::updateModel() const
    {
        clearModel();
        if( solverState() == True )
        {
            for( unsigned i = mVariableVector.size(); i<=mRanking.begin()->second->treeDepth(); ++i )
            {
                stringstream outA;
                outA << "m_inf_" << id() << "_" << i;
                carl::Variable minfVar( Formula::newAuxiliaryRealVariable( outA.str() ) );
                stringstream outB;
                outB << "eps_" << id() << "_" << i;
                carl::Variable epsVar( Formula::newAuxiliaryRealVariable( outB.str() ) );
                mVariableVector.push_back( pair<carl::Variable,carl::Variable>( minfVar, epsVar ) );
            }
            assert( !mRanking.empty() );
            Variables allVarsInRoot;
            mpStateTree->variables( allVarsInRoot );
            const State* state = mRanking.begin()->second;
            while( !state->isRoot() )
            {
                const Substitution& sub = state->substitution();
                Assignment ass;
                ass.theoryValue = NULL;
                if( sub.type() == Substitution::MINUS_INFINITY )
                    ass.theoryValue = new SqrtEx( Polynomial( mVariableVector.at( state->treeDepth()-1 ).first ) );
                else
                {
                    if( state->substitution().variable().getType() == carl::VariableType::VT_INT )
                    {
                        Rational valueRational;
                        sub.term().evaluate( valueRational, getIntervalAssignment( state ), 0 );
                        ass.theoryValue = new SqrtEx( Polynomial( valueRational ) );
                    }
                    else
                    {
                        ass.theoryValue = new SqrtEx( sub.term() );
                        if( sub.type() == Substitution::PLUS_EPSILON )
                            *(ass.theoryValue) = (*(ass.theoryValue)) + SqrtEx( Polynomial( mVariableVector.at( state->treeDepth()-1 ).second ) );
                    }
                }
                mModel.insert( std::pair< const carl::Variable, Assignment >( state->substitution().variable(), ass ) );
                state = state->pFather();
            }
            if( mRanking.begin()->second->tooHighDegree() )
                Module::getBackendsModel();
            // All variables which occur in the root of the constructed state tree but were incidentally eliminated
            // (during the elimination of another variable) can have an arbitrary assignment. If the variable has the
            // real domain, we leave at as a parameter, and, if it has the integer domain we assign 0 to it.
            for( auto var = allVarsInRoot.begin(); var != allVarsInRoot.end(); ++var )
            {
                Assignment ass;
                ass.theoryValue = new SqrtEx( var->getType() == carl::VariableType::VT_INT ? ZERO_POLYNOMIAL : Polynomial( *var ) );
                // Note, that this assignment won't take effect if the variable got an assignment by a backend module.
                mModel.insert( std::pair< const carl::Variable, Assignment >( *var, ass ) ); 
            }
        }
    }
    
    template<class Settings>
    Answer VSModule<Settings>::consistencyTrue()
    {
        #ifdef VS_LOG_INTERMEDIATE_STEPS
        checkAnswer();
        #endif
        #ifdef VS_PRINT_ANSWERS
        printAnswer();
        #endif
        if( Settings::integer_variables )
        {
            #ifdef VS_DEBUG
            printAll();
            #endif
            return solutionInDomain();
        }
        else
            return foundAnswer( True );
    }

    /**
     * Eliminates the given variable by finding test candidates of the constraint of the given
     * condition. All this happens in the state _currentState.
     *
     * @param _currentState   The currently considered state.
     * @param _eliminationVar The substitution to apply.
     * @param _condition      The condition with the constraint, in which should be substituted.
     *
     * @sideeffect: For each test candidate a new child of the currently considered state
     *              is generated. The solved constraint in the currently considered
     *              state is now labeled by true, which means, that the constraint
     *              already served to eliminate for the respective variable in this
     *              state.
     */
    template<class Settings>
    void VSModule<Settings>::eliminate( State* _currentState, const carl::Variable& _eliminationVar, const vs::Condition* _condition )
    {
        // Get the constraint of this condition.
        const Constraint* constraint = (*_condition).pConstraint();
        assert( _condition->constraint().hasVariable( _eliminationVar ) );
        bool generatedTestCandidateBeingASolution = false;
        unsigned numberOfAddedChildren = 0;
        vs::Condition::Set oConditions = vs::Condition::Set();
        oConditions.insert( _condition );
        #ifdef SMTRAT_VS_VARIABLEBOUNDS
        if( !Settings::use_variable_bounds || _currentState->hasRootsInVariableBounds( _condition, Settings::sturm_sequence_for_root_check ) )
        {
            #endif
            Constraint::Relation relation = (*_condition).constraint().relation();
            if( !Settings::use_strict_inequalities_for_test_candidate_generation )
            {
                if( relation == Constraint::LESS || relation == Constraint::GREATER || relation == Constraint::NEQ )
                {
                    _currentState->rTooHighDegreeConditions().insert( _condition );
                    return;
                }
            }
            // Determine the substitution type: normal or +epsilon
            bool weakConstraint = (relation == Constraint::EQ || relation == Constraint::LEQ || relation == Constraint::GEQ);
            Substitution::Type subType = (Settings::integer_variables || weakConstraint) ? Substitution::NORMAL : Substitution::PLUS_EPSILON;
            vector< Polynomial > factors = vector< Polynomial >();
            PointerSet<Constraint> sideConditions = PointerSet<Constraint>();
            if( Settings::elimination_with_factorization && constraint->hasFactorization() )
            {
                for( auto iter = constraint->factorization().begin(); iter != constraint->factorization().end(); ++iter )
                {
                    Variables factorVars;
                    iter->first.gatherVariables( factorVars );
                    if( factorVars.find( _eliminationVar ) != factorVars.end() )
                        factors.push_back( iter->first );
                    else
                    {
                        const smtrat::Constraint* cons = smtrat::Formula::newConstraint( iter->first, Constraint::NEQ );
                        if( cons != Formula::constraintPool().consistentConstraint() )
                        {
                            assert( cons != Formula::constraintPool().inconsistentConstraint() );
                            sideConditions.insert( cons );
                        }
                    }
                }
            }
            else
                factors.push_back( constraint->lhs() );
            for( auto factor = factors.begin(); factor != factors.end(); ++factor )
            {
                #ifdef VS_DEBUG
                cout << "Eliminate for " << *factor << endl;
                #endif
                VarInfo varInfo = factor->getVarInfo<true>( _eliminationVar );
                const map<unsigned, Polynomial>& coeffs = varInfo.coeffs();
                assert( !coeffs.empty() );
                // Generate test candidates for the chosen variable considering the chosen constraint.
                switch( coeffs.rbegin()->first )
                {
                    case 0:
                    {
                        assert( false );
                        break;
                    }
                    //degree = 1
                    case 1:
                    {
                        // Create state ({b!=0} + oldConditions, [x -> -c/b]):
                        Polynomial constantCoeff;
                        auto iter = coeffs.find( 0 );
                        if( iter != coeffs.end() ) constantCoeff = iter->second;
                        const smtrat::Constraint* cons = smtrat::Formula::newConstraint( coeffs.rbegin()->second, Constraint::NEQ );
                        if( cons == Formula::constraintPool().inconsistentConstraint() )
                        {
                            if( relation == Constraint::EQ )
                                generatedTestCandidateBeingASolution = sideConditions.empty();
                        }
                        else
                        {
                            PointerSet<Constraint> sideCond = sideConditions;
                            if( cons != Formula::constraintPool().consistentConstraint() )
                                sideCond.insert( cons );
                            SqrtEx sqEx = SqrtEx( -constantCoeff, ZERO_POLYNOMIAL, coeffs.rbegin()->second, ZERO_POLYNOMIAL );
                            if( Settings::integer_variables && !weakConstraint )
                                sqEx = sqEx + SqrtEx( ONE_POLYNOMIAL );
                            Substitution sub = Substitution( _eliminationVar, sqEx, subType, oConditions, sideCond );
                            if( _currentState->addChild( sub ) )
                            {
                                if( relation == Constraint::EQ && !_currentState->children().back()->hasSubstitutionResults() )
                                {
                                    _currentState->rChildren().back()->setOriginalCondition( _condition );
                                    generatedTestCandidateBeingASolution = true;
                                }
                                // Add its valuation to the current ranking.
                                addStateToRanking( (*_currentState).rChildren().back() );
                                ++numberOfAddedChildren;
                                #ifdef VS_DEBUG
                                (*(*_currentState).rChildren().back()).print( "   ", cout );
                                #endif
                            }
                        }
                        break;
                    }
                    //degree = 2
                    case 2:
                    {
                        Polynomial constantCoeff;
                        auto iter = coeffs.find( 0 );
                        if( iter != coeffs.end() ) constantCoeff = iter->second;
                        Polynomial linearCoeff;
                        iter = coeffs.find( 1 );
                        if( iter != coeffs.end() ) linearCoeff = iter->second;
                        Polynomial radicand = linearCoeff.pow( 2 ) - Rational( 4 ) * coeffs.rbegin()->second * constantCoeff;
                        bool constraintHasZeros = false;
                        // Create state ({a==0, b!=0} + oldConditions, [x -> -c/b]):
                        const smtrat::Constraint* cons11 = smtrat::Formula::newConstraint( coeffs.rbegin()->second, Constraint::EQ );
                        if( cons11 != Formula::constraintPool().inconsistentConstraint() )
                        {
                            const smtrat::Constraint* cons12 = smtrat::Formula::newConstraint( linearCoeff, Constraint::NEQ );
                            if( cons12 != Formula::constraintPool().inconsistentConstraint() )
                            {
                                PointerSet<Constraint> sideCond = sideConditions;
                                if( cons11 != Formula::constraintPool().consistentConstraint() )
                                    sideCond.insert( cons11 );
                                if( cons12 != Formula::constraintPool().consistentConstraint() )
                                    sideCond.insert( cons12 );
                                SqrtEx sqEx = SqrtEx( -constantCoeff, ZERO_POLYNOMIAL, linearCoeff, ZERO_POLYNOMIAL );
                                if( Settings::integer_variables && !weakConstraint )
                                    sqEx = sqEx + SqrtEx( ONE_POLYNOMIAL );
                                Substitution sub = Substitution( _eliminationVar, sqEx, subType, oConditions, sideCond );
                                if( _currentState->addChild( sub ) )
                                {
                                    if( relation == Constraint::EQ && !_currentState->children().back()->hasSubstitutionResults() )
                                    {
                                        _currentState->rChildren().back()->setOriginalCondition( _condition );
                                        generatedTestCandidateBeingASolution = true;
                                    }
                                    // Add its valuation to the current ranking.
                                    addStateToRanking( (*_currentState).rChildren().back() );
                                    ++numberOfAddedChildren;
                                    #ifdef VS_DEBUG
                                    (*(*_currentState).rChildren().back()).print( "   ", cout );
                                    #endif
                                }
                                constraintHasZeros = true;
                            }
                        }
                        // Create state ({a!=0, b^2-4ac>=0} + oldConditions, [x -> (-b+sqrt(b^2-4ac))/2a]):
                        const smtrat::Constraint* cons21 = smtrat::Formula::newConstraint( coeffs.rbegin()->second, Constraint::NEQ );
                        if( cons21 != Formula::constraintPool().inconsistentConstraint() )
                        {
                            const smtrat::Constraint* cons22 = smtrat::Formula::newConstraint( radicand, Constraint::GEQ );
                            if( cons22 != Formula::constraintPool().inconsistentConstraint() )
                            {
                                PointerSet<Constraint> sideCond = sideConditions;
                                if( cons21 != Formula::constraintPool().consistentConstraint() )
                                    sideCond.insert( cons21 );
                                if( cons22 != Formula::constraintPool().consistentConstraint() )
                                    sideCond.insert( cons22 );
                                SqrtEx sqEx = SqrtEx( -linearCoeff, ONE_POLYNOMIAL, Rational( 2 ) * coeffs.rbegin()->second, radicand );
                                if( Settings::integer_variables && !weakConstraint )
                                    sqEx = sqEx + SqrtEx( ONE_POLYNOMIAL );
                                Substitution sub = Substitution( _eliminationVar, sqEx, subType, oConditions, sideCond );
                                if( _currentState->addChild( sub ) )
                                {
                                    if( relation == Constraint::EQ && !_currentState->children().back()->hasSubstitutionResults() )
                                    {
                                        _currentState->rChildren().back()->setOriginalCondition( _condition );
                                        generatedTestCandidateBeingASolution = true;
                                    }
                                    // Add its valuation to the current ranking.
                                    addStateToRanking( (*_currentState).rChildren().back() );
                                    ++numberOfAddedChildren;
                                    #ifdef VS_DEBUG
                                    (*(*_currentState).rChildren().back()).print( "   ", cout );
                                    #endif
                                }
                                constraintHasZeros = true;
                            }
                        }
                        // Create state ({a!=0, b^2-4ac>=0} + oldConditions, [x -> (-b-sqrt(b^2-4ac))/2a]):
                        const smtrat::Constraint* cons31 = smtrat::Formula::newConstraint( coeffs.rbegin()->second, Constraint::NEQ );
                        if( cons31 != Formula::constraintPool().inconsistentConstraint() )
                        {
                            const smtrat::Constraint* cons32 = smtrat::Formula::newConstraint( radicand, Constraint::GEQ );
                            if( cons32 != Formula::constraintPool().inconsistentConstraint() )
                            {
                                PointerSet<Constraint> sideCond = sideConditions;
                                if( cons31 != Formula::constraintPool().consistentConstraint() )
                                    sideCond.insert( cons31 );
                                if( cons32 != Formula::constraintPool().consistentConstraint() )
                                    sideCond.insert( cons32 );
                                SqrtEx sqEx = SqrtEx( -linearCoeff, MINUS_ONE_POLYNOMIAL, Rational( 2 ) * coeffs.rbegin()->second, radicand );
                                if( Settings::integer_variables && !weakConstraint )
                                    sqEx = sqEx + SqrtEx( ONE_POLYNOMIAL );
                                Substitution sub = Substitution( _eliminationVar, sqEx, subType, oConditions, sideCond );
                                if( _currentState->addChild( sub ) )
                                {
                                    if( relation == Constraint::EQ && !_currentState->children().back()->hasSubstitutionResults() )
                                    {
                                        _currentState->rChildren().back()->setOriginalCondition( _condition );
                                        generatedTestCandidateBeingASolution = true;
                                    }
                                    // Add its valuation to the current ranking.
                                    addStateToRanking( (*_currentState).rChildren().back() );
                                    ++numberOfAddedChildren;
                                    #ifdef VS_DEBUG
                                    (*(*_currentState).rChildren().back()).print( "   ", cout );
                                    #endif
                                }
                                constraintHasZeros = true;
                            }
                        }
                        if( !constraintHasZeros && relation == Constraint::EQ )
                            generatedTestCandidateBeingASolution = sideConditions.empty();
                        break;
                    }
                    //degree > 2 (> 3)
                    default:
                    {
                        _currentState->rTooHighDegreeConditions().insert( _condition );
                        break;
                    }
                }
            }
        #ifdef SMTRAT_VS_VARIABLEBOUNDS
        }
        #endif
        if( !generatedTestCandidateBeingASolution && !_currentState->isInconsistent() )
        {
            // Create state ( Conditions, [x -> -infinity]):
            Substitution sub = Substitution( _eliminationVar, Substitution::MINUS_INFINITY, oConditions );
            if( _currentState->addChild( sub ) )
            {
                // Add its valuation to the current ranking.
                addStateToRanking( (*_currentState).rChildren().back() );
                numberOfAddedChildren++;
                #ifdef VS_DEBUG
                (*(*_currentState).rChildren().back()).print( "   ", cout );
                #endif
            }
        }
        if( generatedTestCandidateBeingASolution )
        {
            _currentState->rTooHighDegreeConditions().clear();
            for( auto cond = _currentState->rConditions().begin(); cond != _currentState->conditions().end(); ++cond )
                (**cond).rFlag() = true;
            assert( numberOfAddedChildren <= _currentState->children().size() );
            while( _currentState->children().size() > numberOfAddedChildren )
            {
                State* toDelete = *_currentState->rChildren().begin();
                removeStatesFromRanking( *toDelete );
                _currentState->resetConflictSets();
                _currentState->rChildren().erase( _currentState->rChildren().begin() );
                delete toDelete;
            }
            if( numberOfAddedChildren == 0 )
            {
                ConditionSetSet conflictSet = ConditionSetSet();
                vs::Condition::Set condSet  = vs::Condition::Set();
                condSet.insert( _condition );
                conflictSet.insert( condSet );
                _currentState->addConflicts( NULL, conflictSet );
                _currentState->rInconsistent() = true;
            }
        }
        else
            (*_condition).rFlag() = true;
        addStateToRanking( _currentState );
    }

    /**
     * Applies the substitution of _currentState to the given conditions.
     *
     * @param _currentState     The currently considered state.
     * @param _conditions       The conditions to which the substitution in this state
     *                          shall be applied. Note that these conditions are always
     *                          a subset of the condition vector in the father of this
     *                          state.
     *
     * @sideeffect: The result is stored in the substitution result of the given state.
     */
    template<class Settings>
    bool VSModule<Settings>::substituteAll( State* _currentState, ConditionList& _conditions )
    {
        /*
         * Create a vector to store the results of each single substitution. Each entry corresponds to
         * the results of a single substitution. These results can be considered as a disjunction of
         * conjunctions of constraints.
         */
        vector<DisjunctionOfConditionConjunctions> allSubResults = vector<DisjunctionOfConditionConjunctions>();
        // The substitution to apply.
        assert( !_currentState->isRoot() );
        const Substitution& currentSubs = _currentState->substitution();
        // The variable to substitute.
        const carl::Variable& substitutionVariable = currentSubs.variable();
        // The conditions of the currently considered state, without the one getting just eliminated.
        ConditionList oldConditions = ConditionList();
        bool anySubstitutionFailed = false;
        bool allSubstitutionsApplied = true;
        ConditionSetSet conflictSet = ConditionSetSet();
        #ifdef SMTRAT_VS_VARIABLEBOUNDS
        if( _currentState->father().variableBounds().isConflicting() )
        {
            _currentState->father().printAlone();
            _currentState->printAlone();
        }
//        EvalDoubleIntervalMap solBox = (currentSubs.type() == Substitution::MINUS_INFINITY ? EvalDoubleIntervalMap() : _currentState->rFather().rVariableBounds().getIntervalMap());
        EvalDoubleIntervalMap solBox = _currentState->father().variableBounds().getIntervalMap();
        #else
        EvalDoubleIntervalMap solBox = EvalDoubleIntervalMap();
        #endif
        // Apply the substitution to the given conditions.
        for( auto cond = _conditions.begin(); cond != _conditions.end(); ++cond )
        {
            // The constraint to substitute in.
            const Constraint* currentConstraint = (**cond).pConstraint();
            // Does the condition contain the variable to substitute.
            auto var = currentConstraint->variables().find( substitutionVariable );
            if( var == currentConstraint->variables().end() )
            {
                if( !anySubstitutionFailed )
                {
                    oldConditions.push_back( new vs::Condition( currentConstraint, (**cond).valuation() ) );
                    oldConditions.back()->pOriginalConditions()->insert( *cond );
                }
            }
            else
            {
                DisjunctionOfConstraintConjunctions subResult = DisjunctionOfConstraintConjunctions();
                Variables conflVars = Variables();
                if( !substitute( currentConstraint, currentSubs, subResult, Settings::virtual_substitution_according_paper, conflVars, solBox ) )
                    allSubstitutionsApplied = false;
                // Create the the conditions according to the just created constraint prototypes.
                if( subResult.empty() )
                {
                    anySubstitutionFailed = true;
                    vs::Condition::Set condSet  = vs::Condition::Set();
                    condSet.insert( *cond );
                    if( _currentState->pOriginalCondition() != NULL )
                        condSet.insert( _currentState->pOriginalCondition() );
                    #ifdef SMTRAT_VS_VARIABLEBOUNDS
                    set< const vs::Condition* > conflictingBounds = _currentState->father().variableBounds().getOriginsOfBounds( conflVars );
                    condSet.insert( conflictingBounds.begin(), conflictingBounds.end() );
                    #endif
                    conflictSet.insert( condSet );
                }
                else
                {
                    if( allSubstitutionsApplied && !anySubstitutionFailed )
                    {
                        allSubResults.push_back( DisjunctionOfConditionConjunctions() );
                        DisjunctionOfConditionConjunctions& currentDisjunction = allSubResults.back();
                        for( auto consConj = subResult.begin(); consConj != subResult.end(); ++consConj )
                        {
                            currentDisjunction.push_back( ConditionList() );
                            ConditionList& currentConjunction = currentDisjunction.back();
                            for( auto cons = consConj->begin(); cons != consConj->end(); ++cons )
                            {
                                currentConjunction.push_back( new vs::Condition( *cons, _currentState->treeDepth() ) );
                                currentConjunction.back()->pOriginalConditions()->insert( *cond );
                            }
                        }
                    }
                }
            }
        }
        bool cleanResultsOfThisMethod = false;
        if( anySubstitutionFailed )
        {
            _currentState->rFather().addConflicts( _currentState->pSubstitution(), conflictSet );
            _currentState->rInconsistent() = true;
            _currentState->resetConflictSets();
            while( !_currentState->children().empty() )
            {
                State* toDelete = _currentState->rChildren().back();
                _currentState->rChildren().pop_back();
                delete toDelete;
            }
            while( !_currentState->conditions().empty() )
            {
                const vs::Condition* pCond = _currentState->rConditions().back();
                _currentState->rConditions().pop_back();
                #ifdef SMTRAT_VS_VARIABLEBOUNDS
                _currentState->rVariableBounds().removeBound( pCond->pConstraint(), pCond );
                #endif
                delete pCond;
                pCond = NULL;
            }
            cleanResultsOfThisMethod = true;
        }
        else
        {
            if( !_currentState->isInconsistent() )
            {
                if( allSubstitutionsApplied )
                {
                    allSubResults.push_back( DisjunctionOfConditionConjunctions() );
                    allSubResults.back().push_back( oldConditions );
                    _currentState->addSubstitutionResults( allSubResults );
                    addStateToRanking( _currentState );
                }
                else
                {
                    removeStatesFromRanking( _currentState->rFather() );
                    _currentState->resetConflictSets();
                    while( !_currentState->children().empty() )
                    {
                        State* toDelete = _currentState->rChildren().back();
                        _currentState->rChildren().pop_back();
                        delete toDelete;
                    }
                    while( !_currentState->conditions().empty() )
                    {
                        const vs::Condition* pCond = _currentState->rConditions().back();
                        _currentState->rConditions().pop_back();
                        #ifdef SMTRAT_VS_VARIABLEBOUNDS
                        _currentState->rVariableBounds().removeBound( pCond->pConstraint(), pCond );
                        #endif
                        delete pCond;
                        pCond = NULL;
                    }
                    cleanResultsOfThisMethod = true;
                    _currentState->rMarkedAsDeleted() = true;
                    _currentState->rFather().rToHighDegree() = true;
                    addStatesToRanking( _currentState->pFather() );
                    cleanResultsOfThisMethod = true;
                }
            }
            #ifdef VS_DEBUG
            _currentState->print( "   ", cout );
            #endif
        }
        if( cleanResultsOfThisMethod )
        {
            while( !oldConditions.empty() )
            {
                const vs::Condition* rpCond = oldConditions.back();
                oldConditions.pop_back();
                delete rpCond;
                rpCond = NULL;
            }
            while( !allSubResults.empty() )
            {
                while( !allSubResults.back().empty() )
                {
                    while( !allSubResults.back().back().empty() )
                    {
                        const vs::Condition* rpCond = allSubResults.back().back().back();
                        allSubResults.back().back().pop_back();
                        delete rpCond;
                        rpCond = NULL;
                    }
                    allSubResults.back().pop_back();
                }
                allSubResults.pop_back();
            }
        }
//        if( _currentState->hasSubstitutionResults() )
//        {
//            unsigned numOfCombs = 1;
//            for( auto iter = _currentState->substitutionResults().begin(); iter != _currentState->substitutionResults().end(); ++iter )
//                numOfCombs *= iter->size();
//            cout << numOfCombs << endl;
//        }
        return !anySubstitutionFailed;
    }

    /**
     * Applies the substitution of the given state to all conditions, which were recently added to it.
     *
     * @param _currentState The currently considered state.
     */
    template<class Settings>
    void VSModule<Settings>::propagateNewConditions( State* _currentState )
    {
        removeStatesFromRanking( *_currentState );
        _currentState->rHasRecentlyAddedConditions() = false;
        // Collect the recently added conditions and mark them as not recently added.
        bool deleteExistingTestCandidates = false;
        ConditionList recentlyAddedConditions = ConditionList();
        for( auto cond = _currentState->rConditions().begin(); cond != _currentState->conditions().end(); ++cond )
        {
            if( (**cond).recentlyAdded() )
            {
                (**cond).rRecentlyAdded() = false;
                recentlyAddedConditions.push_back( *cond );
                if( _currentState->pOriginalCondition() == NULL )
                {
                    bool onlyTestCandidateToConsider = false;
                    if( _currentState->index() != carl::Variable::NO_VARIABLE ) // TODO: Maybe only if the degree is not to high
                        onlyTestCandidateToConsider = (**cond).constraint().hasFinitelyManySolutionsIn( _currentState->index() );
                    if( onlyTestCandidateToConsider )
                        deleteExistingTestCandidates = true;
                }
            }
        }
        addStateToRanking( _currentState );
        if( !_currentState->children().empty() )
        {
            if( deleteExistingTestCandidates || _currentState->initIndex( mAllVariables, Settings::prefer_equation_over_all ) )
            {
                _currentState->initConditionFlags();
                // If the recently added conditions make another variable being the best to eliminate next delete all children.
                _currentState->resetConflictSets();
                while( !_currentState->children().empty() )
                {
                    State* toDelete = _currentState->rChildren().back();
                    _currentState->rChildren().pop_back();
                    delete toDelete;
                }
            }
            else
            {
                bool newTestCandidatesGenerated = false;
                if( _currentState->pOriginalCondition() == NULL )
                {
                    /*
                     * Check if there are new conditions in this state, which would have been better choices
                     * to generate test candidates than those conditions of the already generated test
                     * candidates. If so, generate the test candidates of the better conditions.
                     */
                    for( auto cond = recentlyAddedConditions.begin(); cond != recentlyAddedConditions.end(); ++cond )
                    {
                        if( _currentState->index() != carl::Variable::NO_VARIABLE && (**cond).constraint().hasVariable( _currentState->index() ) )
                        {
                            bool worseConditionFound = false;
                            auto child = _currentState->rChildren().begin();
                            while( !worseConditionFound && child != _currentState->children().end() )
                            {
                                if( (**child).substitution().type() != Substitution::MINUS_INFINITY )
                                {
                                    auto oCond = (**child).rSubstitution().rOriginalConditions().begin();
                                    while( !worseConditionFound && oCond != (**child).substitution().originalConditions().end() )
                                    {
                                        if( (**cond).valuate( _currentState->index(), mAllVariables.size(), true, Settings::prefer_equation_over_all ) > (**oCond).valuate( _currentState->index(), mAllVariables.size(), true, Settings::prefer_equation_over_all ) )
                                        {
                                            newTestCandidatesGenerated = true;
                                            #ifdef VS_DEBUG
                                            cout << "*** Eliminate " << _currentState->index() << " in ";
                                            cout << (**cond).constraint().toString( 0, true, true );
                                            cout << " creates:" << endl;
                                            #endif
                                            eliminate( _currentState, _currentState->index(), *cond );
                                            #ifdef VS_DEBUG
                                            cout << "*** Eliminate ready." << endl;
                                            #endif
                                            worseConditionFound = true;
                                            break;
                                        }
                                        ++oCond;
                                    }
                                }
                                ++child;
                            }
                        }
                    }
                }
                // Otherwise, add the recently added conditions to each child of the considered state to
                // which a substitution must be or already has been applied.
                for( auto child = _currentState->rChildren().begin(); child != _currentState->children().end(); ++child )
                {
                    if( (**child).type() != State::SUBSTITUTION_TO_APPLY || (**child).isInconsistent() )
                    {
                        // Apply substitution to new conditions and add the result to the substitution result vector.
                        substituteAll( *child, recentlyAddedConditions );
                        if( (**child).isInconsistent() &&!(**child).subResultsSimplified() )
                        {
                            (**child).simplify();
                            if( !(**child).conflictSets().empty() )
                                addStateToRanking( *child );
                        }
                    }
                    else
                    {
                        if( newTestCandidatesGenerated )
                        {
                            addStateToRanking( *child );
                            if( !(**child).children().empty() )
                                (**child).rHasChildrenToInsert() = true;
                        }
                        else
                            addStatesToRanking( *child );
                    }
                }
            }
        }
    }
    
    /**
     * Inserts a state in the ranking.
     *
     * @param _state The states, which will be inserted.
     */
    template<class Settings>
    void VSModule<Settings>::addStateToRanking( State* _state )
    {
        if( !_state->markedAsDeleted() && !(_state->isInconsistent() && _state->conflictSets().empty() && _state->conditionsSimplified()) )
        {
            if( _state->id() != 0 )
            {
                unsigned id = _state->id();
                removeStateFromRanking( *_state );
                _state->rID()= id;
            }
            else
            {
                increaseIDCounter();
                _state->rID() = mIDCounter;
            }
            _state->updateValuation( !Settings::integer_variables );
            UnsignedTriple key = UnsignedTriple( _state->valuation(), pair< unsigned, unsigned> ( _state->id(), _state->backendCallValuation() ) );
            if( (mRanking.insert( ValStatePair( key, _state ) )).second == false )
            {
                cout << "Warning: Could not insert. Entry already exists.";
                cout << endl;
            }
        }
    }

    /**
     * Inserts a state and all its successors in the ranking.
     *
     * @param _state The root of the states, which will be inserted.
     */
    template<class Settings>
    void VSModule<Settings>::addStatesToRanking( State* _state )
    {
        addStateToRanking( _state );
        if( _state->conditionsSimplified() && _state->subResultsSimplified() && !_state->takeSubResultCombAgain() && !_state->hasRecentlyAddedConditions() )
            for( auto dt = (*_state).rChildren().begin(); dt != (*_state).children().end(); ++dt )
                addStatesToRanking( *dt );
    }

    /**
     * Inserts all states with too high degree conditions being the given state or any of its successors in the ranking.
     * @param _state The root of the states, which will be inserted if they have too high degree conditions.
     */
    template<class Settings>
    void VSModule<Settings>::insertTooHighDegreeStatesInRanking( State* _state )
    {
        if( _state->tooHighDegree() )
            addStateToRanking( _state );
        else
            for( auto dt = (*_state).rChildren().begin(); dt != (*_state).children().end(); ++dt )
                insertTooHighDegreeStatesInRanking( *dt );
    }

    /**
     * Removes a state from the ranking.
     *
     * @param _state The states, which will be erased of the ranking.
     *
     * @return  True, if the state was in the ranking.
     */
    template<class Settings>
    bool VSModule<Settings>::removeStateFromRanking( State& _state )
    {
        UnsignedTriple key = UnsignedTriple( _state.valuation(), pair< unsigned, unsigned> ( _state.id(), _state.backendCallValuation() ) );
        auto valDTPair = mRanking.find( key );
        if( valDTPair != mRanking.end() )
        {
            mRanking.erase( valDTPair );
            _state.rID() = 0;
            return true;
        }
        else
            return false;
    }

    /**
     * Removes a state and its successors from the ranking.
     *
     * @param _state The root of the states, which will be erased of the ranking.
     */
    template<class Settings>
    void VSModule<Settings>::removeStatesFromRanking( State& _state )
    {
        removeStateFromRanking( _state );
        for( auto dt = _state.rChildren().begin(); dt != _state.children().end(); ++dt )
            removeStatesFromRanking( **dt );
    }

    /**
     * Updates the infeasible subset.
     */
    template<class Settings>
    void VSModule<Settings>::updateInfeasibleSubset( bool _includeInconsistentTestCandidates )
    {
        if( !Settings::infeasible_subset_generation )
        {
            // Set the infeasible subset to the set of all received constraints.
            mInfeasibleSubsets.push_back( set<const Formula*>() );
            for( auto cons = mpReceivedFormula->begin(); cons != mpReceivedFormula->end(); ++cons )
                mInfeasibleSubsets.back().insert( *cons );
            return;
        }
        // Determine the minimum covering sets of the conflict sets, i.e. the infeasible subsets of the root.
        ConditionSetSet minCoverSets = ConditionSetSet();
        ConditionSetSetSet confSets  = ConditionSetSetSet();
        State::ConflictSets::iterator nullConfSet = mpStateTree->rConflictSets().find( NULL );
        if( nullConfSet != mpStateTree->rConflictSets().end() && !_includeInconsistentTestCandidates )
            confSets.insert( nullConfSet->second.begin(), nullConfSet->second.end() );
        else
            for( auto confSet = mpStateTree->rConflictSets().begin(); confSet != mpStateTree->rConflictSets().end(); ++confSet )
                confSets.insert( confSet->second.begin(), confSet->second.end() );
        allMinimumCoveringSets( confSets, minCoverSets );
        assert( !minCoverSets.empty() );
        // Change the globally stored infeasible subset to the smaller one.
        mInfeasibleSubsets.clear();
        for( auto minCoverSet = minCoverSets.begin(); minCoverSet != minCoverSets.end(); ++minCoverSet )
        {
            assert( !minCoverSet->empty() );
            mInfeasibleSubsets.push_back( set<const Formula*>() );
            for( auto cond = minCoverSet->begin(); cond != minCoverSet->end(); ++cond )
            {
                for( auto oCond = (**cond).originalConditions().begin(); oCond != (**cond).originalConditions().end();
                        ++oCond )
                {
                    Formula::const_iterator receivedConstraint = mpReceivedFormula->begin();
                    while( receivedConstraint != mpReceivedFormula->end() )
                    {
                        bool constraintsAreEqual = (**oCond).constraint() == (*receivedConstraint)->constraint();
                        if( constraintsAreEqual )
                            break;
                        ++receivedConstraint;
                    }
                    if( receivedConstraint == mpReceivedFormula->end() )
                    {
                        cout << *mpReceivedFormula << endl;
                        cout << (**oCond).constraint() << endl;
                        printAll();
                    }
                    assert( receivedConstraint != mpReceivedFormula->end() );
                    mInfeasibleSubsets.back().insert( *receivedConstraint );
                }
            }
        }
        assert( !mInfeasibleSubsets.empty() );
        assert( !mInfeasibleSubsets.back().empty() );
    }
    
    template<class Settings>
    EvalRationalMap VSModule<Settings>::getIntervalAssignment( const State* _state ) const
    {
        EvalRationalMap varSolutions;
        // The variables have 0 as default assignment, which is going to be overwritten
        // if the variable has been eliminated by virtual substitution. Note, that it is
        // possible that variables are eliminated as a side effect of the elimination of 
        // a different variable, which means that we can choose the assignment of that 
        // variable arbitrarily.
        Variables vars;
        _state->father().variables( vars );
        vars.erase( _state->substitution().variable() );
        while( !vars.empty() )
        {
            varSolutions[*vars.begin()] = ZERO_RATIONAL;
            vars.erase( vars.begin() );
        }
        // Find all assignments of the variables occurring in the conditions
        // of the state's father except of the variable being the index currentState.
        const State* successorState = mRanking.begin()->second;
        while( successorState != _state )
        {
            assert( !successorState->isRoot() );
            assert( successorState->substitution().variable().getType() == carl::VariableType::VT_INT );
            assert( successorState->substitution().type() == Substitution::NORMAL );
            Rational subTermEvaluated;
            successorState->substitution().term().evaluate( subTermEvaluated, varSolutions );
            varSolutions[successorState->substitution().variable()] = subTermEvaluated;
            successorState = successorState->pFather();
        }
        return varSolutions;
    }
    
    template<class Settings>
    Answer VSModule<Settings>::solutionInDomain()
    {
        assert( solverState() != False );
        if( !mRanking.empty() )
        {
            const State* currentState = mRanking.begin()->second;
            while( !currentState->isRoot() )
            {
                if( currentState->substitution().variable().getType() == carl::VariableType::VT_INT )
                {
                    if( currentState->substitution().type() == Substitution::MINUS_INFINITY )
                    {
                        // We establish a set of univariate polynomials being the left-hand sides of
                        // currentState's father's conditions with all their variables (except of 
                        // currentState's index) substituted by the found (integer!) assignments. Then 
                        // we know that the weakest lower Cauchy bound of these univariate polynomials
                        // under-approximates all of their roots.
                        EvalRationalMap varSolutions = getIntervalAssignment( currentState );
                        Rational weakestCauchyBound;
                        for( auto cond = currentState->father().conditions().begin(); cond != currentState->father().conditions().end(); ++cond )
                        {
                            Rational condsLowerCB;
                            if( varSolutions.empty() )
                            {
                                condsLowerCB = (*cond)->constraint().lhs().toUnivariatePolynomial().cauchyBound();
                                if( condsLowerCB > weakestCauchyBound )
                                    weakestCauchyBound = condsLowerCB;
                            }
                            else
                            {
                                Polynomial substituted = (*cond)->constraint().lhs().substitute( varSolutions );
                                if( !substituted.isConstant() )
                                {
                                    condsLowerCB = substituted.toUnivariatePolynomial().cauchyBound();
                                    if( condsLowerCB > weakestCauchyBound )
                                        weakestCauchyBound = condsLowerCB;
                                }
                            }
                        }
                        if( Settings::use_variable_bounds && !Settings::assure_termination )
                        {
                            Interval varInterval = currentState->father().variableBounds().getInterval( currentState->substitution().variable() );
                            if( varInterval.rightType() != carl::BoundType::INFTY && varInterval.right() <= weakestCauchyBound )
                            {
                                weakestCauchyBound = varInterval.right() - 1; 
                            }
                        }
                        // We split at the next greater integer I than the calculated weakest lower Cauchy bound.
                        // By this we force in one case (A) that currentState's test candidate (-infinity) gets
                        // invalid and a new (integer) test candidate at I is generated. In the other case (B) the 
                        // assignment of the other variables in currentState's father cannot hold and must be adapted. 
                        // Note that in the case that there are no other variables in currentState's father, only (A)
                        // can be applied.
                        #ifdef MODULE_VERBOSE_INTEGERS
                        this->printAnswer();
                        #endif
                        branchAt( currentState->substitution().variable(), weakestCauchyBound );
                        return foundAnswer( Unknown );
                    }
                    else
                    {
                        // Insert the (integer!) assignments of the other variables.
                        EvalRationalMap varSolutions = getIntervalAssignment( currentState );
                        const SqrtEx& subTerm = currentState->substitution().term();
                        Rational evaluatedSubTerm;
                        bool assIsInteger = subTerm.evaluate( evaluatedSubTerm, varSolutions, -1 );
                        assIsInteger &= carl::isInteger( evaluatedSubTerm );
                        if( currentState->substitution().type() == Substitution::PLUS_EPSILON || !assIsInteger )
                        {
                            #ifdef MODULE_VERBOSE_INTEGERS
                            this->printAnswer();
                            #endif
                            branchAt( currentState->substitution().variable(), evaluatedSubTerm );
                            return foundAnswer( Unknown );
                        }
                    }
                }
                currentState = currentState->pFather();
            }
        }
        return foundAnswer( True );
    }

    /**
     * Finds all minimum covering sets of a vector of sets of sets. A minimum covering set
     * fulfills the following properties:
     *
     *          1.) It covers in each set of sets at least one of its sets.
     *          2.) If you delete any element of the minimum covering set, the
     *              first property does not hold anymore.
     *
     * @param _conflictSets     The vector of sets of sets, for which the method finds all minimum covering sets.
     * @param _minCovSets   The resulting minimum covering sets.
     */
    template<class Settings>
    void VSModule<Settings>::allMinimumCoveringSets( const ConditionSetSetSet& _conflictSets, ConditionSetSet& _minCovSets )
    {
        if( !_conflictSets.empty() )
        {
            // First we construct all possible combinations of combining all single sets of each set of sets.
            // Store for each set an iterator.
            vector<ConditionSetSet::iterator> conditionSetSetIters = vector<ConditionSetSet::iterator>();
            for( auto conflictSet = _conflictSets.begin(); conflictSet != _conflictSets.end(); ++conflictSet )
            {
                conditionSetSetIters.push_back( (*conflictSet).begin() );
                // Assure, that the set is not empty.
                assert( conditionSetSetIters.back() != (*conflictSet).end() );
            }
            ConditionSetSetSet::iterator conflictSet;
            vector<ConditionSetSet::iterator>::iterator conditionSet;
            // Find all covering sets by forming the union of all combinations.
            bool lastCombinationReached = false;
            while( !lastCombinationReached )
            {
                // Create a new combination of vectors.
                vs::Condition::Set coveringSet = vs::Condition::Set();
                bool previousIteratorIncreased = false;
                // For each set of sets in the vector of sets of sets, choose a set in it. We combine
                // these sets by forming their union and store it as a covering set.
                conditionSet = conditionSetSetIters.begin();
                conflictSet  = _conflictSets.begin();
                while( conditionSet != conditionSetSetIters.end() )
                {
                    if( (*conflictSet).empty() )
                    {
                        conflictSet++;
                        conditionSet++;
                    }
                    else
                    {
                        coveringSet.insert( (**conditionSet).begin(), (**conditionSet).end() );
                        // Set the iterator.
                        if( !previousIteratorIncreased )
                        {
                            (*conditionSet)++;
                            if( *conditionSet != (*conflictSet).end() )
                                previousIteratorIncreased = true;
                            else
                                *conditionSet = (*conflictSet).begin();
                        }
                        conflictSet++;
                        conditionSet++;
                        if( !previousIteratorIncreased && conditionSet == conditionSetSetIters.end() )
                            lastCombinationReached = true;
                    }
                }
                _minCovSets.insert( coveringSet );
            }
            /*
             * Delete all covering sets, which are not minimal. We benefit of the set of sets property,
             * which sorts its sets according to the elements they contain. If a set M is a subset of
             * another set M', the position of M in the set of sets is before M'.
             */
            auto minCoverSet = _minCovSets.begin();
            auto coverSet    = _minCovSets.begin();
            coverSet++;
            while( coverSet != _minCovSets.end() )
            {
                auto cond1 = (*minCoverSet).begin();
                auto cond2 = (*coverSet).begin();
                while( cond1 != (*minCoverSet).end() && cond2 != (*coverSet).end() )
                {
                    if( *cond1 != *cond2 )
                        break;
                    cond1++;
                    cond2++;
                }
                if( cond1 == (*minCoverSet).end() )
                    _minCovSets.erase( coverSet++ );
                else
                {
                    minCoverSet = coverSet;
                    coverSet++;
                }
            }
        }
    }

    /**
     * Adapts the passed formula according to the conditions of the currently considered state.
     *
     * @return  true,   if the passed formula has been changed;
     *          false,  otherwise.
     */
    template<class Settings>
    bool VSModule<Settings>::adaptPassedFormula( const State& _state, FormulaConditionMap& _formulaCondMap, bool _strictInequalitiesOnly )
    {
        bool changedPassedFormula = false;
        // Collect the constraints to check.
        PointerMap<Constraint,const vs::Condition*> constraintsToCheck = PointerMap<Constraint,const vs::Condition*>();
        for( auto cond = _state.conditions().begin(); cond != _state.conditions().end(); ++cond )
        {
            if( (*cond)->flag() )
            {
                const Constraint* constraint = (*cond)->pConstraint();
                switch( constraint->relation() )
                {
                    case Constraint::GEQ:
                    {
                        const Constraint* strictVersion = Formula::newConstraint( constraint->lhs(), Constraint::GREATER );
                        constraintsToCheck.insert( pair< const Constraint*, const vs::Condition*>( strictVersion, *cond ) );
                        break;
                    }
                    case Constraint::LEQ:
                    {
                        const Constraint* strictVersion = Formula::newConstraint( constraint->lhs(), Constraint::LESS );
                        constraintsToCheck.insert( pair< const Constraint*, const vs::Condition*>( strictVersion, *cond ) );
                        break;
                    }
                    default:
                    {
                        constraintsToCheck.insert( pair< const Constraint*, const vs::Condition*>( constraint, *cond ) );
                    }
                }
            }
            else
                constraintsToCheck.insert( pair< const Constraint*, const vs::Condition*>( (*cond)->pConstraint(), *cond ) );
        }
        if( constraintsToCheck.empty() ) return false;
        /*
         * Remove the constraints from the constraints to check, which are already in the passed formula
         * and remove the sub formulas (constraints) in the passed formula, which do not occur in the
         * constraints to add.
         */
        Formula::iterator subformula = mpPassedFormula->begin();
        while( subformula != mpPassedFormula->end() )
        {
            auto iter = constraintsToCheck.find( (*subformula)->pConstraint() );
            if( iter != constraintsToCheck.end() )
            {
                _formulaCondMap[*subformula] = iter->second;
                constraintsToCheck.erase( iter );
                ++subformula;
            }
            else
            {
                subformula           = removeSubformulaFromPassedFormula( subformula );
                changedPassedFormula = true;
            }
        }
        // Add the the remaining constraints to add to the passed formula.
        for( auto iter = constraintsToCheck.begin(); iter != constraintsToCheck.end(); ++iter )
        {
            changedPassedFormula           = true;
            vec_set_const_pFormula origins = vec_set_const_pFormula();
            Formula* formula = new smtrat::Formula( iter->first );
            _formulaCondMap[formula] = iter->second;
            addConstraintToInform( iter->first );
            addSubformulaToPassedFormula( formula, origins );
        }
        return changedPassedFormula;
    }

    /**
     * Run the backend solvers on the conditions of the given state.
     *
     * @param _state    The state to check the conditions of.
     *
     * @return  TS_True,    if the conditions are consistent and there is no unfinished ancestor;
     *          TS_False,   if the conditions are inconsistent;
     *          TS_Unknown, if the theory solver cannot give an answer for these conditons.
     */
    template<class Settings>
    Answer VSModule<Settings>::runBackendSolvers( State* _state, bool _strictInequalitiesOnly )
    {
        // Run the backends on the constraint of the state.
        FormulaConditionMap formulaToConditions = FormulaConditionMap();
        adaptPassedFormula( *_state, formulaToConditions );
        Answer result = runBackends();
        #ifdef VS_DEBUG
        cout << "Ask backend      : ";
        cout << mpPassedFormula->toString( false, 0, "", true, true, true );
        cout << endl;
        cout << "Answer           : " << ( result == True ? "True" : ( result == False ? "False" : "Unknown" ) ) << endl;
        #endif
        switch( result )
        {
            case True:
            {
                return True;
            }
            case False:
            {
                /*
                * Get the conflict sets formed by the infeasible subsets in the backend.
                */
                ConditionSetSet conflictSet = ConditionSetSet();
                vector<Module*>::const_iterator backend = usedBackends().begin();
                while( backend != usedBackends().end() )
                {
                    if( !(*backend)->infeasibleSubsets().empty() )
                    {
                        for( auto infsubset = (*backend)->infeasibleSubsets().begin(); infsubset != (*backend)->infeasibleSubsets().end(); ++infsubset )
                        {
                            vs::Condition::Set conflict = vs::Condition::Set();
                            #ifdef VS_DEBUG
                            cout << "Infeasible Subset: {";
                            #endif
                            for( auto subformula = infsubset->begin(); subformula != infsubset->end(); ++subformula )
                            {
                                #ifdef VS_DEBUG
                                cout << "  " << (*subformula)->constraint();
                                #endif
                                auto fcPair = formulaToConditions.find( *subformula );
                                assert( fcPair != formulaToConditions.end() );
                                conflict.insert( fcPair->second );
                            }
                            #ifdef VS_DEBUG
                            cout << "  }" << endl;
                            #endif
                            #ifdef SMTRAT_DEVOPTION_Validation
                            if( validationSettings->logTCalls() )
                            {
                                set<const smtrat::Constraint*> constraints = set<const smtrat::Constraint*>();
                                for( auto cond = conflict.begin(); cond != conflict.end(); ++cond )
                                    constraints.insert( (**cond).pConstraint() );
                                smtrat::Module::addAssumptionToCheck( constraints, false, moduleName( (*backend)->type() ) + "_infeasible_subset" );
                            }
                            #endif
                            assert( conflict.size() == infsubset->size() );
                            assert( !conflict.empty() );
                            conflictSet.insert( conflict );
                        }
                        break;
                    }
                }
                assert( !conflictSet.empty() );
                _state->addConflictSet( NULL, conflictSet );
                removeStatesFromRanking( *_state );

                #ifdef VS_LOG_INTERMEDIATE_STEPS
                logConditions( *_state, false, "Intermediate_conflict_of_VSModule" );
                #endif
                // If the considered state is the root, update the found infeasible subset as infeasible subset.
                if( _state->isRoot() )
                    updateInfeasibleSubset();
                // If the considered state is not the root, pass the infeasible subset to the father.
                else
                {
                    removeStatesFromRanking( *_state );
                    _state->passConflictToFather( Settings::check_conflict_for_side_conditions );
                    removeStateFromRanking( _state->rFather() );
                    addStateToRanking( _state->pFather() );
                }
                return False;
            }
            case Unknown:
            {
                return Unknown;
            }
            default:
            {
                cerr << "Unknown answer type!" << endl;
                assert( false );
                return Unknown;
            }
        }
    }

    #ifdef VS_LOG_INTERMEDIATE_STEPS
    /**
     * Checks the correctness of the symbolic assignment given by the path from the root
     * state to the satisfying state.
     */
    template<class Settings>
    void VSModule<Settings>::checkAnswer() const
    {
        if( !mRanking.empty() )
        {
            const State* currentState = mRanking.begin()->second;
            while( !(*currentState).isRoot() )
            {
                logConditions( *currentState, true, "Intermediate_result_of_VSModule" );
                currentState = currentState->pFather();
            }
        }
    }

    /**
     * Checks whether the set of conditions is is consistent/inconsistent.
     */
    template<class Settings>
    void VSModule<Settings>::logConditions( const State& _state, bool _assumption, const string& _description ) const
    {
        if( !_state.conditions().empty() )
        {
            set<const smtrat::Constraint*> constraints = set<const smtrat::Constraint*>();
            for( auto cond = _state.conditions().begin(); cond != _state.conditions().end(); ++cond )
                constraints.insert( (**cond).pConstraint() );
            smtrat::Module::addAssumptionToCheck( constraints, _assumption, _description );
        }
    }
    #endif

    /**
     * Prints the history to the output stream.
     *
     * @param _init The beginning of each row.
     * @param _out The output stream where the history should be printed.
     */
    template<class Settings>
    void VSModule<Settings>::printAll( const string& _init, ostream& _out ) const
    {
        _out << _init << " Current solver status, where the constraints" << endl;
        printFormulaConditionMap( _init, _out );
        _out << _init << " have been added:" << endl;
        _out << _init << " mInconsistentConstraintAdded: " << mInconsistentConstraintAdded << endl;
        _out << _init << " mIDCounter: " << mIDCounter << endl;
        _out << _init << " Current ranking:" << endl;
        printRanking( _init, cout );
        _out << _init << " State tree:" << endl;
        mpStateTree->print( _init + "   ", _out );
    }

    /**
     * Prints the history to the output stream.
     *
     * @param _init The beginning of each row.
     * @param _out The output stream where the history should be printed.
     */
    template<class Settings>
    void VSModule<Settings>::printFormulaConditionMap( const string& _init, ostream& _out ) const
    {
        for( auto cond = mFormulaConditionMap.begin(); cond != mFormulaConditionMap.end(); ++cond )
        {
            _out << _init << "    ";
            _out << cond->first->toString( false, 0, "", true, true, true );
            _out << " <-> ";
            cond->second->print( _out );
            _out << endl;
        }
    }

    /**
     * Prints the history to the output stream.
     *
     * @param _init The beginning of each row.
     * @param _out The output stream where the history should be printed.
     */
    template<class Settings>
    void VSModule<Settings>::printRanking( const string& _init, ostream& _out ) const
    {
        for( auto valDTPair = mRanking.begin(); valDTPair != mRanking.end(); ++valDTPair )
            (*(*valDTPair).second).printAlone( "   ", _out );
    }

    /**
     * Prints the answer if existent.
     *
     * @param _init The beginning of each row.
     * @param _out The output stream where the answer should be printed.
     */
    template<class Settings>
    void VSModule<Settings>::printAnswer( const string& _init, ostream& _out ) const
    {
        _out << _init << " Answer:" << endl;
        if( mRanking.empty() )
            _out << _init << "        False." << endl;
        else
        {
            _out << _init << "        True:" << endl;
            const State* currentState = mRanking.begin()->second;
            while( !(*currentState).isRoot() )
            {
                _out << _init << "           " << (*currentState).substitution().toString( true ) << endl;
                currentState = (*currentState).pFather();
            }
        }
        _out << endl;
    }
}    // end namespace smtrat