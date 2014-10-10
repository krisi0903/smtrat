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
 * @file Variable.h
 * @author Florian Corzilius <corzilius@cs.rwth-aachen.de>
 * @since 2012-04-05
 * @version 2014-10-01
 */

#pragma once

#define LRA_NO_DIVISION

#include "Bound.h"
#include "../../Common.h"
#include <sstream>
#include <iomanip>
#include <list>

namespace smtrat
{
    namespace lra
    {
        ///
        typedef size_t EntryID;
        ///
        static EntryID LAST_ENTRY_ID = 0;
        
        template<typename T1, typename T2>
        class Variable
        {
            private:
                ///
                bool mBasic;
                ///
                bool mOriginal;
                ///
                bool mInteger;
                ///
                EntryID mStartEntry;
                ///
                size_t mSize;
                ///
                double mConflictActivity;
                ///
                union
                {
                    ///
                    size_t mPosition;
                    ///
                    typename std::list<std::list<std::pair<Variable<T1,T2>*,T2>>>::iterator mPositionInNonActives;
                };
                ///
                typename Bound<T1, T2>::BoundSet mUpperbounds;
                ///
                typename Bound<T1, T2>::BoundSet mLowerbounds;
                ///
                const Bound<T1, T2>* mpSupremum;
                ///
                const Bound<T1, T2>* mpInfimum;
                ///
                const smtrat::Polynomial* mExpression;
                ///
                Value<T1> mAssignment;
                ///
                Value<T1> mLastConsistentAssignment;
                #ifdef LRA_NO_DIVISION
                ///
                T2 mFactor;
                #endif

            public:
                /**
                 * 
                 * @param _position
                 * @param _expression
                 * @param _defaultBoundPosition
                 * @param _isInteger
                 */
                Variable( size_t _position, const smtrat::Polynomial* _expression, ModuleInput::iterator _defaultBoundPosition, bool _isInteger );
                
                /**
                 * 
                 * @param _positionInNonActives
                 * @param _expression
                 * @param _defaultBoundPosition
                 * @param _isInteger
                 */
                Variable( typename std::list<std::list<std::pair<Variable<T1,T2>*,T2>>>::iterator _positionInNonActives, const smtrat::Polynomial* _expression, ModuleInput::iterator _defaultBoundPosition, bool _isInteger );
                
                /**
                 * 
                 */
                virtual ~Variable();

                /**
                 * @return 
                 */
                const Value<T1>& assignment() const
                {
                    return mAssignment;
                }

                /**
                 * @return 
                 */
                Value<T1>& rAssignment()
                {
                    return mAssignment;
                }
                
                /**
                 * 
                 */
                void resetAssignment()
                {
                    mAssignment = mLastConsistentAssignment;
                }
                
                /**
                 * 
                 */
                void storeAssignment()
                {
                    mLastConsistentAssignment = mAssignment;
                }

                /**
                 * 
                 * @param _basic
                 */
                void setBasic( bool _basic )
                {
                    mBasic = _basic;
                }

                /**
                 * @return 
                 */
                bool isBasic() const
                {
                    return mBasic;
                }

                /**
                 * @return 
                 */
                bool isOriginal() const
                {
                    return mOriginal;
                }

                /**
                 * @return 
                 */
                bool isInteger() const
                {
                    return mInteger;
                }
                
                /**
                 * @return 
                 */
                bool isActive() const
                {
                    return !(mpInfimum->isInfinite() && mpSupremum->isInfinite());
                }
                
                /**
                 * @return 
                 */
                bool involvesEquation() const
                {
                    return !mpInfimum->isInfinite() && mpInfimum->type() == Bound<T1,T2>::EQUAL;
                }
                
                /**
                 * @return 
                 */
                EntryID startEntry() const
                {
                    return mStartEntry;
                }
                
                /**
                 * @return 
                 */
                EntryID& rStartEntry()
                {
                    return mStartEntry;
                }
                
                /**
                 * @return 
                 */
                size_t size() const
                {
                    return mSize;
                }
                
                /**
                 * @return 
                 */
                size_t& rSize()
                {
                    return mSize;
                }
                
                /**
                 * @return 
                 */
                double conflictActivity() const
                {
                    return mConflictActivity;
                }
                
                /**
                 * 
                 * @param _supremum
                 */
                void setSupremum( const Bound<T1, T2>* _supremum )
                {
                    assert( _supremum->isActive() );
                    assert( mpSupremum->isActive() );
                    if( !mpSupremum->isInfinite() )
                        --mpSupremum->pInfo()->updated;
                    ++_supremum->pInfo()->updated;
                    mpSupremum = _supremum;
                }

                /**
                 * @return 
                 */
                const Bound<T1, T2>* pSupremum() const
                {
                    assert( !mpSupremum->origins().empty() );
                    return mpSupremum;
                }

                /**
                 * @return 
                 */
                const Bound<T1, T2>& supremum() const
                {
                    assert( !mpSupremum->origins().empty() );
                    return *mpSupremum;
                }

                /**
                 * 
                 * @param _infimum
                 */
                void setInfimum( const Bound<T1, T2>* _infimum )
                {
                    assert( _infimum->isActive() );
                    assert( mpInfimum->isActive() );
                    if( !mpInfimum->isInfinite() )
                        --mpInfimum->pInfo()->updated;
                    ++_infimum->pInfo()->updated;
                    mpInfimum = _infimum;
                    updateConflictActivity();
                }

                /**
                 * @return 
                 */
                const Bound<T1, T2>* pInfimum() const
                {
                    assert( !mpInfimum->origins().empty() );
                    return mpInfimum;
                }

                /**
                 * @return 
                 */
                const Bound<T1, T2>& infimum() const
                {
                    assert( !mpInfimum->origins().empty() );
                    return *mpInfimum;
                }

                /**
                 * @return 
                 */
                size_t position() const
                {
                    return mPosition;
                }
                
                /**
                 * @param _position
                 */
                void setPosition( size_t _position )
                {
                    mPosition = _position;
                }

                /**
                 * @return 
                 */
                typename std::list<std::list<std::pair<Variable<T1,T2>*,T2>>>::iterator positionInNonActives() const
                {
                    return mPositionInNonActives;
                }
                
                /**
                 * 
                 * @param _positionInNonActives
                 */
                void setPositionInNonActives( typename std::list<std::list<std::pair<Variable<T1,T2>*,T2>>>::iterator _positionInNonActives )
                {
                    mPositionInNonActives = _positionInNonActives;
                }

                /**
                 * @return 
                 */
                size_t rLowerBoundsSize()
                {
                    return mLowerbounds.size();
                }

                /**
                 * @return 
                 */
                size_t rUpperBoundsSize()
                {
                    return mUpperbounds.size();
                }

                /**
                 * @return 
                 */
                const typename Bound<T1, T2>::BoundSet& upperbounds() const
                {
                    return mUpperbounds;
                }

                /**
                 * @return 
                 */
                const typename Bound<T1, T2>::BoundSet& lowerbounds() const
                {
                    return mLowerbounds;
                }

                /**
                 * @return 
                 */
                typename Bound<T1, T2>::BoundSet& rUpperbounds()
                {
                    return mUpperbounds;
                }

                /**
                 * @return 
                 */
                typename Bound<T1, T2>::BoundSet& rLowerbounds()
                {
                    return mLowerbounds;
                }

                /**
                 * @return 
                 */
                size_t& rPosition()
                {
                    return mPosition;
                }

                /**
                 * @return 
                 */
                const smtrat::Polynomial* pExpression() const
                {
                    return mExpression;
                }

                /**
                 * @return 
                 */
                const smtrat::Polynomial& expression() const
                {
                    return *mExpression;
                }
                
                #ifdef LRA_NO_DIVISION
                /**
                 * @return 
                 */
                const T2& factor() const
                {
                    return mFactor;
                }
                
                /**
                 * @return 
                 */
                T2& rFactor()
                {
                    return mFactor;
                }
                #endif

                /**
                 * 
                 * @param _ass
                 * @return 
                 */
                unsigned isSatisfiedBy( const smtrat::EvalRationalMap& _ass ) const
                {
                    smtrat::Polynomial polyTmp = mExpression->substitute( _ass );
                    if( polyTmp.isConstant() )
                        return (*mpInfimum) <= polyTmp.constantPart() && (*mpSupremum) >= polyTmp.constantPart();
                    return 2;
                }

                /**
                 * 
                 */
                void updateConflictActivity()
                {
                    mConflictActivity = 0;
                    int counter = 0;
                    if( !mpInfimum->isInfinite() )
                    {
                        for( const Formula* form : mpInfimum->pOrigins()->front() )
                        {
                            mConflictActivity += form->activity();
                            ++counter;
                        }
                    }
                    if( !mpSupremum->isInfinite() )
                    {
                        for( const Formula* form : mpSupremum->pOrigins()->front() )
                        {
                            mConflictActivity += form->activity();
                            ++counter;
                        }
                    }
                    if( counter != 0 ) mConflictActivity /= counter;
                }

                /**
                 * 
                 * @param _val
                 * @param _position
                 * @param _constraint
                 * @param _deduced
                 * @return 
                 */
                std::pair<const Bound<T1, T2>*, bool> addUpperBound( Value<T1>* const _val, ModuleInput::iterator _position, const smtrat::Formula* _constraint = NULL, bool _deduced = false );
                
                /**
                 * 
                 * @param _val
                 * @param _position
                 * @param _constraint
                 * @param _deduced
                 * @return 
                 */
                std::pair<const Bound<T1, T2>*, bool> addLowerBound( Value<T1>* const _val, ModuleInput::iterator _position, const smtrat::Formula* _constraint = NULL, bool _deduced = false );
                
                /**
                 * 
                 * @param _val
                 * @param _position
                 * @param _constraint
                 * @return 
                 */
                std::pair<const Bound<T1, T2>*, bool> addEqualBound( Value<T1>* const _val, ModuleInput::iterator _position, const smtrat::Formula* _constraint = NULL );
                
                /**
                 * 
                 * @param bound
                 * @param _position
                 * @return 
                 */
                bool deactivateBound( const Bound<T1, T2>* bound, ModuleInput::iterator _position );
                
                /**
                 * 
                 * @return 
                 */
                Interval getVariableBounds() const;
                
                /**
                 * 
                 * @return 
                 */
                PointerSet<smtrat::Formula> getDefiningOrigins() const;

                /**
                 * 
                 * @param _out
                 */
                void print( std::ostream& _out = std::cout ) const;
                
                /**
                 * 
                 * @param _out
                 * @param _init
                 */
                void printAllBounds( std::ostream& _out = std::cout, const std::string _init = "" ) const;
        };
    }    // end namspace lra
} // end namespace smtrat

#include "Variable.tpp"