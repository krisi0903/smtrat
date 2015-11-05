/**
 * @file LRAModule.tpp
 * @author Florian Corzilius <corzilius@cs.rwth-aachen.de>
 * @since 2012-04-05
 * @version 2014-10-01
 */

#include "LRAModule.h"

#ifdef DEBUG_METHODS_TABLEAU
//#define DEBUG_METHODS_LRA_MODULE
#endif
//#define DEBUG_LRA_MODULE

using namespace smtrat::lra;

namespace smtrat
{
    template<class Settings>
    LRAModule<Settings>::LRAModule( const ModuleInput* _formula, RuntimeSettings*, Conditionals& _conditionals, Manager* _manager ):
        Module( _formula, _conditionals, _manager ),
        mInitialized( false ),
        mAssignmentFullfilsNonlinearConstraints( false ),
        mStrongestBoundsRemoved( false ),
        mTableau( passedFormulaEnd() ),
        mLinearConstraints(),
        mNonlinearConstraints(),
        mActiveResolvedNEQConstraints(),
        mActiveUnresolvedNEQConstraints(),
        mDelta( carl::freshRealVariable( "delta_" + to_string( id() ) ) ),
        mBoundCandidatesToPass(),
        mBranch_Success()    
    {
        #ifdef SMTRAT_DEVOPTION_Statistics
        stringstream s;
        s << moduleName() << "_" << id();
        mpStatistics = new LRAModuleStatistics( s.str() );
        #endif
    }

    template<class Settings>
    LRAModule<Settings>::~LRAModule()
    {
        #ifdef SMTRAT_DEVOPTION_Statistics
        delete mpStatistics;
        #endif
    }

    template<class Settings>
    bool LRAModule<Settings>::informCore( const FormulaT& _constraint )
    {
        #ifdef DEBUG_LRA_MODULE
        cout << "LRAModule::inform  " << "inform about " << _constraint << endl;
        #endif
        if( _constraint.getType() == carl::FormulaType::CONSTRAINT )
        {
            const ConstraintT& constraint = _constraint.constraint();
            if( !constraint.lhs().isConstant() && constraint.lhs().isLinear() )
            {
                mLinearConstraints.push_back( _constraint );
                setBound( _constraint );
            }
            return constraint.isConsistent() != 0;
        }
        return true;
    }

    template<class Settings>
    bool LRAModule<Settings>::addCore( ModuleInput::const_iterator _subformula )
    {
        #ifdef DEBUG_LRA_MODULE
        cout << "LRAModule::add " << _subformula->formula() << endl;
        #endif
        switch( _subformula->formula().getType() )
        {
            case carl::FormulaType::FALSE:
            {
                FormulaSetT infSubSet;
                infSubSet.insert( _subformula->formula() );
                mInfeasibleSubsets.push_back( infSubSet );
                #ifdef SMTRAT_DEVOPTION_Statistics
                mpStatistics->addConflict( mInfeasibleSubsets );
                #endif
                return false;
            }
            case carl::FormulaType::TRUE:
            {
                return true;
            }
            case carl::FormulaType::CONSTRAINT:
            {
                const FormulaT& formula = _subformula->formula();
                const ConstraintT& constraint  = formula.constraint();
                #ifdef SMTRAT_DEVOPTION_Statistics
                mpStatistics->add( constraint );
                #endif
                unsigned consistency = constraint.isConsistent();
                if( consistency == 2 )
                {
                    mAssignmentFullfilsNonlinearConstraints = false;
                    if( constraint.lhs().isLinear() )
                    {
//                        bool elementInserted = mLinearConstraints.insert( formula ).second;
//                        if( elementInserted && mInitialized )
//                        {
//                            mTableau.newBound( formula );
//                        }
                        if( constraint.relation() != carl::Relation::NEQ )
                        {
                            auto constrBoundIter = mTableau.constraintToBound().find( formula );
                            assert( constrBoundIter != mTableau.constraintToBound().end() );
                            const std::vector< const LRABound* >* bounds = constrBoundIter->second;
                            assert( bounds != NULL );
                            activateBound( *bounds->begin(), formula );

                            if( !(*bounds->begin())->neqRepresentation().isTrue() )
                            {
                                auto pos = mActiveUnresolvedNEQConstraints.find( (*bounds->begin())->neqRepresentation() );
                                if( pos != mActiveUnresolvedNEQConstraints.end() )
                                {
                                    auto entry = mActiveResolvedNEQConstraints.insert( *pos );
                                    removeOrigin( pos->second.position, pos->second.origin );
                                    entry.first->second.position = passedFormulaEnd();
                                    mActiveUnresolvedNEQConstraints.erase( pos );
                                    auto constrBoundIter = mTableau.constraintToBound().find( (*bounds->begin())->neqRepresentation() );
                                    assert( constrBoundIter != mTableau.constraintToBound().end() );
                                    const std::vector< const LRABound* >* boundsOfNeqConstraint = constrBoundIter->second;
                                    if( *bounds->begin() == (*boundsOfNeqConstraint)[1] || *bounds->begin() == (*boundsOfNeqConstraint)[2] )
                                    {
                                        bool leqBoundActive = *bounds->begin() == (*boundsOfNeqConstraint)[1];
                                        activateStrictBound( (*bounds->begin())->neqRepresentation(), **bounds->begin(), (*boundsOfNeqConstraint)[leqBoundActive ? 0 : 3] );
                                    }
                                }
                            }
                            return mInfeasibleSubsets.empty();
                        }
                        else
                        {
                            auto constrBoundIter = mTableau.constraintToBound().find( formula );
                            assert( constrBoundIter != mTableau.constraintToBound().end() );
                            const std::vector< const LRABound* >* bounds = constrBoundIter->second;
//                            bool intValued = constraint.integerValued();
//                            if( (intValued && ((*bounds)[1]->isActive() || (*bounds)[2]->isActive()))
//                                || (!intValued && ((*bounds)[0]->isActive() || (*bounds)[1]->isActive() || (*bounds)[2]->isActive() || (*bounds)[3]->isActive())) )
                            if( (*bounds)[0]->isActive() || (*bounds)[1]->isActive() || (*bounds)[2]->isActive() || (*bounds)[3]->isActive() )
                            {
                                Context context( formula, passedFormulaEnd() );
                                mActiveResolvedNEQConstraints.insert( std::pair< FormulaT, Context >( formula, std::move(context) ) );
                                bool leqBoundActive = (*bounds)[1]->isActive();
                                if( leqBoundActive || (*bounds)[2]->isActive() )
                                {
                                    activateStrictBound( formula, *(*bounds)[leqBoundActive ? 1 : 2], (*bounds)[leqBoundActive ? 0 : 3] );
                                }
                            }
                            else
                            {
                                Context context( formula, addSubformulaToPassedFormula( formula, formula ).first );
                                mActiveUnresolvedNEQConstraints.insert( std::pair< FormulaT, Context >( formula, std::move(context) ) );
                            }
                        }
                    }
                    else
                    {
                        addSubformulaToPassedFormula( formula, formula );
                        mNonlinearConstraints.push_back( formula );
                        return true;
                    }
                }
            }
            default:
                return true;
        }
        return true;
    }

    template<class Settings>
    void LRAModule<Settings>::removeCore( ModuleInput::const_iterator _subformula )
    {
        #ifdef DEBUG_LRA_MODULE
        cout << "LRAModule::remove " << _subformula->formula() << endl;
        #endif
        const FormulaT& formula = _subformula->formula();
        if( formula.getType() == carl::FormulaType::CONSTRAINT )
        {
            // Remove the mapping of the constraint to the sub-formula in the received formula
            const ConstraintT& constraint = formula.constraint();
            const FormulaT& pformula = _subformula->formula();
            #ifdef SMTRAT_DEVOPTION_Statistics
            mpStatistics->remove( constraint );
            #endif
            if( constraint.isConsistent() == 2 )
            {
                if( constraint.lhs().isLinear() )
                {
                    // Deactivate the bounds regarding the given constraint
                    auto constrBoundIter = mTableau.constraintToBound().find( pformula );
                    assert( constrBoundIter != mTableau.rConstraintToBound().end() );
                    std::vector< const LRABound* >* bounds = constrBoundIter->second;
                    assert( bounds != NULL );
                    auto bound = bounds->begin();
                    int pos = 0;
                    int dontRemoveBeforePos = constraint.relation() == carl::Relation::NEQ ? 4 : 1;
                    while( bound != bounds->end() )
                    {
                        if( !(*bound)->origins().empty() )
                        {
                            auto origin = (*bound)->pOrigins()->begin();
                            bool mainOriginRemains = true;
                            while( origin != (*bound)->origins().end() )
                            {
                                if( origin->getType() == carl::FormulaType::AND && origin->contains( pformula ) )
                                {
                                    origin = (*bound)->pOrigins()->erase( origin );
                                }
                                else if( mainOriginRemains && *origin == pformula )
                                {
                                    assert( origin->getType() == carl::FormulaType::CONSTRAINT );
                                    origin = (*bound)->pOrigins()->erase( origin );
                                    // ensures that only one main origin is removed, in the case that a formula is contained more than once in the module input
                                    mainOriginRemains = false; 
                                }
                                else
                                {
                                    ++origin;
                                }
                            }
                            if( (*bound)->origins().empty() )
                            {
                                if( !(*bound)->neqRepresentation().isTrue() )
                                {
                                    auto constrBoundIterB = mTableau.constraintToBound().find( (*bound)->neqRepresentation() );
                                    assert( constrBoundIterB != mTableau.constraintToBound().end() );
                                    const std::vector< const LRABound* >* uebounds = constrBoundIterB->second;
                                    assert( uebounds != NULL );
                                    assert( uebounds->size() >= 4 );
//                                    bool intValued = (*bound)->neqRepresentation().constraint().integerValued();
//                                    if( (intValued && !(*uebounds)[1]->isActive() && !(*uebounds)[2]->isActive()) ||
//                                        (!intValued && !(*uebounds)[0]->isActive() && !(*uebounds)[1]->isActive() && !(*uebounds)[2]->isActive() && !(*uebounds)[3]->isActive()) )
                                    if( !(*uebounds)[0]->isActive() && !(*uebounds)[1]->isActive() && !(*uebounds)[2]->isActive() && !(*uebounds)[3]->isActive() )
                                    {
                                        auto pos = mActiveResolvedNEQConstraints.find( (*bound)->neqRepresentation() );
                                        if( pos != mActiveResolvedNEQConstraints.end() )
                                        {
                                            auto entry = mActiveUnresolvedNEQConstraints.insert( *pos );
                                            mActiveResolvedNEQConstraints.erase( pos );
                                            entry.first->second.position = addSubformulaToPassedFormula( entry.first->first, entry.first->second.origin ).first;
                                        }
                                    }
                                }
                                LRAVariable& var = *(*bound)->pVariable();
                                if( Settings::restore_previous_consistent_assignment )
                                {
                                    if( var.deactivateBound( *bound, passedFormulaEnd() ) )
                                    {
                                        mStrongestBoundsRemoved = true;
                                    }
                                    if( var.isConflicting() )
                                    {
                                        FormulaSetT infsubset;
                                        collectOrigins( *var.supremum().origins().begin(), infsubset );
                                        collectOrigins( var.infimum().pOrigins()->back(), infsubset );
                                        mInfeasibleSubsets.push_back( std::move(infsubset) );
                                    }
                                }
                                else
                                {
                                    if( var.deactivateBound( *bound, passedFormulaEnd() ) && !var.isBasic() )
                                    {
                                        if( var.supremum() < var.assignment() )
                                        {
                                            mTableau.updateBasicAssignments( var.position(), LRAValue( var.supremum().limit() - var.assignment() ) );
                                            var.rAssignment() = var.supremum().limit();
                                        }
                                        else if( var.infimum() > var.assignment() )
                                        {
                                            mTableau.updateBasicAssignments( var.position(), LRAValue( var.infimum().limit() - var.assignment() ) );
                                            var.rAssignment() = var.infimum().limit();
                                        }
                                        if( var.isConflicting() )
                                        {
                                            FormulaSetT infsubset;
                                            collectOrigins( *var.supremum().origins().begin(), infsubset );
                                            collectOrigins( var.infimum().pOrigins()->back(), infsubset );
                                            mInfeasibleSubsets.push_back( std::move(infsubset) );
                                        }
                                    }
                                }
                                if( !(*bound)->pVariable()->pSupremum()->isInfinite() )
                                {
                                    mBoundCandidatesToPass.push_back( (*bound)->pVariable()->pSupremum() );
                                }
                                if( !(*bound)->pVariable()->pInfimum()->isInfinite() )
                                {
                                    mBoundCandidatesToPass.push_back( (*bound)->pVariable()->pInfimum() );
                                }

                                if( !(*bound)->variable().isActive() && (*bound)->variable().isBasic() && !(*bound)->variable().isOriginal() )
                                {
                                    mTableau.deactivateBasicVar( (*bound)->pVariable() );
                                }
                            }
                        }
                        if( (*bound)->origins().empty() && pos >= dontRemoveBeforePos )
                            bound = bounds->erase( bound );
                        else
                        {
                            ++bound;
                            ++pos;
                        }
                    }
                    if( constraint.relation() == carl::Relation::NEQ )
                    {
                        if( mActiveResolvedNEQConstraints.erase( pformula ) == 0 )
                        {
                            auto iter = mActiveUnresolvedNEQConstraints.find( pformula );
                            if( iter != mActiveUnresolvedNEQConstraints.end() )
                            {
                                removeOrigin( iter->second.position, iter->second.origin );
                                mActiveUnresolvedNEQConstraints.erase( iter );
                            }
                        }
                    }
                }
                else
                {
                    auto nonLinearConstraint = std::find(mNonlinearConstraints.begin(), mNonlinearConstraints.end(), pformula);
                    assert( nonLinearConstraint != mNonlinearConstraints.end() );
                    mNonlinearConstraints.erase( nonLinearConstraint );
                }
            }
        }
    }

    template<class Settings>
    Answer LRAModule<Settings>::checkCore( bool _full )
    {
        #ifdef DEBUG_LRA_MODULE
        cout << "LRAModule::check" << endl;
        printReceivedFormula();
        #endif
        bool backendsResultUnknown = true;
        bool containsIntegerValuedVariables = true;
        Answer result = Unknown;
        if( !rReceivedFormula().isConstraintConjunction() )
        {
            goto Return; // Unknown
        }
        if( !mInfeasibleSubsets.empty() )
        {
            result = False;
            goto Return;
        }
        if( rReceivedFormula().isRealConstraintConjunction() )
            containsIntegerValuedVariables = false;
        assert( !mTableau.isConflicting() );
        #ifdef LRA_USE_PIVOTING_STRATEGY
        mTableau.setBlandsRuleStart( 1000 );//(unsigned) mTableau.columns().size() );
        #endif
        mTableau.compressRows();
        for( ; ; )
        {
            // Check whether a module which has been called on the same instance in parallel, has found an answer.
            if( anAnswerFound() )
            {
                result = Unknown;
                goto Return;
            }
            // Find a pivoting element in the tableau.
            struct std::pair<EntryID,bool> pivotingElement = mTableau.nextPivotingElement();
//            cout << "\r" << mTableau.numberOfPivotingSteps() << endl;
            #ifdef DEBUG_LRA_MODULE
            if( pivotingElement.second && pivotingElement.first != LAST_ENTRY_ID )
            {
                cout << endl;
                mTableau.print( pivotingElement.first, cout, "    " );
                cout << endl;
            }
            #endif
            // If there is no conflict.
            if( pivotingElement.second )
            {
                // If no basic variable violates its bounds (and hence no variable at all).
                if( pivotingElement.first == 0 )
                {
                    #ifdef DEBUG_LRA_MODULE
                    mTableau.print();
                    mTableau.printVariables();
                    cout << "True" << endl;
                    #endif
                    // If the current assignment also fulfills the nonlinear constraints.
                    if( checkAssignmentForNonlinearConstraint() )
                    {
                        if( containsIntegerValuedVariables )
                        {
                            if( Settings::use_gomory_cuts && gomory_cut() )
                            {
                                goto Return; // Unknown
                            }
                            if( !Settings::use_gomory_cuts && Settings::use_cuts_from_proofs && cuts_from_proofs() )
                            {
                                goto Return; // Unknown
                            }
                            if( !Settings::use_gomory_cuts && !Settings::use_cuts_from_proofs && branch_and_bound() )
                            {
                                goto Return; // Unknown
                            }
                        }
                        result = True;
                        if( Settings::restore_previous_consistent_assignment )
                        {
                            mTableau.storeAssignment();
                        }
                        goto Return;
                    }
                    // Otherwise, check the consistency of the formula consisting of the nonlinear constraints and the tightest bounds with the backends.
                    else
                    {
                        adaptPassedFormula();
                        Answer a = runBackends( _full );
                        if( a == False )
                            getInfeasibleSubsets();
                        result = a;
                        if( a != Unknown )
                            backendsResultUnknown = false;
                        goto Return;
                    }
                }
                else
                {
                    // Pivot at the found pivoting entry.
                    if( Settings::branch_and_bound_early )
                    {
                        LRAVariable* newBasicVar = mTableau.pivot( pivotingElement.first );
                        Rational ratAss = (Rational)newBasicVar->assignment().mainPart();
                        if( newBasicVar->isActive() && newBasicVar->isInteger() && !carl::isInteger( ratAss ) )
                        {
                            if( !probablyLooping( newBasicVar->expression(), ratAss ) )
                            {
                                assert( newBasicVar->assignment().deltaPart() == 0 );
                                FormulasT premises;
                                mTableau.collect_premises( newBasicVar, premises );
                                std::vector<FormulaT> premisesOrigins;
                                for( auto& pf : premises )
                                {
                                    collectOrigins( pf, premisesOrigins );
                                }
                                branchAt( newBasicVar->expression(), true, ratAss, std::move(premisesOrigins) );
                                goto Return;
                            }
                        }
                    }
                    else
                    {
                        mTableau.pivot( pivotingElement.first );
                    }
                    #ifdef SMTRAT_DEVOPTION_Statistics
                    mpStatistics->pivotStep();
                    #endif
                    #ifdef LRA_REFINEMENT
                    // Learn all bounds which have been deduced during the pivoting process.
                    while( !mTableau.rNewLearnedBounds().empty() )
                    {
                        FormulasT originSet;
                        typename LRATableau::LearnedBound& learnedBound = mTableau.rNewLearnedBounds().back()->second;
                        mTableau.rNewLearnedBounds().pop_back();
                        std::vector<const LRABound*>& bounds = learnedBound.premise;
                        for( auto bound = bounds.begin(); bound != bounds.end(); ++bound )
                        {
                            const FormulaT& boundOrigins = *(*bound)->origins().begin(); 
                            if( boundOrigins.getType() == carl::FormulaType::AND )
                            {
                                originSet.insert( originSet.end(), boundOrigins.subformulas().begin(), boundOrigins.subformulas().end() );
                                for( auto origin = boundOrigins.subformulas().begin(); origin != boundOrigins.subformulas().end(); ++origin )
                                {
                                    auto constrBoundIter = mTableau.rConstraintToBound().find( boundOrigins );
                                    assert( constrBoundIter != mTableau.constraintToBound().end() );
                                    std::vector< const LRABound* >* constraintToBounds = constrBoundIter->second;
                                    constraintToBounds->push_back( learnedBound.nextWeakerBound );
                                    #ifdef LRA_INTRODUCE_NEW_CONSTRAINTS
                                    if( learnedBound.newBound != NULL ) constraintToBounds->push_back( learnedBound.newBound );
                                    #endif
                                }
                            }
                            else
                            {
                                assert( boundOrigins.getType() == carl::FormulaType::CONSTRAINT );
                                originSet.push_back( boundOrigins );
                                auto constrBoundIter = mTableau.rConstraintToBound().find( boundOrigins );
                                assert( constrBoundIter != mTableau.constraintToBound().end() );
                                std::vector< const LRABound* >* constraintToBounds = constrBoundIter->second;
                                constraintToBounds->push_back( learnedBound.nextWeakerBound );
                                #ifdef LRA_INTRODUCE_NEW_CONSTRAINTS
                                if( learnedBound.newBound != NULL ) constraintToBounds->push_back( learnedBound.newBound );
                                #endif
                            }
                        }
                        FormulaT origin = FormulaT( carl::FormulaType::AND, std::move(originSet) );
                        activateBound( learnedBound.nextWeakerBound, origin );
                        #ifdef LRA_INTRODUCE_NEW_CONSTRAINTS
                        if( learnedBound.newBound != NULL )
                        {
                            const FormulaT& newConstraint = learnedBound.newBound->asConstraint();
                            addConstraintToInform( newConstraint );
                            mLinearConstraints.insert( newConstraint );
                            std::vector< const LRABound* >* boundVector = new std::vector< const LRABound* >();
                            boundVector->push_back( learnedBound.newBound );
                            mConstraintToBound[newConstraint] = boundVector;
                            activateBound( learnedBound.newBound, origin );
                        }
                        #endif
                    }
                    #endif
                    // Maybe an easy conflict occurred with the learned bounds.
                    if( !mInfeasibleSubsets.empty() )
                    {
                        result = False;
                        goto Return;
                    }
                }
            }
            // There is a conflict, namely a basic variable violating its bounds without any suitable non-basic variable.
            else
            {
                // Create the infeasible subsets.
                if( Settings::one_conflict_reason )
                {
                    std::vector< const LRABound* > conflict = mTableau.getConflict( pivotingElement.first );
                    FormulaSetT infSubSet;
                    for( auto bound = conflict.begin(); bound != conflict.end(); ++bound )
                    {
                        assert( (*bound)->isActive() );
                        collectOrigins( *(*bound)->origins().begin(), infSubSet );
                    }
                    mInfeasibleSubsets.push_back( infSubSet );
                }
                else
                {
                    std::vector< std::vector< const LRABound* > > conflictingBounds = mTableau.getConflictsFrom( pivotingElement.first );
                    for( auto conflict = conflictingBounds.begin(); conflict != conflictingBounds.end(); ++conflict )
                    {
                        FormulaSetT infSubSet;
                        for( auto bound = conflict->begin(); bound != conflict->end(); ++bound )
                        {
                            assert( (*bound)->isActive() );
                            collectOrigins( *(*bound)->origins().begin(), infSubSet );
                        }
                        mInfeasibleSubsets.push_back( infSubSet );
                    }
                }
                result = False;
                goto Return;
            }
        }
        assert( false );
Return:
        #ifdef LRA_REFINEMENT
        learnRefinements();
        #endif
        #ifdef SMTRAT_DEVOPTION_Statistics
        if( result != Unknown )
        {
            mpStatistics->check( rReceivedFormula() );
            if( result == False )
                mpStatistics->addConflict( mInfeasibleSubsets );
            mpStatistics->setNumberOfTableauxEntries( mTableau.size() );
            mpStatistics->setTableauSize( mTableau.rows().size()*mTableau.columns().size() );
        }
        #endif
        if( result != Unknown )
        {
            mTableau.resetNumberOfPivotingSteps();
            if( result == True && backendsResultUnknown )
            {
                // If there are unresolved notequal-constraints and the found satisfying assignment
                // conflicts this constraint, resolve it by creating the lemma (p<0 or p>0) <-> p!=0 ) and return Unknown.
                EvalRationalMap ass = getRationalModel();
                for( auto iter = mActiveUnresolvedNEQConstraints.begin(); iter != mActiveUnresolvedNEQConstraints.end(); ++iter )
                {
                    unsigned consistency = iter->first.satisfiedBy( ass );
                    assert( consistency != 2 );
                    if( consistency == 0 )
                    {
                        splitUnequalConstraint( iter->first );
                        result = Unknown;
                        break;
                    }
                }
                // TODO: This is a rather unfortunate hack, because I couldn't fix the efficient neq-constraint-handling with integer-valued constraints
                if( result != Unknown && !rReceivedFormula().isRealConstraintConjunction() )
                {
                    for( auto iter = mActiveResolvedNEQConstraints.begin(); iter != mActiveResolvedNEQConstraints.end(); ++iter )
                    {
                        unsigned consistency = iter->first.satisfiedBy( ass );
                        assert( consistency != 2 );
                        if( consistency == 0 )
                        {
                            splitUnequalConstraint( iter->first );
                            result = Unknown;
                            break;
                        }
                    }
                }
                assert( result != True || assignmentCorrect() );
            }
        }
        #ifdef DEBUG_LRA_MODULE
        cout << endl;
        mTableau.print();
        cout << endl;
        cout << ANSWER_TO_STRING( result ) << endl;
        #endif
        return result;
    }

    template<class Settings>
    void LRAModule<Settings>::updateModel() const
    {
        clearModel();
        if( solverState() == True )
        {
            if( mAssignmentFullfilsNonlinearConstraints )
            {
                EvalRationalMap rationalAssignment = getRationalModel();
                for( auto ratAss = rationalAssignment.begin(); ratAss != rationalAssignment.end(); ++ratAss )
                {
                    mModel.insert(mModel.end(), std::make_pair(ratAss->first, ratAss->second) );
                }
            }
            else
            {
                Module::getBackendsModel();
            }
        }
    }

    template<class Settings>
    EvalRationalMap LRAModule<Settings>::getRationalModel() const
    {
        if( mInfeasibleSubsets.empty() )
        {
            auto ret = mTableau.getRationalAssignment();
            return ret;
        }
        return EvalRationalMap();
    }

    template<class Settings>
    EvalRationalIntervalMap LRAModule<Settings>::getVariableBounds() const
    {
        EvalRationalIntervalMap result;
        for( auto iter = mTableau.originalVars().begin(); iter != mTableau.originalVars().end(); ++iter )
        {
            const LRAVariable& var = *iter->second;
            carl::BoundType lowerBoundType;
            Rational lowerBoundValue;
            carl::BoundType upperBoundType;
            Rational upperBoundValue;
            if( var.infimum().isInfinite() )
            {
                lowerBoundType = carl::BoundType::INFTY;
                lowerBoundValue = 0;
            }
            else
            {
                lowerBoundType = var.infimum().isWeak() ? carl::BoundType::WEAK : carl::BoundType::STRICT;
                lowerBoundValue = Rational(var.infimum().limit().mainPart());
            }
            if( var.supremum().isInfinite() )
            {
                upperBoundType = carl::BoundType::INFTY;
                upperBoundValue = 0;
            }
            else
            {
                upperBoundType = var.supremum().isWeak() ? carl::BoundType::WEAK : carl::BoundType::STRICT;
                upperBoundValue = Rational(var.supremum().limit().mainPart());
            }
            RationalInterval interval = RationalInterval( lowerBoundValue, lowerBoundType, upperBoundValue, upperBoundType );
            result.insert( std::pair< carl::Variable, RationalInterval >( iter->first, interval ) );
        }
        return result;
    }
    
    #ifdef LRA_REFINEMENT
    template<class Settings>
    void LRAModule<Settings>::learnRefinements()
    {
        for( auto iter = mTableau.rLearnedLowerBounds().begin(); iter != mTableau.rLearnedLowerBounds().end(); ++iter )
        {
            FormulasT subformulas;
            for( auto bound = iter->second.premise.begin(); bound != iter->second.premise.end(); ++bound )
            {
                const FormulaT& origin = *(*bound)->origins().begin();
                if( origin.getType() == carl::VariableType::AND )
                {
                    for( auto& subformula : origin.subformulas() )
                    {
                        assert( subformula.getType() == carl::VariableType::CONSTRAINT );
                        subformulas.emplace_back( carl::FormulaType::NOT, subformula );
                    }
                }
                else
                {
                    assert( origin.getType() == carl::VariableType::CONSTRAINT );
                    subformulas.emplace_back( carl::FormulaType::NOT, origin )
                }
            }
            subformulas.push_back( iter->second.nextWeakerBound->asConstraint() );
            addDeduction( FormulaT( carl::FormulaType::OR, std::move(subformulas) ) );
            #ifdef SMTRAT_DEVOPTION_Statistics
            mpStatistics->addRefinement();
            mpStatistics->addDeduction();
            #endif
        }
        mTableau.rLearnedLowerBounds().clear();
        for( auto iter = mTableau.rLearnedUpperBounds().begin(); iter != mTableau.rLearnedUpperBounds().end(); ++iter )
        {
            FormulasT subformulas;
            for( auto bound = iter->second.premise.begin(); bound != iter->second.premise.end(); ++bound )
            {
                const FormulaT& origin = *(*bound)->origins().begin();
                if( origin.getType() == carl::VariableType::AND )
                {
                    for( auto& subformula : origin.subformulas() )
                    {
                        assert( subformula.getType() == carl::VariableType::CONSTRAINT );
                        subformulas.emplace_back( carl::FormulaType::NOT, subformula );
                    }
                }
                else
                {
                    assert( origin.getType() == carl::VariableType::CONSTRAINT );
                    subformulas.emplace_back( carl::FormulaType::NOT, origin );
                }
            }
            subformulas.push_back( iter->second.nextWeakerBound->asConstraint() );
            addDeduction( FormulaT( carl::FormulaType::OR, std::move(subformulas) ) );
            #ifdef SMTRAT_DEVOPTION_Statistics
            mpStatistics->addRefinement();
            mpStatistics->addDeduction();
            #endif
        }
        mTableau.rLearnedUpperBounds().clear();
    }
    #endif

    template<class Settings>
    void LRAModule<Settings>::adaptPassedFormula()
    {
        while( !mBoundCandidatesToPass.empty() )
        {
            const LRABound& bound = *mBoundCandidatesToPass.back();
            if( bound.pInfo()->updated > 0 )
            {
                bound.pInfo()->position = addSubformulaToPassedFormula( bound.asConstraint(), bound.pOrigins() ).first;
                bound.pInfo()->updated = 0;
            }
            else if( bound.pInfo()->updated < 0 )
            {
                eraseSubformulaFromPassedFormula( bound.pInfo()->position, true );
                bound.pInfo()->position = passedFormulaEnd();
                bound.pInfo()->updated = 0;
            }
            mBoundCandidatesToPass.pop_back();
        }
    }

    template<class Settings>
    bool LRAModule<Settings>::checkAssignmentForNonlinearConstraint()
    {
        if( mNonlinearConstraints.empty() )
        {
            mAssignmentFullfilsNonlinearConstraints = true;
            return true;
        }
        else
        {
            EvalRationalMap assignments = getRationalModel();
            // Check whether the assignment satisfies the non linear constraints.
            for( auto constraint = mNonlinearConstraints.begin(); constraint != mNonlinearConstraints.end(); ++constraint )
            {
                if( constraint->satisfiedBy( assignments ) != 1 )
                {
                    return false;
                }
            }
            mAssignmentFullfilsNonlinearConstraints = true;
            return true;
        }
    }

    template<class Settings>
    void LRAModule<Settings>::activateBound( const LRABound* _bound, const FormulaT& _formula )
    {
        if( mStrongestBoundsRemoved )
        {
            mTableau.resetAssignment();
            mStrongestBoundsRemoved = false;
        }
        if( Settings::simple_conflicts_and_propagation_on_demand )
        {
            if( Settings::simple_theory_propagation )
            {
                addSimpleBoundDeduction( _bound, true, false );
            }
            if( Settings::simple_conflict_search )
            {
                findSimpleConflicts( *_bound );
            }
        }
        // If the bounds constraint has already been passed to the backend, add the given formulas to it's origins
        const LRAVariable& var = _bound->variable();
        const LRABound* psup = var.pSupremum();
        const LRABound& sup = *psup;
        const LRABound* pinf = var.pInfimum();
        const LRABound& inf = *pinf;
        const LRABound& bound = *_bound;
        mTableau.activateBound( _bound, _formula );
        if( bound.isUpperBound() )
        {
            if( inf > bound.limit() && !bound.deduced() )
            {
                FormulaSetT infsubset;
                collectOrigins( *bound.origins().begin(), infsubset );
                collectOrigins( inf.pOrigins()->back(), infsubset );
                mInfeasibleSubsets.push_back( std::move(infsubset) );
            }
            if( sup > bound )
            {
                if( !sup.isInfinite() )
                    mBoundCandidatesToPass.push_back( psup );
                mBoundCandidatesToPass.push_back( _bound );
            }
        }
        if( bound.isLowerBound() )
        {
            if( sup < bound.limit() && !bound.deduced() )
            {
                FormulaSetT infsubset;
                collectOrigins( *bound.origins().begin(), infsubset );
                collectOrigins( sup.pOrigins()->back(), infsubset );
                mInfeasibleSubsets.push_back( std::move(infsubset) );
            }
            if( inf < bound )
            {
                if( !inf.isInfinite() )
                    mBoundCandidatesToPass.push_back( pinf );
                mBoundCandidatesToPass.push_back( _bound );
            }
        }
        assert( mInfeasibleSubsets.empty() || !mInfeasibleSubsets.begin()->empty() );
        #ifdef SMTRAT_DEVOPTION_Statistics
        if( !mInfeasibleSubsets.empty() )
            mpStatistics->addConflict( mInfeasibleSubsets );
        #endif
    }
    
    template<class Settings>
    void LRAModule<Settings>::activateStrictBound( const FormulaT& _neqOrigin, const LRABound& _weakBound, const LRABound* _strictBound )
    {
        FormulaSetT involvedConstraints;
        FormulasT originSet;
        originSet.push_back( _neqOrigin );
        auto iter = _weakBound.origins().begin();
        assert( iter != _weakBound.origins().end() );
        if( iter->getType() == carl::FormulaType::AND )
        {
            originSet.insert( originSet.end(), iter->subformulas().begin(), iter->subformulas().end() );
            involvedConstraints.insert( iter->subformulas().begin(), iter->subformulas().end() );
        }
        else
        {
            assert( iter->getType() == carl::FormulaType::CONSTRAINT );
            originSet.push_back( *iter );
            involvedConstraints.insert( *iter );
        }
        FormulaT origin = FormulaT( carl::FormulaType::AND, std::move(originSet) );
        activateBound( _strictBound, origin );
        ++iter;
        while( iter != _weakBound.origins().end() )
        {
            FormulasT originSetB;
            originSetB.push_back( _neqOrigin );
            if( iter->getType() == carl::FormulaType::AND )
            {
                originSetB.insert( originSetB.end(), iter->subformulas().begin(), iter->subformulas().end() );
                involvedConstraints.insert( iter->subformulas().begin(), iter->subformulas().end() );
            }
            else
            {
                assert( iter->getType() == carl::FormulaType::CONSTRAINT );
                originSetB.push_back( *iter );
                involvedConstraints.insert( *iter );
            }
            FormulaT originB = FormulaT( carl::FormulaType::AND, std::move(originSet) );
            _strictBound->pOrigins()->push_back( originB );
            ++iter;
        }
        for( const FormulaT& fconstraint : involvedConstraints )
        {
            auto constrBoundIter = mTableau.rConstraintToBound().find( fconstraint );
            assert( constrBoundIter != mTableau.constraintToBound().end() );
            std::vector< const LRABound* >* constraintToBounds = constrBoundIter->second;
            constraintToBounds->push_back( _strictBound );
        }
    }

    template<class Settings>
    void LRAModule<Settings>::setBound( const FormulaT& _constraint )
    {
        if( Settings::simple_conflicts_and_propagation_on_demand )
        {
            mTableau.newBound( _constraint );
        }
        else
        {
            std::pair<const LRABound*, bool> retValue = mTableau.newBound( _constraint );
            if( retValue.second )
            {
                if( Settings::simple_theory_propagation )
                {
                    addSimpleBoundDeduction( retValue.first, true, _constraint.constraint().relation() == carl::Relation::NEQ );
                }
                if( Settings::simple_conflict_search )
                {
                    findSimpleConflicts( *retValue.first );
                }
            }
        }
    }
    
    template<class Settings>
    void LRAModule<Settings>::addSimpleBoundDeduction( const LRABound* _bound, bool _exhaustively, bool _boundNeq )
    {
        const LRAVariable& lraVar = _bound->variable();
        if( _bound->isUpperBound() )
        {
            LRABound::BoundSet::const_iterator boundPos = lraVar.upperbounds().find( _bound );
            assert( boundPos != lraVar.upperbounds().end() );
            LRABound::BoundSet::const_iterator currentBound = lraVar.upperbounds().begin();
            if( _bound->type() == LRABound::Type::EQUAL )
            {
                currentBound = boundPos;
                ++currentBound;
            }
            else
            {
                while( currentBound != boundPos )
                {
                    if( _exhaustively && (*currentBound)->pInfo()->exists )
                    {
                        FormulasT subformulas
                        {
                            FormulaT(carl::FormulaType::NOT, (*currentBound)->asConstraint()), 
                            (_boundNeq ? _bound->neqRepresentation() : _bound->asConstraint())
                        };
                        addDeduction( FormulaT( carl::FormulaType::OR, std::move(subformulas) ) );
                        #ifdef SMTRAT_DEVOPTION_Statistics
                        mpStatistics->addDeduction();
                        #endif
                    }
                    ++currentBound;
                }
                ++currentBound;
            }
            if( !_boundNeq )
            {
                while( currentBound != lraVar.upperbounds().end() )
                {
                    if( (*currentBound)->pInfo()->exists && (*currentBound)->type() != LRABound::Type::EQUAL )
                    {
                        FormulasT subformulas
                        {
                            FormulaT( carl::FormulaType::NOT, _bound->asConstraint() ),
                            (*currentBound)->asConstraint()
                        };
                        addDeduction( FormulaT( carl::FormulaType::OR, std::move(subformulas) ) );
                        #ifdef SMTRAT_DEVOPTION_Statistics
                        mpStatistics->addDeduction();
                        #endif
                    }
                    ++currentBound;
                }
            }
        }
        if( _bound->isLowerBound() )
        {
            LRABound::BoundSet::const_iterator boundPos = lraVar.lowerbounds().find( _bound );
            assert( boundPos != lraVar.lowerbounds().end() );
            LRABound::BoundSet::const_iterator currentBound = lraVar.lowerbounds().begin();
            if( _boundNeq )
            {
                currentBound = boundPos;
                ++currentBound;
            }
            else
            {
                while( currentBound != boundPos )
                {
                    if( (*currentBound)->pInfo()->exists && (*currentBound)->type() != LRABound::Type::EQUAL )
                    {
                        FormulasT subformulas
                        {
                            FormulaT( carl::FormulaType::NOT, _bound->asConstraint() ),
                            (*currentBound)->asConstraint()
                        };
                        addDeduction( FormulaT( carl::FormulaType::OR, std::move(subformulas) ) );
                        #ifdef SMTRAT_DEVOPTION_Statistics
                        mpStatistics->addDeduction();
                        #endif
                    }
                    ++currentBound;
                }
                if( _exhaustively )
                    ++currentBound;
            }
            if( _exhaustively && _bound->type() != LRABound::Type::EQUAL )
            {
                while( currentBound != lraVar.lowerbounds().end() )
                {
                    if( (*currentBound)->pInfo()->exists )
                    {
                        FormulasT subformulas
                        {
                            FormulaT( carl::FormulaType::NOT, (*currentBound)->asConstraint() ),
                            ( _boundNeq ? _bound->neqRepresentation() : _bound->asConstraint() )
                        };
                        addDeduction( FormulaT( carl::FormulaType::OR, std::move(subformulas) ) );
                        #ifdef SMTRAT_DEVOPTION_Statistics
                        mpStatistics->addDeduction();
                        #endif
                    }
                    ++currentBound;
                }
            }
        }
    }
    
    template<class Settings>
    void LRAModule<Settings>::addSimpleBoundConflict( const LRABound& _caseA, const LRABound& _caseB, bool _caseBneq )
    {
        FormulasT subformulas
        {
            FormulaT( carl::FormulaType::NOT, _caseA.asConstraint() ),
            FormulaT( carl::FormulaType::NOT, _caseBneq ? _caseB.neqRepresentation() : _caseB.asConstraint() )
        };
        addDeduction( FormulaT( carl::FormulaType::OR, std::move(subformulas) ) );
        #ifdef SMTRAT_DEVOPTION_Statistics
        mpStatistics->addDeduction();
        #endif
    }

    template<class Settings>
    void LRAModule<Settings>::findSimpleConflicts( const LRABound& _bound )
    {
        assert( !_bound.deduced() );
        if( _bound.isUpperBound() )
        {
            const LRABound::BoundSet& lbounds = _bound.variable().lowerbounds();
            for( auto lbound = lbounds.rbegin(); lbound != --lbounds.rend(); ++lbound )
            {
                if( **lbound > _bound.limit() && !(*lbound)->asConstraint().isTrue() )
                {
                    if( !(*lbound)->neqRepresentation().isTrue() )
                    {
                        if( _bound.type() == LRABound::EQUAL && (*lbound)->limit().mainPart() == _bound.limit().mainPart() )
                        {
                            addSimpleBoundConflict( _bound, **lbound, true );
                        }
                    }
                    else if( !_bound.neqRepresentation().isTrue() )
                    {
                        if( (*lbound)->type() == LRABound::EQUAL && (*lbound)->limit().mainPart() == _bound.limit().mainPart() )
                        {
                            addSimpleBoundConflict( **lbound, _bound, true );
                        }
                    }
                    else
                    {
                        addSimpleBoundConflict( _bound, **lbound );
                    }
                }
                else
                {
                    break;
                }
            }
        }
        if( _bound.isLowerBound() )
        {
            const LRABound::BoundSet& ubounds = _bound.variable().upperbounds();
            for( auto ubound = ubounds.begin(); ubound != --ubounds.end(); ++ubound )
            {
                if( **ubound < _bound.limit() && !(*ubound)->asConstraint().isTrue() )
                {
                    if( !(*ubound)->neqRepresentation().isTrue() )
                    {
                        if( _bound.type() == LRABound::EQUAL && (*ubound)->limit().mainPart() == _bound.limit().mainPart() )
                        {
                            addSimpleBoundConflict( _bound, **ubound, true );
                        }
                    }
                    else if( !_bound.neqRepresentation().isTrue() )
                    {
                        if( (*ubound)->type() == LRABound::EQUAL && (*ubound)->limit().mainPart() == _bound.limit().mainPart() )
                        {
                            addSimpleBoundConflict( **ubound, _bound, true );
                        }
                    }
                    else
                    {
                        addSimpleBoundConflict( _bound, **ubound );
                    }
                }
                else
                {
                    break;
                }
            }
        }
    }

    template<class Settings>
    void LRAModule<Settings>::init()
    {
        if( !mInitialized )
        {
            mInitialized = true;
            for( const FormulaT& constraint : mLinearConstraints )
            {
                setBound( constraint );
            }
//            mTableau.setSize( mTableau.slackVars().size(), mTableau.originalVars().size(), mLinearConstraints.size() );
            #ifdef LRA_USE_PIVOTING_STRATEGY
            mTableau.setBlandsRuleStart( 1000 );//(unsigned) mTableau.columns().size() );
            #endif
        }
    }
    
    template<class Settings>
    bool LRAModule<Settings>::gomory_cut()
    {
        EvalRationalMap rMap_ = getRationalModel();
        bool all_int = true;
        for( LRAVariable* basicVar : mTableau.rows() )
        {            
            if( basicVar->isOriginal() )
            {
                carl::Variables vars;
                basicVar->expression().gatherVariables( vars );
                assert( vars.size() == 1 );
                auto found_ex = rMap_.find(*vars.begin()); 
                Rational& ass = found_ex->second;
                if( !carl::isInteger( ass ) )
                {
                    all_int = false;
                    const Poly::PolyType* gomory_poly = mTableau.gomoryCut(ass, basicVar);
                    if( *gomory_poly != ZERO_RATIONAL )
                    { 
                        ConstraintT gomory_constr = ConstraintT( *gomory_poly , carl::Relation::GEQ );
                        ConstraintT neg_gomory_constr = ConstraintT( *gomory_poly - (*gomory_poly).evaluate( rMap_ ), carl::Relation::LESS );
                        //std::cout << gomory_constr << endl;
                        assert( !gomory_constr.satisfiedBy( rMap_ ) );
                        assert( !neg_gomory_constr.satisfiedBy( rMap_ ) );
                        /*
                        FormulaSetT subformulas; 
                        mTableau.collect_premises( basicVar, subformulas );
                        FormulaSetT premisesOrigins;
                        for( auto& pf : subformulas )
                        {
                            collectOrigins( pf, premisesOrigins );
                        }
                        FormulaSetT premise;
                        for( const Formula* pre : premisesOrigins )
                        {
                            premise.insert( FormulaT( carl::FormulaType::NOT, pre ) );
                        }
                        */
                        FormulaT gomory_formula = FormulaT( gomory_constr );
                        FormulaT neg_gomory_formula = FormulaT( neg_gomory_constr );
                        FormulasT subformulas = { gomory_formula, neg_gomory_formula };
                        FormulaT branch_formula = FormulaT( carl::FormulaType::OR, std::move( subformulas ) );
                        //premise.insert( gomory_formula );
                        addDeduction( branch_formula );
                    } 
                }
            }    
        }          
        return !all_int;
    }
    
    template<class Settings>
    bool LRAModule<Settings>::cuts_from_proofs()
    {
        #ifdef LRA_DEBUG_CUTS_FROM_PROOFS
        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
        cout << "Cut from proofs:" << endl;
        mTableau.print();
        #endif
        // Check if the solution is integer.
        EvalRationalMap _rMap = getRationalModel();
        auto map_iterator = _rMap.begin();
        auto var = mTableau.originalVars().begin();
        while( var != mTableau.originalVars().end() )
        {
            assert( var->first == map_iterator->first );
            Rational& ass = map_iterator->second; 
            if( var->first.getType() == carl::VariableType::VT_INT && !carl::isInteger( ass ) )
            {
                #ifdef LRA_DEBUG_CUTS_FROM_PROOFS
                cout << "Fix: " << var->first << endl;
                #endif
                break;
            }
            ++map_iterator;
            ++var;
        }
        if( var == mTableau.originalVars().end() )
        {
            #ifdef LRA_DEBUG_CUTS_FROM_PROOFS
            cout << "Assignment is already integer!" << endl;
            cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
            #endif
            return false;
        }
        /*
         * Build the new Tableau consisting out of the defining constraints.
         */
        LRATableau dc_Tableau = LRATableau( passedFormulaEnd() );  
        size_t i=0;
        for( auto nbVar = mTableau.columns().begin(); nbVar != mTableau.columns().end(); ++nbVar )
        {
            dc_Tableau.newNonbasicVariable( new Poly::PolyType( (*mTableau.columns().at(i)).expression() ), true );
            ++i;
        } 
        size_t numRows = mTableau.rows().size();
        LRAEntryType max_value = 0;
        std::vector<size_t> dc_positions = std::vector<size_t>();
        #ifdef LRA_NO_DIVISION
        std::vector<LRAEntryType> lcm_rows;
        #endif
        std::vector<ConstraintT> DC_Matrix;
        for( size_t i = 0; i < numRows; ++i )
        {
            std::vector<std::pair<size_t,LRAEntryType>> nonbasicindex_coefficient = std::vector<std::pair<size_t,LRAEntryType>>();
            LRAEntryType lcmOfCoeffDenoms = 1;
            ConstraintT dc_constraint = mTableau.isDefining( i, nonbasicindex_coefficient,  lcmOfCoeffDenoms, max_value );
            if( dc_constraint != ConstraintT()  )
            {      
                LRAVariable* new_var = dc_Tableau.newBasicVariable( nonbasicindex_coefficient, (*mTableau.rows().at(i)).expression(), (*mTableau.rows().at(i)).factor(),dc_constraint.integerValued() );
                dc_Tableau.activateBasicVar(new_var);
                dc_positions.push_back(i); 
                #ifdef LRA_NO_DIVISION
                lcm_rows.push_back( lcmOfCoeffDenoms );
                #endif
                DC_Matrix.push_back( dc_constraint );
            }   
        }
        #ifdef LRA_NO_DIVISION
        /* Multiply all defining constraints with the corresponding least common multiple 
         * of the occuring denominators in order to ensure that we work on a tableau 
         * contatining only integers
         */
        for( size_t i = 0; i < dc_Tableau.rows().size(); ++i )
        {
            dc_Tableau.multiplyRow(i,lcm_rows.at(i));          
        }
        #endif
        auto pos = mProcessedDCMatrices.find( DC_Matrix );
        if( pos == mProcessedDCMatrices.end() )
        {
            mProcessedDCMatrices.insert( DC_Matrix );
        }
        if( dc_Tableau.rows().size() > 0 && pos == mProcessedDCMatrices.end() )
        {
            #ifdef LRA_DEBUG_CUTS_FROM_PROOFS
            cout << "Defining constraints:" << endl;
            dc_Tableau.print();   
            #endif

            // At least one DC exists -> Construct and embed it.
            std::vector<size_t> diagonals = std::vector<size_t>();    
            std::vector<size_t>& diagonals_ref = diagonals;               
            /*
            dc_Tableau.addColumns(0,2,2);
            dc_Tableau.addColumns(1,2,4);
            dc_Tableau.addColumns(2,2,2);
            dc_Tableau.addColumns(3,4,1);
            dc_Tableau.addColumns(5,4,1);
            dc_Tableau.addColumns(4,4,-1);
            dc_Tableau.print( LAST_ENTRY_ID, std::cout, "", true, true );
            */
            bool full_rank = true;
            dc_Tableau.calculate_hermite_normalform( diagonals_ref, full_rank );
            if( !full_rank )
            {
                branchAt( var->first, (Rational)map_iterator->second );
                return true;
            }
            #ifdef LRA_DEBUG_CUTS_FROM_PROOFS
            cout << "HNF of defining constraints:" << endl;
            dc_Tableau.print( LAST_ENTRY_ID, std::cout, "", true, true );
            cout << "Actual order of columns:" << endl;
            for( auto iter = diagonals.begin(); iter != diagonals.end(); ++iter ) 
                printf( "%u", (unsigned)*iter );
            #endif  
            dc_Tableau.invert_HNF_Matrix( diagonals );
            #ifdef LRA_DEBUG_CUTS_FROM_PROOFS
            cout << "Inverted matrix:" << endl;
            dc_Tableau.print( LAST_ENTRY_ID, std::cout, "", true, true );
            #endif 
            Poly::PolyType* cut_from_proof = nullptr;
            for( size_t i = 0; i < dc_positions.size(); ++i )
            {
                LRAEntryType upper_lower_bound;
                cut_from_proof = dc_Tableau.create_cut_from_proof( dc_Tableau, mTableau, i, diagonals, dc_positions, upper_lower_bound, max_value );
                if( cut_from_proof != nullptr )
                {
                    #ifdef LRA_DEBUG_CUTS_FROM_PROOFS
                    cout << "Proof of unsatisfiability:  " << *cut_from_proof << " = 0" << endl;
                    #endif
                    LRAEntryType bound_add = 1;
                    LRAEntryType bound = upper_lower_bound;
                    if( carl::isInteger( upper_lower_bound ) )
                    {
                        bound_add = 0;
                    }
                    ConstraintT cut_constraint = ConstraintT( *cut_from_proof - (Rational)carl::floor((Rational)upper_lower_bound) , carl::Relation::LEQ );
                    ConstraintT cut_constraint2 = ConstraintT( *cut_from_proof - Rational(((Rational)carl::floor((Rational)upper_lower_bound)+Rational(bound_add))), carl::Relation::GEQ );
                    // Construct and add (p<=I-1 or p>=I))
                    FormulaT cons1 = FormulaT( cut_constraint );
                    cons1.setActivity( -numeric_limits<double>::infinity() );
                    FormulaT cons2 = FormulaT( cut_constraint2 );
                    cons2.setActivity( -numeric_limits<double>::infinity() );
                    addDeduction( FormulaT( carl::FormulaType::OR, FormulasT{ cons1, cons2 } ) );   
                    // (not(p<=I-1) or not(p>=I))
                    FormulasT subformulasB{ FormulaT( carl::FormulaType::NOT, cons1 ), FormulaT( carl::FormulaType::NOT, cons2 ) };
                    addDeduction( FormulaT( carl::FormulaType::OR, std::move( subformulasB ) ) );
                    #ifdef LRA_DEBUG_CUTS_FROM_PROOFS
                    cout << "After adding proof of unsatisfiability:" << endl;
                    mTableau.print( LAST_ENTRY_ID, std::cout, "", true, true );
                    cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
                    #endif
                    delete cut_from_proof;
                    return true;
                }
            }
            #ifdef LRA_DEBUG_CUTS_FROM_PROOFS
            cout << "Found no proof of unsatisfiability!" << endl;
            #endif
        }
        #ifdef LRA_DEBUG_CUTS_FROM_PROOFS
            cout << "No defining constraint!" << endl;
        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
        cout << "Branch at: " << var->first << endl;
        #endif
        branchAt( var->first, (Rational)map_iterator->second );
        return true;
    }
    
    template<class Settings>
    bool LRAModule<Settings>::branch_and_bound()
    {
        BRANCH_STRATEGY strat = MOST_INFEASIBLE;
        bool gc_support = true;
        bool result = true;
        if( strat == MIN_PIVOT )
        {
            result = minimal_row_var( gc_support );            
        }
        else if( strat == MOST_FEASIBLE )
        {
            result = most_feasible_var( gc_support );
        }
        else if( strat == MOST_INFEASIBLE )
        {
            result = most_infeasible_var( gc_support );
        }
        else if( strat == NATIVE )
        {
            result = first_var( gc_support );
        }  
        else if( strat == PSEUDO_COST )
        {
            BRANCH_STRATEGY alternative_strat = MOST_INFEASIBLE;
            result = pseudo_cost_branching( gc_support, alternative_strat );
        } 
        return result;
    }
    
    template<class Settings>
    bool LRAModule<Settings>::maybeGomoryCut( const LRAVariable* _lraVar, const Rational& _branchingValue )
    {
        if( probablyLooping( _lraVar->expression(), _branchingValue ) )
        {
            return gomory_cut();
        }
        branchAt( _lraVar->expression(), true, _branchingValue );
        return true;
    }
    
    template<class Settings>
    bool LRAModule<Settings>::minimal_row_var( bool _gc_support )
    {
        EvalRationalMap _rMap = getRationalModel();
        auto branch_var = mTableau.originalVars().begin();
        Rational ass_;
        Rational row_count_min = mTableau.columns().size()+1;
        bool result = false;
        for( auto map_iterator = _rMap.begin(); map_iterator != _rMap.end(); ++map_iterator )
        {
            auto var = mTableau.originalVars().find( map_iterator->first );
            assert( var->first == map_iterator->first );
            Rational& ass = map_iterator->second;
            if( var->first.getType() == carl::VariableType::VT_INT && !carl::isInteger( ass ) )
            {
                size_t row_count_new = mTableau.getNumberOfEntries( var->second );
                if( row_count_new < row_count_min )
                {
                    result = true;
                    row_count_min = row_count_new; 
                    branch_var = var;
                    ass_ = ass; 
                }
            }
        }
        if( result )
        {
            if( _gc_support )
            {
                return maybeGomoryCut( branch_var->second, ass_ );
            }
//            FormulaSetT premises;
//            mTableau.collect_premises( branch_var->second , premises  ); 
//            FormulaSetT premisesOrigins;
//            for( auto& pf : premises )
//            {
//                collectOrigins( pf, premisesOrigins );
//            }
            branchAt( branch_var->second->expression(), true, ass_ );
            return true;
        }
        else
        {
            return false;
        }
    }
    
    template<class Settings>
    bool LRAModule<Settings>::most_feasible_var( bool _gc_support )
    {
        EvalRationalMap _rMap = getRationalModel();
        auto branch_var = mTableau.originalVars().begin();
        Rational ass_;
        bool result = false;
        Rational diff = MINUS_ONE_RATIONAL;
        for( auto map_iterator = _rMap.begin(); map_iterator != _rMap.end(); ++map_iterator )
        {
            auto var = mTableau.originalVars().find( map_iterator->first );
            assert( var->first == map_iterator->first );
            Rational& ass = map_iterator->second; 
            if( var->first.getType() == carl::VariableType::VT_INT && !carl::isInteger( ass ) )
            {
                Rational curr_diff = ass - carl::floor(ass);
                if( carl::abs(Rational(curr_diff -  ONE_RATIONAL/Rational(2))) > diff )
                {
                    result = true;
                    diff = carl::abs(Rational(curr_diff -  ONE_RATIONAL/Rational(2))); 
                    branch_var = var;
                    ass_ = ass;                   
                }
            }
        }
        if( result )
        {
            if( _gc_support )
            {
                return maybeGomoryCut( branch_var->second, ass_ );
            }
//            FormulaSetT premises;
//            mTableau.collect_premises( branch_var->second , premises  );
//            FormulaSetT premisesOrigins;
//            for( auto& pf : premises )
//            {
//                collectOrigins( pf, premisesOrigins );
//            }            
            branchAt( branch_var->second->expression(), true, ass_ );
            return true;         
        }
        else
        {
            return false;
        } 
    }
    
    template<class Settings>
    bool LRAModule<Settings>::most_infeasible_var( bool _gc_support ) 
    {
        EvalRationalMap _rMap = getRationalModel();
        
        auto branch_var = mTableau.originalVars().begin();
        Rational ass_;
        bool result = false;
        Rational diff = ONE_RATIONAL;
        for( auto map_iterator = _rMap.begin(); map_iterator != _rMap.end(); ++map_iterator )
        {
            auto var = mTableau.originalVars().find( map_iterator->first );
            assert( var->first == map_iterator->first );
            Rational& ass = map_iterator->second; 
            if( var->first.getType() == carl::VariableType::VT_INT && !carl::isInteger( ass ) )
            {
                Rational curr_diff = ass - carl::floor(ass);
                if( carl::abs( Rational(curr_diff - ONE_RATIONAL/Rational(2)) ) < diff )
                {
                    result = true;
                    diff = carl::abs( Rational(curr_diff -  ONE_RATIONAL/Rational(2)) ); 
                    branch_var = var;
                    ass_ = ass;                   
                }
            }
        }
        if( result )
        {
            if( _gc_support )
            {
                return maybeGomoryCut( branch_var->second, ass_ );
            }
//            FormulaSetT premises;
//            mTableau.collect_premises( branch_var->second , premises  ); 
//            FormulaSetT premisesOrigins;
//            for( auto& pf : premises )
//            {
//                collectOrigins( pf, premisesOrigins );
//            }
            branchAt( branch_var->second->expression(), true, ass_ );
            return true;
        }
        else
        {
            return false;
        } 
    }
    
    template<class Settings>
    bool LRAModule<Settings>::first_var( bool _gc_support )
    {
        EvalRationalMap _rMap = getRationalModel();
        for( auto map_iterator = _rMap.begin(); map_iterator != _rMap.end(); ++map_iterator )
        {
            auto var = mTableau.originalVars().find( map_iterator->first );
            assert( var->first == map_iterator->first );
            Rational& ass = map_iterator->second; 
            if( var->first.getType() == carl::VariableType::VT_INT && !carl::isInteger( ass ) )
            {
                if( _gc_support )
                {
                    return maybeGomoryCut( var->second, ass );
                }
//                FormulasT premises;
//                mTableau.collect_premises( var->second, premises  ); 
//                FormulasT premisesOrigins;
//                for( auto& pf : premises )
//                {
//                    collectOrigins( pf, premisesOrigins );
//                }
                branchAt( var->second->expression(), true, ass );
                return true;           
            }
        } 
        return false;
    }
    
    template<class Settings>
    void LRAModule<Settings>::calculatePseudoCosts()
    {
        // Count how many integer variables violate their domain
        EvalRationalMap _rMap = getRationalModel();
        auto map_iterator = _rMap.begin();
        unsigned count = 0;
        for( auto var = mTableau.originalVars().begin(); var != mTableau.originalVars().end(); ++var )
        {
            assert( var->first == map_iterator->first );
            Rational& ass = map_iterator->second;
            if( var->first.getType() == carl::VariableType::VT_INT && !carl::isInteger( ass ) )
            {
                count++;
            }
            ++map_iterator;
        }    
        // Go through the received constraints and store for which variables we branch 'left' 
        // resp. 'right'
        auto iter_constr = rReceivedFormula().begin();
        while( iter_constr != rReceivedFormula().end() )
        {
            Poly branching_poly = iter_constr->formula().constraint().lhs();
            std::set< carl::Variable > occ_vars;
            branching_poly.gatherVariables( occ_vars );
            // Check whether the current constraint is a branching constraint
            if( occ_vars.size() == 1 )
            {
                auto iter_poly = branching_poly.begin();
                while( iter_poly != branching_poly.end() )
                {
                    if( !iter_poly->isConstant() )
                    {    
                        if( iter_poly->isLinear() )
                        {
                            // Update mBranch_Success
                            auto iter_help = mBranch_Success.find( *( occ_vars.begin() ) );
                            if( iter_help == mBranch_Success.end() )
                            {
                                std::pair< carl::Variable, std::pair< std::vector< unsigned >, std::vector< unsigned > > > to_be_ins;
                                to_be_ins.first = *( occ_vars.begin() );
                                std::pair< std::vector< unsigned >, std::vector< unsigned > > new_pair;
                                if( branching_poly.begin()->coeff() < 0 )
                                {                               
                                    new_pair.first = std::vector< unsigned >();
                                    new_pair.second = std::vector< unsigned >( count );
                                }
                                else
                                {
                                    new_pair.second = std::vector< unsigned >();
                                    new_pair.first = std::vector< unsigned >( count );                                                                                                
                                }
                            }
                            else
                            {
                                if( branching_poly.begin()->coeff() < 0 )
                                {
                                    iter_help->second.second.push_back( count );
                                }
                                else
                                {
                                    iter_help->second.first.push_back( count );                                                    
                                }                                                
                            }
                        }
                    }
                    ++iter_poly;
                }    
            }
            ++iter_constr;
        }
    }
    
    template<class Settings>
    bool LRAModule<Settings>::pseudo_cost_branching( bool _gc_support, BRANCH_STRATEGY strat )
    {
        EvalRationalMap _rMap = getRationalModel();
        auto branch_var = mTableau.originalVars().begin();
        Rational ass_;
        Rational min_score;
        bool result = false, first_round = true, at_least_one = false;
        for( auto map_iterator = _rMap.begin(); map_iterator != _rMap.end(); ++map_iterator )
        {
            auto var = mTableau.originalVars().find( map_iterator->first );
            assert( var->first == map_iterator->first );
            Rational& ass = map_iterator->second; 
            if( var->first.getType() == carl::VariableType::VT_INT && !carl::isInteger( ass ) )
            {
                result = true;
                // Check whether we already have data about the considered variable
                // If so, determine its branching success according to the
                // heuristic branching function that we apply.
                // Otherwise, move on to the next variable.
                auto iter_succ = mBranch_Success.find( var->first );
                if( iter_succ == mBranch_Success.end() )
                {
                    continue;
                }
                at_least_one = true;
                unsigned sum_left = 0;
                auto iter_left = iter_succ->second.first.begin();
                while( iter_left != iter_succ->second.first.end() )
                {
                    sum_left += *iter_left;
                    ++iter_left;
                }
                unsigned sum_right = 0;
                auto iter_right = iter_succ->second.second.begin();
                while( iter_right != iter_succ->second.second.end() )
                {
                    sum_right += *iter_right;
                    ++iter_right;
                }
                Rational min_temp = 0;
                Rational score_left = 0;
                if( iter_succ->second.first.size() != 0 )
                {
                    score_left = Rational( sum_right )/Rational( iter_succ->second.first.size() );
                }   
                Rational score_right = 0;
                if( iter_succ->second.second.size() != 0 )
                {
                    score_right = Rational( sum_left )/Rational( iter_succ->second.second.size() );
                }
                if( score_left < score_right )
                {
                    if( score_left != 0 )
                    {
                        min_temp = score_left;
                    }
                    else
                    {
                        min_temp = score_right;                        
                    }
                }
                else
                {
                    if( score_right != 0 )
                    {
                        min_temp = score_right;
                    }
                    else
                    {
                        min_temp = score_left;
                    }
                }
                assert( min_temp != 0 );
                if( first_round )
                {
                    min_score = min_temp;  
                    branch_var = var;
                    ass_ = ass;
                }
                else
                {
                    if( min_temp < min_score )
                    {
                        min_score = min_temp;
                        branch_var = var;
                        ass_ = ass;
                    }
                }
            }
        }
        if( !at_least_one && result )
        {
            assert( strat != PSEUDO_COST );
            // If we don't have data about any of the violated variables but there
            // exist violated ones, apply the branching heuristic specified by strat
            if( strat == MIN_PIVOT )
            {
                result = minimal_row_var( _gc_support );            
            }
            else if( strat == MOST_FEASIBLE )
            {
                result = most_feasible_var( _gc_support );
            }
            else if( strat == MOST_INFEASIBLE )
            {
                result = most_infeasible_var( _gc_support );
            }
            else if( strat == NATIVE )
            {
                result = first_var( _gc_support );
            }
        }    
        if( result )
        {
            if( _gc_support )
            {
                result = maybeGomoryCut( branch_var->second, ass_ );
                if( result )
                    calculatePseudoCosts();
                return result;
            }
//            FormulasT premises;
//            mTableau.collect_premises( branch_var->second , premises  );
//            FormulasT premisesOrigins;
//            for( auto& pf : premises )
//            {
//                collectOrigins( pf, premisesOrigins );
//            }            
            branchAt( branch_var->second->expression(), true, ass_ );
            calculatePseudoCosts();
            return true;         
        }
        else
        {
            return false;
        } 
    }
    
    template<class Settings>
    bool LRAModule<Settings>::assignmentConsistentWithTableau( const EvalRationalMap& _assignment, const LRABoundType& _delta ) const
    {
        for( auto slackVar : mTableau.slackVars() )
        {
            Poly tmp = slackVar.first->substitute( _assignment );
            assert( tmp.isConstant() );
            LRABoundType slackVarAssignment = slackVar.second->assignment().mainPart() + slackVar.second->assignment().deltaPart() * _delta;
            if( !(tmp == Poly(Rational(slackVarAssignment))) )
            {
                return false;
            }
        }
        return true;
    }
    
    template<class Settings>
    bool LRAModule<Settings>::assignmentCorrect() const
    {
        if( solverState() == False ) return true;
        if( !mAssignmentFullfilsNonlinearConstraints ) return true;
        EvalRationalMap model = getRationalModel();
        for( auto ass = model.begin(); ass != model.end(); ++ass )
        {
            if( ass->first.getType() == carl::VariableType::VT_INT && !carl::isInteger( ass->second ) )
            {
                return false;
            }
        }
        for( auto iter = rReceivedFormula().begin(); iter != rReceivedFormula().end(); ++iter )
        {
            if( iter->formula().constraint().satisfiedBy( model ) != 1 )
            {
                assert( iter->formula().constraint().satisfiedBy( model ) == 0 );
                return false;
            }
        }
        return true;
    }
    
    #ifdef DEBUG_METHODS_LRA_MODULE

    template<class Settings>
    void LRAModule<Settings>::printLinearConstraints( ostream& _out, const string _init ) const
    {
        _out << _init << "Linear constraints:" << endl;
        for( auto iter = mLinearConstraints.begin(); iter != mLinearConstraints.end(); ++iter )
        {
            _out << _init << "   " << iter->toString() << endl;
        }
    }

    template<class Settings>
    void LRAModule<Settings>::printNonlinearConstraints( ostream& _out, const string _init ) const
    {
        _out << _init << "Nonlinear constraints:" << endl;
        for( auto iter = mNonlinearConstraints.begin(); iter != mNonlinearConstraints.end(); ++iter )
        {
            _out << _init << "   " << iter->toString() << endl;
        }
    }

    template<class Settings>
    void LRAModule<Settings>::printConstraintToBound( ostream& _out, const string _init ) const
    {
        _out << _init << "Mapping of constraints to bounds:" << endl;
        for( auto iter = mTableau.constraintToBound().begin(); iter != mTableau.constraintToBound().end(); ++iter )
        {
            _out << _init << "   " << iter->first.toString() << endl;
            for( auto iter2 = iter->second->begin(); iter2 != iter->second->end(); ++iter2 )
            {
                _out << _init << "        ";
                (*iter2)->print( true, cout, true );
                _out << endl;
            }
        }
    }

    template<class Settings>
    void LRAModule<Settings>::printBoundCandidatesToPass( ostream& _out, const string _init ) const
    {
        _out << _init << "Bound candidates to pass:" << endl;
        for( auto iter = mBoundCandidatesToPass.begin(); iter != mBoundCandidatesToPass.end(); ++iter )
        {
            _out << _init << "   ";
            (*iter)->print( true, cout, true );
            _out << " [" << (*iter)->pInfo()->updated << "]" << endl;
        }
    }

    template<class Settings>
    void LRAModule<Settings>::printRationalModel( ostream& _out, const string _init ) const
    {
        EvalRationalMap rmodel = getRationalModel();
        _out << _init << "Rational model:" << endl;
        for( auto assign = rmodel.begin(); assign != rmodel.end(); ++assign )
        {
            _out << _init;
            _out << setw(10) << assign->first;
            _out << " -> " << assign->second << endl;
        }
    }

    template<class Settings>
    void LRAModule<Settings>::printTableau( ostream& _out, const string _init ) const
    {
        mTableau.print( LAST_ENTRY_ID, _out, _init );
    }

    template<class Settings>
    void LRAModule<Settings>::printVariables( ostream& _out, const string _init ) const
    {
        mTableau.printVariables( true, _out, _init );
    }
    #endif
}    // namespace smtrat

#include "Instantiation.h"
