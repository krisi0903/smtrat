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
 * @file Tableau.hpp
 * @author Florian Corzilius <corzilius@cs.rwth-aachen.de>
 *
 * @version 2012-04-05
 * Created on November 14th, 2012
 */

#ifndef LRA_TABLEAU_H
#define LRA_TABLEAU_H

#include <vector>
#include <stack>
#include <map>
#include "Variable.hpp"

#define LRA_USE_PIVOTING_STRATEGY
#define LRA_REFINEMENT
//#define LRA_PRINT_STATS
//#define LRA_USE_OCCURENCE_STRATEGY
#ifndef LRA_USE_OCCURENCE_STRATEGY
#define LRA_USE_THETA_STRATEGY
#endif
#ifdef LRA_REFINEMENT
//#define LRA_INTRODUCE_NEW_CONSTRAINTS
#endif
//#define LRA_GOMORY_CUTS
#ifndef LRA_GOMORY_CUTS
//#define LRA_CUTS_FROM_PROOFS
#ifdef LRA_CUTS_FROM_PROOFS
//#define LRA_DEBUG_CUTS_FROM_PROOFS
#endif
#endif

namespace smtrat
{
    namespace lra
    {
        typedef size_t EntryID;
        static EntryID LAST_ENTRY_ID = 0;

        template<typename T>
        class TableauEntry
        {
            private:
                EntryID mUp;
                EntryID mDown;
                EntryID mLeft;
                EntryID mRight;
                size_t  mRowNumber;
                size_t  mColumnNumber;
                T       mpContent;

            public:
                TableauEntry():
                    mUp( LAST_ENTRY_ID ),
                    mDown( LAST_ENTRY_ID ),
                    mLeft( LAST_ENTRY_ID ),
                    mRight( LAST_ENTRY_ID ),
                    mRowNumber( 0 ),
                    mColumnNumber( 0 ),
                    mpContent()
                {}
                ;
                TableauEntry( EntryID _up,
                              EntryID _down,
                              EntryID _left,
                              EntryID _right,
                              size_t _rowNumber,
                              size_t _columnNumber,
                              const T& _content ):
                    mUp( _up ),
                    mDown( _down ),
                    mLeft( _left ),
                    mRight( _right ),
                    mRowNumber( _rowNumber ),
                    mColumnNumber( _columnNumber ),
                    mpContent( _content )
                {}
                ;
                TableauEntry( const TableauEntry& _entry ):
                    mUp( _entry.mUp ),
                    mDown( _entry.mDown ),
                    mLeft( _entry.mLeft ),
                    mRight( _entry.mRight ),
                    mRowNumber( _entry.mRowNumber ),
                    mColumnNumber( _entry.mColumnNumber ),
                    mpContent( _entry.mpContent )
                {}
                ;
                ~TableauEntry()
                {}
                ;

                EntryID up() const
                {
                    return mUp;
                }

                void setUp( const EntryID _up )
                {
                    mUp = _up;
                }

                EntryID down() const
                {
                    return mDown;
                }

                void setDown( const EntryID _down )
                {
                    mDown = _down;
                }

                EntryID left() const
                {
                    return mLeft;
                }

                void setLeft( const EntryID _left )
                {
                    mLeft = _left;
                }

                EntryID right() const
                {
                    return mRight;
                }

                void setRight( const EntryID _right )
                {
                    mRight = _right;
                }

                size_t rowNumber() const
                {
                    return mRowNumber;
                }

                void setRowNumber( size_t _rowNumber )
                {
                    mRowNumber = _rowNumber;
                }

                size_t columnNumber() const
                {
                    return mColumnNumber;
                }

                void setColumnNumber( size_t _columnNumber )
                {
                    mColumnNumber = _columnNumber;
                }

                const T& content() const
                {
                    return mpContent;
                }

                T& rContent()
                {
                    return mpContent;
                }
        };

        template <typename T1, typename T2>
        class Tableau
        {
            public:
                struct LearnedBound
                {
                    const Bound<T1, T2>*                newBound;
                    const Bound<T1, T2>*                nextWeakerBound;
                    std::vector< const Bound<T1, T2>*>* premise;
                };
                struct TableauHead
                {
                    EntryID           mStartEntry;
                    size_t            mSize;
                    Variable<T1, T2>* mName;
                    size_t            mActivity;
                };
            private:
                size_t                          mHeight;
                size_t                          mWidth;
                size_t                          mPivotingSteps;
                #ifdef LRA_USE_PIVOTING_STRATEGY
                size_t                          mRestarts;
                size_t                          mNextRestartBegin;
                size_t                          mNextRestartEnd;
                #endif
                smtrat::Formula::iterator       mDefaultBoundPosition;
                std::stack<EntryID>             mUnusedIDs;
                std::vector<TableauHead>        mRows;       // First element is the head of the row and the second the length of the row.
                std::vector<TableauHead>        mColumns;    // First element is the end of the column and the second the length of the column.
                std::set< size_t >              mActiveRows;
                std::vector<TableauEntry<T2> >* mpEntries;
                Value<T1>*                      mpTheta;
                #ifdef LRA_REFINEMENT
                std::map<Variable<T1, T2>*, LearnedBound> mLearnedLowerBounds;
                std::map<Variable<T1, T2>*, LearnedBound> mLearnedUpperBounds;
                std::vector<typename std::map<Variable<T1, T2>*, LearnedBound>::iterator> mNewLearnedBounds;
                #endif

                class Iterator
                {
                    private:
                        EntryID                   mEntryID;
                        std::vector<TableauEntry<T2> >* mpEntries;

                    public:
                        Iterator( EntryID _start, std::vector<TableauEntry<T2> >* const _entries ):
                            mEntryID( _start ),
                            mpEntries( _entries )
                        {}
                        ;
                        Iterator( const Iterator& _iter ):
                            mEntryID( _iter.entryID() ),
                            mpEntries( _iter.pEntries() )
                        {}
                        ;

                        EntryID entryID() const
                        {
                            return mEntryID;
                        }

                        TableauEntry<T2>& operator *()
                        {
                            return (*mpEntries)[mEntryID];
                        }

                        bool rowBegin() const
                        {
                            return (*mpEntries)[mEntryID].left() == LAST_ENTRY_ID;
                        }

                        bool rowEnd() const
                        {
                            return (*mpEntries)[mEntryID].right() == LAST_ENTRY_ID;
                        }

                        bool columnBegin() const
                        {
                            return (*mpEntries)[mEntryID].up() == LAST_ENTRY_ID;
                        }

                        bool columnEnd() const
                        {
                            return (*mpEntries)[mEntryID].down() == LAST_ENTRY_ID;
                        }

                        void up()
                        {
                            assert( !columnBegin() );
                            mEntryID = (*mpEntries)[mEntryID].up();
                        }

                        void down()
                        {
                            assert( !columnEnd() );
                            mEntryID = (*mpEntries)[mEntryID].down();
                        }

                        void left()
                        {
                            assert( !rowBegin() );
                            mEntryID = (*mpEntries)[mEntryID].left();
                        }

                        void right()
                        {
                            assert( !rowEnd() );
                            mEntryID = (*mpEntries)[mEntryID].right();
                        }

                        std::vector<TableauEntry<T2> >* pEntries() const
                        {
                            return mpEntries;
                        }

                        bool operator ==( const Iterator& _iter ) const
                        {
                            return mEntryID == _iter.entryID();
                        }

                        bool operator !=( const Iterator& _iter ) const
                        {
                            return mEntryID != _iter.entryID();
                        }
                };    /* class Tableau<T1,T2>::Iterator */

            public:
                Tableau( smtrat::Formula::iterator );
                ~Tableau();

                void setSize( size_t _expectedHeight, size_t _expectedWidth, size_t _expectedNumberOfBounds )
                {
                    mRows.reserve( _expectedHeight );
                    mColumns.reserve( _expectedWidth );
                    mpEntries->reserve( _expectedHeight*_expectedWidth+1 );
                    carl::reserve<T1>( 2*(_expectedNumberOfBounds+1) );
                    carl::reserve<T2>( _expectedHeight*_expectedWidth+1 );
                }
                
                size_t size() const
                {
                    return mpEntries->size();
                }

                #ifdef LRA_USE_PIVOTING_STRATEGY
                void setBlandsRuleStart( size_t _start )
                {
                    mNextRestartEnd = _start;
                }
                #endif

                const std::vector<TableauHead>& rows() const
                {
                    return mRows;
                }

                const std::vector<TableauHead>& columns() const
                {
                    return mColumns;
                }

                void incrementBasicActivity( const Variable<T1, T2>& _var )
                {
                    if( mRows[_var.position()].mActivity++ == 0 )
                    {
                        mActiveRows.insert( _var.position() );
                    }
                }

                void incrementNonbasicActivity( const Variable<T1, T2>& _var )
                {
                    ++mColumns[_var.position()].mActivity;
                }

                void decrementBasicActivity( const Variable<T1, T2>& _var )
                {
                    assert( mRows[_var.position()].mActivity != 0 );
                    if( --mRows[_var.position()].mActivity == 0 )
                    {
                        mActiveRows.erase( _var.position() );
                    }
                }

                void decrementNonbasicActivity( const Variable<T1, T2>& _var )
                {
                    assert( mColumns[_var.position()].mActivity != 0 );
                    --mColumns[_var.position()].mActivity;
                }

                size_t numberOfPivotingSteps() const
                {
                    return mPivotingSteps;
                }
                
                void resetNumberOfPivotingSteps() 
                {
                    mPivotingSteps = 0;
                }
                
                size_t numberOfRestarts() const
                {
                    return mRestarts;
                }

                #ifdef LRA_REFINEMENT
                std::map<Variable<T1, T2>*, LearnedBound>& rLearnedLowerBounds()
                {
                    return mLearnedLowerBounds;
                }

                std::map<Variable<T1, T2>*, LearnedBound>& rLearnedUpperBounds()
                {
                    return mLearnedUpperBounds;
                }
                
                std::vector<typename std::map<Variable<T1, T2>*, LearnedBound>::iterator>& rNewLearnedBounds()
                {
                    return mNewLearnedBounds;
                }
                #endif

                smtrat::Formula::const_iterator defaultBoundPosition() const
                {
                    return mDefaultBoundPosition;
                }

                EntryID newTableauEntry( const T2& );
                void removeEntry( EntryID );
                Variable<T1, T2>* newNonbasicVariable( const smtrat::Polynomial* );
                Variable<T1, T2>* newBasicVariable( const smtrat::Polynomial*, const std::vector<Variable<T1, T2>*>&, std::vector<T2>& );
                std::pair<EntryID, bool> nextPivotingElement();
                std::pair<EntryID, bool> isSuitable( EntryID, Value<T1>& ) const;
                bool betterEntry( EntryID, EntryID ) const;
                std::vector< const Bound<T1, T2>* > getConflict( EntryID ) const;
                std::vector< std::set< const Bound<T1, T2>* > > getConflictsFrom( EntryID ) const;
                void updateBasicAssignments( size_t, const Value<T1>& );
                void pivot( EntryID );
                void updateDownwards( EntryID, std::vector<Iterator>&, std::vector<Iterator>& );
                void updateUpwards( EntryID, std::vector<Iterator>&, std::vector<Iterator>& );
                #ifdef LRA_REFINEMENT
                void rowRefinement( const TableauHead& );
                #endif
                size_t checkCorrectness() const;
                bool rowCorrect( size_t _rowNumber ) const;
                #ifdef LRA_CUTS_FROM_PROOFS
                bool isDefining( size_t, std::vector<size_t>&, std::vector<T2>&, T2&, T2& ) const;
                bool isDefining_Easy( std::vector<size_t>&, size_t );
                bool isDiagonal( size_t, std::vector<size_t>& );
                size_t position_DC( size_t, std::vector<size_t>& );
                size_t revert_diagonals( size_t, std::vector<size_t>& );
                void invertColumn( size_t );
                void addColumns( size_t, size_t, T2 );
                void multiplyRow( size_t, T2 );
                T2 Scalar_Product( Tableau<T2>&, Tableau<T2>&, size_t, size_t, T, std::vector<size_t>&, std::vector<size_t>& );
                void calculate_hermite_normalform( std::vector<size_t>& );
                void invert_HNF_Matrix( std::vector<size_t> );
                bool create_cut_from_proof( Tableau<T2>&, Tableau<T2>&, size_t&, T2&, std::vector<T2>&, std::vector<bool>&, smtrat::Polynomial&, std::vector<size_t>&, std::vector<size_t>&, Bound<T1, T2>*&);
                #endif
                #ifdef LRA_GOMORY_CUTS
                const smtrat::Constraint* gomoryCut( const T2&, size_t, std::vector<const smtrat::Constraint*>& );
                #endif
                void printHeap( std::ostream& = std::cout, int = 30, const std::string = "" ) const;
                void printEntry( EntryID, std::ostream& = std::cout, int = 20 ) const;
                void printVariables( bool = true, std::ostream& = std::cout, const std::string = "" ) const;
                #ifdef LRA_REFINEMENT
                void printLearnedBounds( const std::string = "", std::ostream& = std::cout ) const;
                #endif
                void print( std::ostream& = std::cout, int = 28, const std::string = "" ) const;

        };

        template<typename T1, typename T2>
        Tableau<T1,T2>::Tableau( smtrat::Formula::iterator _defaultBoundPosition ):
            mHeight( 0 ),
            mWidth( 0 ),
            mPivotingSteps( 0 ),
            #ifdef LRA_USE_PIVOTING_STRATEGY
            mRestarts( 0 ),
            mNextRestartBegin( 0 ),
            mNextRestartEnd( 0 ),
            #endif
            mDefaultBoundPosition( _defaultBoundPosition ),
            mUnusedIDs(),
            mRows(),
            mColumns(),
            mActiveRows()
            #ifdef LRA_REFINEMENT
            ,
            mLearnedLowerBounds(),
            mLearnedUpperBounds(),
            mNewLearnedBounds()
            #endif
        {
            mpEntries = new std::vector< TableauEntry<T2> >();
            mpEntries->push_back( TableauEntry<T2>() );
            mpTheta = new Value<T1>();
        };

        template<typename T1, typename T2>
        Tableau<T1,T2>::~Tableau()
        {
            #ifdef LRA_PRINT_STATS
            std::cout << "#Pivoting steps:  " << mPivotingSteps << std::endl;
            std::cout << "#Tableus entries: " << mpEntries->size()-1 << std::endl;
            std::cout << "Tableau coverage: " << (double)(mpEntries->size()-1)/(double)(mRows.size()*mColumns.size())*100 << "%" << std::endl;
            #endif
            while( !mRows.empty() )
            {
                Variable<T1, T2>* varToDel = mRows.back().mName;
                mRows.pop_back();
                delete varToDel;
            }
            while( !mColumns.empty() )
            {
                Variable<T1, T2>* varToDel = mColumns.back().mName;
                mColumns.pop_back();
                delete varToDel;
            }
            while( !mUnusedIDs.empty() )
            {
                mUnusedIDs.pop();
            }
            delete mpEntries;
            delete mpTheta;
        };

        /**
         *
         * @return
         */
        template<typename T1, typename T2>
        EntryID Tableau<T1,T2>::newTableauEntry( const T2& _content )
        {
            if( mUnusedIDs.empty() )
            {
                mpEntries->push_back( TableauEntry<T2>( LAST_ENTRY_ID, LAST_ENTRY_ID, LAST_ENTRY_ID, LAST_ENTRY_ID, 0, 0, _content ) );
                return ( ( mpEntries->size() ) - 1);
            }
            else
            {
                EntryID id = mUnusedIDs.top();
                mUnusedIDs.pop();
                (*mpEntries)[id].rContent() = _content;
                return id;
            }
        }

        /**
         *
         * @param _entryID
         */
        template<typename T1, typename T2>
        void Tableau<T1,T2>::removeEntry( EntryID _entryID )
        {
            TableauEntry<T2>& entry = (*mpEntries)[_entryID];
            TableauHead& rowHead = mRows[entry.rowNumber()];
            TableauHead& columnHead = mColumns[entry.columnNumber()];
            const EntryID& up = entry.up();
            const EntryID& down = entry.down();
            if( up != LAST_ENTRY_ID )
            {
                (*mpEntries)[up].setDown( down );
            }
            if( down != LAST_ENTRY_ID )
            {
                (*mpEntries)[down].setUp( up );
            }
            else
            {
                columnHead.mStartEntry = up;
            }
            const EntryID& left = entry.left();
            const EntryID& right = entry.right();
            if( left != LAST_ENTRY_ID )
            {
                (*mpEntries)[left].setRight( right );
            }
            else
            {
                rowHead.mStartEntry = right;
            }
            if( right != LAST_ENTRY_ID )
            {
                (*mpEntries)[right].setLeft( left );
            }
            --rowHead.mSize;
            --columnHead.mSize;
            mUnusedIDs.push( _entryID );
        }

        /**
         *
         * @param _ex
         * @return
         */
        template<typename T1, typename T2>
        Variable<T1, T2>* Tableau<T1,T2>::newNonbasicVariable( const smtrat::Polynomial* _poly )
        {
            Variable<T1, T2>* var = new Variable<T1, T2>( mWidth++, false, _poly, mDefaultBoundPosition );
            mColumns.push_back( TableauHead() );
            mColumns[mWidth-1].mStartEntry = LAST_ENTRY_ID;
            mColumns[mWidth-1].mSize = 0;
            mColumns[mWidth-1].mName = var;
            return var;
        }

        /**
         *
         * @param _poly
         * @param _nonbasicVariables
         * @param _coefficients
         * @return
         */
        template<typename T1, typename T2>
        Variable<T1, T2>* Tableau<T1,T2>::newBasicVariable( const smtrat::Polynomial* _poly, const std::vector< Variable<T1, T2>* >& _nonbasicVariables, std::vector< T2 >& _coefficients )
        {
            assert( _coefficients.size() == _coefficients.size() );
            Variable<T1, T2>* var = new Variable<T1, T2>( mHeight++, true, _poly, mDefaultBoundPosition );
            mRows.push_back( TableauHead() );
            EntryID currentStartEntryOfRow = LAST_ENTRY_ID;
            typename std::vector< Variable<T1, T2>* >::const_iterator basicVar = _nonbasicVariables.begin();
            typename std::vector< T2 >::iterator coeff = _coefficients.begin();
            while( basicVar != _nonbasicVariables.end() )
            {
                EntryID entryID = newTableauEntry( *coeff );
                TableauEntry<T2>& entry = (*mpEntries)[entryID];
                // Fix the position.
                entry.setColumnNumber( (*basicVar)->position() );
                entry.setRowNumber( mHeight-1 );
                TableauHead& columnHead = mColumns[entry.columnNumber()];
                EntryID& columnStart = columnHead.mStartEntry;
                // Set it as column end.
                if( columnStart != LAST_ENTRY_ID )
                {
                    (*mpEntries)[columnStart].setDown( entryID );
                }
                entry.setUp( columnStart );
                columnStart = entryID;
                ++columnHead.mSize;
                entry.setDown( LAST_ENTRY_ID );
                // Put it in the row.
                if( currentStartEntryOfRow == LAST_ENTRY_ID )
                {
                    currentStartEntryOfRow = entryID;
                }
                else
                {
                    Iterator rowIter = Iterator( currentStartEntryOfRow, mpEntries );
                    while( !rowIter.rowEnd() && (*rowIter).columnNumber() < entry.columnNumber() )
                    {
                        rowIter.right();
                    }
                    assert( (*rowIter).columnNumber() !=  entry.columnNumber() );
                    if( (*rowIter).columnNumber() > entry.columnNumber() )
                    {
                        // Entry horizontally between two entries.
                        EntryID leftEntryID = (*rowIter).left();
                        if( leftEntryID != LAST_ENTRY_ID )
                        {
                            (*mpEntries)[leftEntryID].setRight( entryID );
                        }
                        (*rowIter).setLeft( entryID );
                        entry.setLeft( leftEntryID );
                        entry.setRight( rowIter.entryID() );
                        if( leftEntryID == LAST_ENTRY_ID )
                        {
                            currentStartEntryOfRow = entryID;
                        }
                    }
                    else
                    {
                        // Entry will be the rightmost in this row.
                        (*rowIter).setRight( entryID );
                        entry.setLeft( rowIter.entryID() );
                        entry.setRight( LAST_ENTRY_ID );
                    }
                }
                ++basicVar;
                ++coeff;
            }
            TableauHead& rowHead = mRows[mHeight-1];
            rowHead.mStartEntry = currentStartEntryOfRow;
            rowHead.mSize = _nonbasicVariables.size();
            rowHead.mName = var;
            return var;
        }

        #ifdef LRA_USE_PIVOTING_STRATEGY
    //    /**
    //     *
    //     * @param y
    //     * @param x
    //     * @return
    //     */
    //    template<typename T1, typename T2>
    //    static unsigned luby( unsigned _numberOfRestarts )
    //    {
    //        // Find the finite subsequence that contains index 'x', and the
    //        // size of that subsequence:
    //        std::cout << "_numberOfRestarts = " << _numberOfRestarts;
    //        unsigned size, seq;
    //        for( size = 1, seq = 0; size < _numberOfRestarts + 1; seq++, size = 2 * size + 1 );
    //
    //        while( size - 1 != _numberOfRestarts )
    //        {
    //            size = (size - 1) >> 1;
    //            seq--;
    //            _numberOfRestarts = _numberOfRestarts % size;
    //        }
    //        std::cout << " results in seq = " << seq << std::endl;
    //        if( seq >= 64 ) return 0;
    //        std::cout << " results in seq = " << seq << std::endl;
    //        unsigned result = 1;
    //        result = result << seq;
    //        std::cout << "result = " << result << std::endl;
    //        return result;
    //    }
        #endif

        /**
         *
         * @return
         */
        template<typename T1, typename T2>
        std::pair<EntryID,bool> Tableau<T1,T2>::nextPivotingElement()
        {
            #ifdef LRA_USE_PIVOTING_STRATEGY
            //  Dynamic strategy for a fixed number of steps
    //        if( mPivotingSteps >= mNextRestartBegin && mPivotingSteps < mNextRestartEnd )
            if( mPivotingSteps < mNextRestartEnd )
            {
                #ifdef LRA_USE_OCCURENCE_STRATEGY
                size_t smallestRowSize = mWidth;
                size_t smallestColumnSize = mHeight;
                #endif
                EntryID beginOfBestRow = LAST_ENTRY_ID;
                EntryID beginOfFirstConflictRow = LAST_ENTRY_ID;
                *mpTheta = Value<T1>( 0 );
                Value<T1> conflictTheta =  Value<T1>( 0 );
                for( auto rowNumber = mActiveRows.begin(); rowNumber != mActiveRows.end(); ++rowNumber )
                {
                    Value<T1> theta = Value<T1>();
                    std::pair<EntryID,bool> result = isSuitable( *rowNumber, theta );
                    if( !result.second )
                    {
                        // Found a conflicting row.
                        if( beginOfFirstConflictRow == LAST_ENTRY_ID || theta.mainPart() > conflictTheta.mainPart() )
                        {
                            conflictTheta = theta;
                            beginOfFirstConflictRow = result.first;
                        }
                    }
                    else if( result.first != LAST_ENTRY_ID )
                    {
                        #ifdef LRA_USE_THETA_STRATEGY
                        if( beginOfBestRow == LAST_ENTRY_ID || carl::abs( theta.mainPart() ) > carl::abs( mpTheta->mainPart() ) )
                        {
                            beginOfBestRow = result.first;
                            *mpTheta = theta;
                        }
                        #endif
                        #ifdef LRA_USE_OCCURENCE_STRATEGY
                        if( mRows[(*mpEntries)[result.first].rowNumber()].mSize < smallestRowSize )
                        {
                            // Found a better pivoting element.
                            smallestRowSize = mRows[(*mpEntries)[result.first].rowNumber()].mSize;
                            smallestColumnSize = mColumns[(*mpEntries)[result.first].columnNumber()].mSize;
                            beginOfBestRow = result.first;
                            *mpTheta = theta;
                        }
                        else if( mRows[(*mpEntries)[result.first].rowNumber()].mSize == smallestRowSize
                                 && mColumns[(*mpEntries)[result.first].columnNumber()].mSize < smallestColumnSize )
                        {
                            // Found a better pivoting element.
                            smallestColumnSize = mColumns[(*mpEntries)[result.first].columnNumber()].mSize;
                            beginOfBestRow = result.first;
                            *mpTheta = theta;
                        }
                        #endif
                    }
                }
                if( beginOfBestRow == LAST_ENTRY_ID && beginOfFirstConflictRow != LAST_ENTRY_ID )
                {
                    // Found a conflict
                    return std::pair<EntryID,bool>( beginOfFirstConflictRow, false );
                }
                else if( beginOfBestRow != LAST_ENTRY_ID )
                {
                    // The best pivoting element found
                    return std::pair<EntryID,bool>( beginOfBestRow, true );
                }
                else
                {
                    // Found no pivoting element, that is no variable violates its bounds.
                    return std::pair<EntryID,bool>( LAST_ENTRY_ID, true );
                }
            }
            // Bland's rule
            else
            {
    //            if( mPivotingSteps == mNextRestartEnd )
    //            {
    //                mNextRestartBegin = mNextRestartEnd + mWidth * luby( mRestarts++ );
    //                mNextRestartEnd = mNextRestartBegin + mWidth;
    //                std::cout << "Next restart range = [" << mNextRestartBegin << "," << mNextRestartEnd << "]" << std::endl;
    //            }
            #endif
                for( auto rowNumber = mActiveRows.begin(); rowNumber != mActiveRows.end(); ++rowNumber )
                {
                    std::pair<EntryID,bool> result = isSuitable( *rowNumber, *mpTheta );
                    if( !result.second )
                    {
                        // Found a conflicting row.
                        return std::pair<EntryID,bool>( result.first, false );
                    }
                    else if( result.first != LAST_ENTRY_ID )
                    {
                        // Found a pivoting element
                        return std::pair<EntryID,bool>( result.first, true );
                    }
                }
                // Found no pivoting element, that is no variable violates its bounds.
                return std::pair<EntryID,bool>( LAST_ENTRY_ID, true );
            #ifdef LRA_USE_PIVOTING_STRATEGY
            }
            #endif
        }

        /**
         *
         * @param _rowNumber
         * @return
         */
        template<typename T1, typename T2>
        std::pair<EntryID,bool> Tableau<T1,T2>::isSuitable( size_t _rowNumber, Value<T1>& _theta ) const
        {
            EntryID bestEntry = LAST_ENTRY_ID;
            const TableauHead& _rowHead = mRows[_rowNumber];
            const Variable<T1, T2>& basicVar = *_rowHead.mName;
            const Bound<T1, T2>& basicVarSupremum = basicVar.supremum();
            const Value<T1>& basicVarAssignment = basicVar.assignment();
            const Bound<T1, T2>& basicVarInfimum = basicVar.infimum();
            const EntryID& rowStartEntry = _rowHead.mStartEntry;
            // Upper bound is violated
            if( basicVarSupremum < basicVarAssignment )
            {
                // Check all entries in the row / nonbasic variables
                Iterator rowIter = Iterator( rowStartEntry, mpEntries );
                while( true )
                {
                    const Variable<T1, T2>& nonBasicVar = *mColumns[(*rowIter).columnNumber()].mName;
                    #ifdef LRA_NO_DIVISION
                    if( ((*rowIter).content() < 0 && basicVar.factor() > 0) || ((*rowIter).content() > 0 && basicVar.factor() < 0) )
                    #else
                    if( (*rowIter).content() < 0 )
                    #endif
                    {
                        if( nonBasicVar.supremum() > nonBasicVar.assignment() )
                        {
                            // Nonbasic variable suitable
                            assert( (*rowIter).content() != 0 );
                            if( betterEntry( rowIter.entryID(), bestEntry ) )
                            {
                                #ifdef LRA_NO_DIVISION
                                _theta = ((basicVarSupremum.limit() - basicVarAssignment)*basicVar.factor())/(*rowIter).content();
                                #else
                                _theta = (basicVarSupremum.limit() - basicVarAssignment)/(*rowIter).content();
                                #endif
                                bestEntry = rowIter.entryID();
                            }
                        }
                    }
                    else
                    {
                        if( nonBasicVar.infimum() < nonBasicVar.assignment()  )
                        {
                            // Nonbasic variable suitable
                            assert( (*rowIter).content() != 0 );
                            if( betterEntry( rowIter.entryID(), bestEntry ) )
                            {
                                #ifdef LRA_NO_DIVISION
                                _theta = ((basicVarSupremum.limit() - basicVarAssignment)*basicVar.factor())/(*rowIter).content();
                                #else
                                _theta = (basicVarSupremum.limit() - basicVarAssignment)/(*rowIter).content();
                                #endif
                                bestEntry = rowIter.entryID();
                            }
                        }
                    }
                    if( rowIter.rowEnd() )
                    {
                        if( bestEntry == LAST_ENTRY_ID )
                        {
                            _theta = basicVarAssignment - basicVarSupremum.limit();
                            return std::pair<EntryID,bool>( rowStartEntry, false );
                        }
                        break;
                    }
                    else
                    {
                        rowIter.right();
                    }
                }
            }
            // Lower bound is violated
            else if( basicVarInfimum > basicVarAssignment )
            {
                // Check all entries in the row / nonbasic variables
                Iterator rowIter = Iterator( rowStartEntry, mpEntries );
                while( true )
                {
                    const Variable<T1, T2>& nonBasicVar = *mColumns[(*rowIter).columnNumber()].mName;
                    #ifdef LRA_NO_DIVISION
                    if( ((*rowIter).content() > 0 && basicVar.factor() > 0) || ((*rowIter).content() < 0 && basicVar.factor() < 0) )
                    #else
                    if( (*rowIter).content() > 0 )
                    #endif
                    {
                        if( nonBasicVar.supremum() > nonBasicVar.assignment() )
                        {
                            // Nonbasic variable suitable
                            assert( (*rowIter).content() != 0 );
                            if( betterEntry( rowIter.entryID(), bestEntry ) )
                            {
                                #ifdef LRA_NO_DIVISION
                                _theta = ((basicVarInfimum.limit() - basicVarAssignment)*basicVar.factor())/(*rowIter).content();
                                #else
                                _theta = (basicVarInfimum.limit() - basicVarAssignment)/(*rowIter).content();
                                #endif
                                bestEntry = rowIter.entryID();
                            }
                        }
                    }
                    else
                    {
                        if( nonBasicVar.infimum() < nonBasicVar.assignment() )
                        {
                            // Nonbasic variable suitable
                            assert( (*rowIter).content() != 0 );
                            if( betterEntry( rowIter.entryID(), bestEntry ) )
                            {
                                #ifdef LRA_NO_DIVISION
                                _theta = ((basicVarInfimum.limit() - basicVarAssignment)*basicVar.factor())/(*rowIter).content();
                                #else
                                _theta = (basicVarInfimum.limit() - basicVarAssignment)/(*rowIter).content();
                                #endif
                                bestEntry = rowIter.entryID();
                            }
                        }
                    }
                    if( rowIter.rowEnd() )
                    {
                        if( bestEntry == LAST_ENTRY_ID )
                        {
                            _theta = basicVarInfimum.limit() - basicVarAssignment;
                            return std::pair<EntryID,bool>( rowStartEntry, false );
                        }
                        break;
                    }
                    else
                    {
                        rowIter.right();
                    }
                }
            }
            return std::pair<EntryID,bool>( bestEntry, true );
        }

        template<typename T1, typename T2>
        bool Tableau<T1,T2>::betterEntry( EntryID _isBetter, EntryID _than ) const
        {
            assert( _isBetter != LAST_ENTRY_ID );
            if( _than == LAST_ENTRY_ID ) return true;
            const TableauHead& isBetterColumn = mColumns[(*mpEntries)[_isBetter].columnNumber()];
            const TableauHead& thanColumn = mColumns[(*mpEntries)[_than].columnNumber()];
            if( isBetterColumn.mActivity < thanColumn.mActivity ) return true;
            else if( isBetterColumn.mActivity == thanColumn.mActivity )
            {
                if( isBetterColumn.mSize < thanColumn.mSize ) return true;
            }
            return false;
        }

        /**
         *
         * @param _startRow
         * @return
         */
        template<typename T1, typename T2>
        std::vector< const Bound<T1, T2>* > Tableau<T1,T2>::getConflict( EntryID _rowEntry ) const
        {
            assert( _rowEntry != LAST_ENTRY_ID );
            const TableauHead& row = mRows[(*mpEntries)[_rowEntry].rowNumber()];
            // Upper bound is violated
            std::vector< const Bound<T1, T2>* > conflict = std::vector< const Bound<T1, T2>* >();
            if( row.mName->supremum() < row.mName->assignment() )
            {
                conflict.push_back( row.mName->pSupremum() );
                // Check all entries in the row / basic variables
                Iterator rowIter = Iterator( row.mStartEntry, mpEntries );
                while( true )
                {
                    #ifdef LRA_NO_DIVISION
                    if( ((*rowIter).content() < 0 && row.mName->factor() > 0) || ((*rowIter).content() > 0 && row.mName->factor() < 0) )
                    #else
                    if( (*rowIter).content() < 0 )
                    #endif
                    {
                        assert( !(mColumns[(*rowIter).columnNumber()].mName->supremum() > mColumns[(*rowIter).columnNumber()].mName->assignment()) );
                        conflict.push_back( mColumns[(*rowIter).columnNumber()].mName->pSupremum() );
                    }
                    else
                    {
                        assert( !(mColumns[(*rowIter).columnNumber()].mName->infimum() < mColumns[(*rowIter).columnNumber()].mName->assignment()) );
                        conflict.push_back( mColumns[(*rowIter).columnNumber()].mName->pInfimum() );
                    }
                    if( rowIter.rowEnd() )
                    {
                        break;
                    }
                    else
                    {
                        rowIter.right();
                    }
                }
            }
            // Lower bound is violated
            else
            {
                assert( row.mName->infimum() > row.mName->assignment() );
                conflict.push_back( row.mName->pInfimum() );
                // Check all entries in the row / basic variables
                Iterator rowIter = Iterator( row.mStartEntry, mpEntries );
                while( true )
                {
                    #ifdef LRA_NO_DIVISION
                    if( ((*rowIter).content() > 0 && row.mName->factor() > 0) || ((*rowIter).content() < 0 && row.mName->factor() < 0) )
                    #else
                    if( (*rowIter).content() > 0 )
                    #endif
                    {
                        assert( !(mColumns[(*rowIter).columnNumber()].mName->supremum() > mColumns[(*rowIter).columnNumber()].mName->assignment()) );
                        conflict.push_back( mColumns[(*rowIter).columnNumber()].mName->pSupremum() );
                    }
                    else
                    {
                        assert( !(mColumns[(*rowIter).columnNumber()].mName->infimum() < mColumns[(*rowIter).columnNumber()].mName->assignment()) );
                        conflict.push_back( mColumns[(*rowIter).columnNumber()].mName->pInfimum() );
                    }
                    if( rowIter.rowEnd() )
                    {
                        break;
                    }
                    else
                    {
                        rowIter.right();
                    }
                }
            }
            return conflict;
        }

        /**
         *
         * @param _startRow
         * @return
         */
        template<typename T1, typename T2>
        std::vector< std::set< const Bound<T1, T2>* > > Tableau<T1,T2>::getConflictsFrom( EntryID _rowEntry ) const
        {
            std::vector< std::set< const Bound<T1, T2>* > > conflicts = std::vector< std::set< const Bound<T1, T2>* > >();
            for( size_t rowNumber = (*mpEntries)[_rowEntry].rowNumber(); rowNumber < mRows.size(); ++rowNumber )
            {
                // Upper bound is violated
                if( mRows[rowNumber].mName->supremum() < mRows[rowNumber].mName->assignment() )
                {
                    conflicts.push_back( std::set< const Bound<T1, T2>* >() );
                    conflicts.back().insert( mRows[rowNumber].mName->pSupremum() );
                    // Check all entries in the row / basic variables
                    Iterator rowIter = Iterator( mRows[rowNumber].mStartEntry, mpEntries );
                    while( true )
                    {
                        #ifdef LRA_NO_DIVISION
                        if( ( (*rowIter).content() < 0 && mRows[rowNumber].mName->factor() > 0) || ((*rowIter).content() > 0 && mRows[rowNumber].mName->factor() < 0) )
                        #else
                        if( (*rowIter).content() < 0 )
                        #endif
                        {
                            if( mColumns[(*rowIter).columnNumber()].mName->supremum() > mColumns[(*rowIter).columnNumber()].mName->assignment() )
                            {
                                // Not a conflict.
                                conflicts.pop_back();
                                break;
                            }
                            else
                            {
                                conflicts.back().insert( mColumns[(*rowIter).columnNumber()].mName->pSupremum() );
                            }
                        }
                        else
                        {
                            if( mColumns[(*rowIter).columnNumber()].mName->infimum() < mColumns[(*rowIter).columnNumber()].mName->assignment() )
                            {
                                // Not a conflict.
                                conflicts.pop_back();
                                break;
                            }
                            else
                            {
                                conflicts.back().insert( mColumns[(*rowIter).columnNumber()].mName->pInfimum() );
                            }
                        }
                        if( rowIter.rowEnd() )
                        {
                            break;
                        }
                        else
                        {
                            rowIter.right();
                        }
                    }
                }
                // Lower bound is violated
                else if( mRows[rowNumber].mName->infimum() > mRows[rowNumber].mName->assignment() )
                {
                    conflicts.push_back( std::set< const Bound<T1, T2>* >() );
                    conflicts.back().insert( mRows[rowNumber].mName->pInfimum() );
                    // Check all entries in the row / basic variables
                    Iterator rowIter = Iterator( mRows[rowNumber].mStartEntry, mpEntries );
                    while( true )
                    {
                        #ifdef LRA_NO_DIVISION
                        if( ((*rowIter).content() > 0 && mRows[rowNumber].mName->factor() > 0) || ((*rowIter).content() < 0 && mRows[rowNumber].mName->factor() < 0) )
                        #else
                        if( (*rowIter).content() > 0 )
                        #endif
                        {
                            if( mColumns[(*rowIter).columnNumber()].mName->supremum() > mColumns[(*rowIter).columnNumber()].mName->assignment()  )
                            {
                                // Not a conflict.
                                conflicts.pop_back();
                                break;
                            }
                            else
                            {
                                conflicts.back().insert( mColumns[(*rowIter).columnNumber()].mName->pSupremum() );
                            }
                        }
                        else
                        {
                            if( mColumns[(*rowIter).columnNumber()].mName->infimum() < mColumns[(*rowIter).columnNumber()].mName->assignment()  )
                            {
                                // Not a conflict.
                                conflicts.pop_back();
                                break;
                            }
                            else
                            {
                                conflicts.back().insert( mColumns[(*rowIter).columnNumber()].mName->pInfimum() );
                            }
                        }
                        if( rowIter.rowEnd() )
                        {
                            break;
                        }
                        else
                        {
                            rowIter.right();
                        }
                    }
                }
            }
            return conflicts;
        }

        /**
         *
         * @param _column
         * @param _change
         */
        template<typename T1, typename T2>
        void Tableau<T1,T2>::updateBasicAssignments( size_t _column, const Value<T1>& _change )
        {
            if( mColumns[_column].mSize > 0 )
            {
                Iterator columnIter = Iterator( mColumns[_column].mStartEntry, mpEntries );
                while( true )
                {
                    Variable<T1, T2>& basic = *mRows[(*columnIter).rowNumber()].mName;
                    #ifdef LRA_NO_DIVISION
                    basic.rAssignment() += (_change * (*columnIter).content())/basic.factor();
                    #else
                    basic.rAssignment() += (_change * (*columnIter).content());
                    #endif
                    if( columnIter.columnBegin() )
                    {
                        break;
                    }
                    else
                    {
                        columnIter.up();
                    }
                }
            }
        }

        /**
         *
         * @param _pivotingElement
         */
        template<typename T1, typename T2>
        void Tableau<T1,T2>::pivot( EntryID _pivotingElement )
        {
            // TODO: refine the pivoting row
            // Find all columns having "a nonzero entry in the pivoting row"**, update this entry and store it.
            // First the column with ** left to the pivoting column until the leftmost column with **.
            std::vector<Iterator> pivotingRowLeftSide = std::vector<Iterator>();
            TableauEntry<T2>& pivotEntry = (*mpEntries)[_pivotingElement];
            T2& pivotContent = pivotEntry.rContent();
            Iterator iterTemp = Iterator( _pivotingElement, mpEntries );
            while( !iterTemp.rowBegin() )
            {
                iterTemp.left();
                #ifdef LRA_NO_DIVISION
                (*iterTemp).rContent() = -(*iterTemp).content();
                #else
                (*iterTemp).rContent() /= -pivotContent;
                #endif
                pivotingRowLeftSide.push_back( iterTemp );
            }
            // Then the column with ** right to the pivoting column until the rightmost column with **.
            std::vector<Iterator> pivotingRowRightSide = std::vector<Iterator>();
            iterTemp = Iterator( _pivotingElement, mpEntries );
            while( !iterTemp.rowEnd() )
            {
                iterTemp.right();
                #ifdef LRA_NO_DIVISION
                (*iterTemp).rContent() = -(*iterTemp).content();
                #else
                (*iterTemp).rContent() /= -pivotContent;
                #endif
                pivotingRowRightSide.push_back( iterTemp );
            }
            TableauHead& rowHead = mRows[pivotEntry.rowNumber()];
            TableauHead& columnHead = mColumns[pivotEntry.columnNumber()];
            Variable<T1, T2>* nameTmp = rowHead.mName;
            // Update the assignments of the pivoting variables
            #ifdef LRA_NO_DIVISION
            nameTmp->rAssignment() += ((*mpTheta) * pivotContent) / nameTmp->factor();
            #else
            nameTmp->rAssignment() += (*mpTheta) * pivotContent;
            #endif
            assert( nameTmp->supremum() > nameTmp->assignment() || nameTmp->supremum() == nameTmp->assignment() );
            assert( nameTmp->infimum() < nameTmp->assignment() || nameTmp->infimum() == nameTmp->assignment() );
            columnHead.mName->rAssignment() += (*mpTheta);
            // Swap the row and the column head.
            rowHead.mName = columnHead.mName;
            columnHead.mName = nameTmp;
            size_t activityTmp = rowHead.mActivity;
            rowHead.mActivity = columnHead.mActivity;
            if( activityTmp == 0 && rowHead.mActivity > 0 )
            {
                mActiveRows.insert( pivotEntry.rowNumber() );
            }
            else if( activityTmp > 0 && rowHead.mActivity == 0 )
            {
                mActiveRows.erase( pivotEntry.rowNumber() );
            }
            columnHead.mActivity = activityTmp;
            // Adapt both variables.
            Variable<T1, T2>& basicVar = *rowHead.mName;
            basicVar.rPosition() = pivotEntry.rowNumber();
            basicVar.setBasic( true );
            Variable<T1, T2>& nonbasicVar = *columnHead.mName;
            nonbasicVar.rPosition() = pivotEntry.columnNumber();
            nonbasicVar.setBasic( false );
            #ifdef LRA_NO_DIVISION
            basicVar.rFactor() = pivotContent;
            #endif
            // Update the content of the pivoting entry
            #ifdef LRA_NO_DIVISION
            pivotContent = nonbasicVar.factor();
            nonbasicVar.rFactor() = 1;
            #else
            pivotContent = carl::div( T2(1), pivotContent );
            #endif
            #ifdef LRA_REFINEMENT
            rowRefinement( rowHead );
            #endif
            // Let (p_r,p_c,p_e) be the pivoting entry, where p_r is the row number, p_c the column number and p_e the content.
            // For all rows R having a nonzero entry in the pivoting column:
            //    For all columns C having a nonzero entry (r_r,r_c,r_e) in the pivoting row:
            //        Update the entry (t_r,t_c,t_e) of the intersection of R and C to (t_r,t_c,t_e+r_e).
            if( pivotEntry.up() == LAST_ENTRY_ID )
            {
                updateDownwards( _pivotingElement, pivotingRowLeftSide, pivotingRowRightSide );
            }
            else if( pivotEntry.down() == LAST_ENTRY_ID )
            {
                updateUpwards( _pivotingElement, pivotingRowLeftSide, pivotingRowRightSide );
            }
            else
            {
                updateDownwards( _pivotingElement, pivotingRowLeftSide, pivotingRowRightSide );
                updateUpwards( _pivotingElement, pivotingRowLeftSide, pivotingRowRightSide );
            }
            assert( checkCorrectness() == mRows.size() );
            ++mPivotingSteps;
        }

        /**
         *
         * @param _pivotingElement
         * @param _pivotingRow
         */
        template<typename T1, typename T2>
        void Tableau<T1,T2>::updateDownwards( EntryID _pivotingElement, std::vector<Iterator>& _pivotingRowLeftSide, std::vector<Iterator>& _pivotingRowRightSide )
        {
            std::vector<Iterator> leftColumnIters = std::vector<Iterator>( _pivotingRowLeftSide );
            std::vector<Iterator> rightColumnIters = std::vector<Iterator>( _pivotingRowRightSide );
            Iterator pivotingColumnIter = Iterator( _pivotingElement, mpEntries );
            #ifdef LRA_NO_DIVISION
            const T2& pivotingRowFactor = mRows[(*mpEntries)[_pivotingElement].rowNumber()].mName->factor();
            #endif
            while( true )
            {
                // TODO: exclude not activated rows and update them when they get activated
                if( !pivotingColumnIter.columnEnd() )
                {
                    pivotingColumnIter.down();
                }
                else
                {
                    break;
                }
                // Update the assignment of the basic variable corresponding to this row
                #ifdef LRA_NO_DIVISION
                mRows[(*pivotingColumnIter).rowNumber()].mName->rAssignment() += ((*mpTheta) * (*pivotingColumnIter).content())/mRows[(*pivotingColumnIter).rowNumber()].mName->factor();
                #else
                mRows[(*pivotingColumnIter).rowNumber()].mName->rAssignment() += (*mpTheta) * (*pivotingColumnIter).content();
                #endif
                // Update the row
                Iterator currentRowIter = pivotingColumnIter;
                #ifdef LRA_NO_DIVISION
                T2 l = carl::lcm( (*pivotingColumnIter).content(), pivotingRowFactor );
                assert( l > 0 );
                if( (*pivotingColumnIter).content() < 0 && pivotingRowFactor < 0 )
                    l *= T2( -1 );
                T2 ca = carl::div( l, pivotingRowFactor );
                T2 cb = carl::div( l, (*pivotingColumnIter).content() );
                mRows[(*pivotingColumnIter).rowNumber()].mName->rFactor() *= cb;
                Iterator rowIter = Iterator( mRows[(*pivotingColumnIter).rowNumber()].mStartEntry, mpEntries );
                while( true )
                {
                    (*rowIter).rContent() *= cb;
                    if( rowIter.rowEnd() ) break;
                    rowIter.right();
                }
                T2 g = carl::abs( mRows[(*pivotingColumnIter).rowNumber()].mName->factor() );
                #endif
                auto pivotingRowIter = _pivotingRowLeftSide.begin();
                for( auto currentColumnIter = leftColumnIters.begin(); currentColumnIter != leftColumnIters.end(); ++currentColumnIter )
                {
                    assert( pivotingRowIter != _pivotingRowLeftSide.end() );
                    while( !(*currentColumnIter).columnEnd() && (**currentColumnIter).rowNumber() < (*pivotingColumnIter).rowNumber() )
                    {
                        (*currentColumnIter).down();
                    }
                    while( !currentRowIter.rowBegin() && (*currentRowIter).columnNumber() > (**currentColumnIter).columnNumber() )
                    {
                        currentRowIter.left();
                    }
                    // Update the entry
                    if( (*currentColumnIter) == currentRowIter )
                    {
                        // Entry already exists, so update it only and maybe remove it.
                        T2& currentRowContent = (*currentRowIter).rContent();
                        #ifdef LRA_NO_DIVISION
                        currentRowContent += ca * (**pivotingRowIter).content();
                        #else
                        currentRowContent += (*pivotingColumnIter).content() * (**pivotingRowIter).content();
                        #endif
                        if( currentRowContent == 0 )
                        {
                            EntryID toRemove = currentRowIter.entryID();
                            (*currentColumnIter).up();
                            currentRowIter.right();
                            removeEntry( toRemove );
                        }
                    }
                    else
                    {
                        #ifdef LRA_NO_DIVISION
                        EntryID entryID = newTableauEntry( ca * (**pivotingRowIter).content() );
                        #else
                        EntryID entryID = newTableauEntry( (*pivotingColumnIter).content() * (**pivotingRowIter).content() );
                        #endif
                        TableauEntry<T2>& entry = (*mpEntries)[entryID];
                        // Set the position.
                        entry.setRowNumber( (*mpEntries)[currentRowIter.entryID()].rowNumber() );
                        entry.setColumnNumber( (*mpEntries)[(*currentColumnIter).entryID()].columnNumber() );
                        if( (**currentColumnIter).rowNumber() > (*pivotingColumnIter).rowNumber() )
                        {
                            // Entry vertically between two entries.
                            EntryID upperEntryID = (**currentColumnIter).up();
                            if( upperEntryID != LAST_ENTRY_ID )
                            {
                                (*mpEntries)[upperEntryID].setDown( entryID );
                            }
                            (**currentColumnIter).setUp( entryID );
                            entry.setUp( upperEntryID );
                            entry.setDown( (*currentColumnIter).entryID() );
                        }
                        else
                        {
                            // Entry will be the lowest in this column.
                            (**currentColumnIter).setDown( entryID );
                            entry.setUp( (*currentColumnIter).entryID() );
                            entry.setDown( LAST_ENTRY_ID );
                            mColumns[entry.columnNumber()].mStartEntry = entryID;
                        }
                        if( (*currentRowIter).columnNumber() < (**currentColumnIter).columnNumber() )
                        {
                            // Entry horizontally between two entries.
                            EntryID rightEntryID = (*currentRowIter).right();
                            if( rightEntryID != LAST_ENTRY_ID )
                            {
                                (*mpEntries)[rightEntryID].setLeft( entryID );
                            }
                            (*currentRowIter).setRight( entryID );
                            entry.setRight( rightEntryID );
                            entry.setLeft( currentRowIter.entryID() );
                        }
                        else
                        {
                            // Entry will be the leftmost in this row.
                            (*currentRowIter).setLeft( entryID );
                            entry.setRight( currentRowIter.entryID() );
                            entry.setLeft( LAST_ENTRY_ID );
                            mRows[entry.rowNumber()].mStartEntry = entryID;
                        }
                        // Set the content of the entry.
                        ++mRows[entry.rowNumber()].mSize;
                        ++mColumns[entry.columnNumber()].mSize;
                    }
                    ++pivotingRowIter;
                }
                currentRowIter = pivotingColumnIter;
                pivotingRowIter = _pivotingRowRightSide.begin();
                for( auto currentColumnIter = rightColumnIters.begin(); currentColumnIter != rightColumnIters.end(); ++currentColumnIter )
                {
                    assert( pivotingRowIter != _pivotingRowRightSide.end() );
                    while( !(*currentColumnIter).columnEnd() && (**currentColumnIter).rowNumber() < (*pivotingColumnIter).rowNumber() )
                    {
                        (*currentColumnIter).down();
                    }
                    while( !currentRowIter.rowEnd() && (*currentRowIter).columnNumber() < (**currentColumnIter).columnNumber() )
                    {
                        currentRowIter.right();
                    }
                    // Update the entry
                    if( (*currentColumnIter) == currentRowIter )
                    {
                        // Entry already exists, so update it only and maybe remove it.
                        T2& currentRowContent = (*currentRowIter).rContent();
                        #ifdef LRA_NO_DIVISION
                        currentRowContent += ca * (**pivotingRowIter).content();
                        #else
                        currentRowContent += (*pivotingColumnIter).content() * (**pivotingRowIter).content();
                        #endif
                        if( currentRowContent == 0 )
                        {
                            EntryID toRemove = currentRowIter.entryID();
                            (*currentColumnIter).up();
                            currentRowIter.left();
                            removeEntry( toRemove );
                        }
                    }
                    else
                    {
                        #ifdef LRA_NO_DIVISION
                        EntryID entryID = newTableauEntry( ca * (**pivotingRowIter).content() );
                        #else
                        EntryID entryID = newTableauEntry( (*pivotingColumnIter).content() * (**pivotingRowIter).content() );
                        #endif
                        TableauEntry<T2>& entry = (*mpEntries)[entryID];
                        // Set the position.
                        entry.setRowNumber( (*mpEntries)[currentRowIter.entryID()].rowNumber() );
                        entry.setColumnNumber( (*mpEntries)[(*currentColumnIter).entryID()].columnNumber() );
                        if( (**currentColumnIter).rowNumber() > (*pivotingColumnIter).rowNumber() )
                        {
                            // Entry vertically between two entries.
                            EntryID upperEntryID = (**currentColumnIter).up();
                            if( upperEntryID != LAST_ENTRY_ID )
                            {
                                (*mpEntries)[upperEntryID].setDown( entryID );
                            }
                            (**currentColumnIter).setUp( entryID );
                            entry.setUp( upperEntryID );
                            entry.setDown( (*currentColumnIter).entryID() );
                        }
                        else
                        {
                            // Entry will be the lowest in this column.
                            (**currentColumnIter).setDown( entryID );
                            entry.setUp( (*currentColumnIter).entryID() );
                            entry.setDown( LAST_ENTRY_ID );
                            mColumns[entry.columnNumber()].mStartEntry = entryID;
                        }
                        if( (*currentRowIter).columnNumber() > (**currentColumnIter).columnNumber() )
                        {
                            // Entry horizontally between two entries.
                            EntryID leftEntryID = (*currentRowIter).left();
                            if( leftEntryID != LAST_ENTRY_ID )
                            {
                                (*mpEntries)[leftEntryID].setRight( entryID );
                            }
                            (*currentRowIter).setLeft( entryID );
                            entry.setLeft( leftEntryID );
                            entry.setRight( currentRowIter.entryID() );
                        }
                        else
                        {
                            // Entry will be the rightmost in this row.
                            (*currentRowIter).setRight( entryID );
                            entry.setLeft( currentRowIter.entryID() );
                            entry.setRight( LAST_ENTRY_ID );
                        }
                        // Set the content of the entry.
                        ++mRows[entry.rowNumber()].mSize;
                        ++mColumns[entry.columnNumber()].mSize;
                    }
                    ++pivotingRowIter;
                }
                #ifdef LRA_NO_DIVISION
                (*pivotingColumnIter).rContent() = ca * (*mpEntries)[_pivotingElement].content();
                rowIter = Iterator( mRows[(*pivotingColumnIter).rowNumber()].mStartEntry, mpEntries );
                while( !(g == 1) )
                {
                    carl::gcd_here( g, (*rowIter).content() );
                    if( rowIter.rowEnd() ) break;
                    rowIter.right();
                }
                if( !(g == 1) )
                {
                    assert( g > 0 );
                    rowIter = Iterator( mRows[(*pivotingColumnIter).rowNumber()].mStartEntry, mpEntries );
                    while( true )
                    {
                        carl::div_here( (*rowIter).rContent(), g );
                        if( rowIter.rowEnd() ) break;
                        else rowIter.right();
                    }
                    carl::div_here( mRows[(*pivotingColumnIter).rowNumber()].mName->rFactor(), g );
                }
                #else
                (*pivotingColumnIter).rContent() *= (*mpEntries)[_pivotingElement].content();
                #endif
                #ifdef LRA_REFINEMENT
                rowRefinement( mRows[(*pivotingColumnIter).rowNumber()] );
                #endif
            }
        }

        /**
         *
         * @param _pivotingElement
         * @param _pivotingRow
         */
        template<typename T1, typename T2>
        void Tableau<T1,T2>::updateUpwards( EntryID _pivotingElement, std::vector<Iterator>& _pivotingRowLeftSide, std::vector<Iterator>& _pivotingRowRightSide )
        {
            std::vector<Iterator> leftColumnIters = std::vector<Iterator>( _pivotingRowLeftSide );
            std::vector<Iterator> rightColumnIters = std::vector<Iterator>( _pivotingRowRightSide );
            Iterator pivotingColumnIter = Iterator( _pivotingElement, mpEntries );
            #ifdef LRA_NO_DIVISION
            const T2& pivotingRowFactor = mRows[(*mpEntries)[_pivotingElement].rowNumber()].mName->factor();
            #endif
            while( true )
            {
                // TODO: exclude not activated rows and update them when they get activated
                if( !pivotingColumnIter.columnBegin() )
                {
                    pivotingColumnIter.up();
                }
                else
                {
                    break;
                }
                // Update the assignment of the basic variable corresponding to this row
                #ifdef LRA_NO_DIVISION
                mRows[(*pivotingColumnIter).rowNumber()].mName->rAssignment() += ((*mpTheta) * (*pivotingColumnIter).content())/mRows[(*pivotingColumnIter).rowNumber()].mName->factor();
                #else
                mRows[(*pivotingColumnIter).rowNumber()].mName->rAssignment() += (*mpTheta) * (*pivotingColumnIter).content();
                #endif
                // Update the row
                Iterator currentRowIter = pivotingColumnIter;
                #ifdef LRA_NO_DIVISION
                T2 l = carl::lcm( (*pivotingColumnIter).content(), pivotingRowFactor );
                assert( l > 0 );
                if( (*pivotingColumnIter).content() < 0 && pivotingRowFactor < 0 )
                    l *= T2( -1 );
                T2 ca = carl::div( l, pivotingRowFactor );
                T2 cb = carl::div( l, (*pivotingColumnIter).content() );
                mRows[(*pivotingColumnIter).rowNumber()].mName->rFactor() *= cb;
                Iterator rowIter = Iterator( mRows[(*pivotingColumnIter).rowNumber()].mStartEntry, mpEntries );
                while( true )
                {
                    (*rowIter).rContent() *= cb;
                    if( rowIter.rowEnd() ) break;
                    rowIter.right();
                }
                T2 g = carl::abs( mRows[(*pivotingColumnIter).rowNumber()].mName->factor() ); 
                #endif
                auto pivotingRowIter = _pivotingRowLeftSide.begin();
                for( auto currentColumnIter = leftColumnIters.begin(); currentColumnIter != leftColumnIters.end(); ++currentColumnIter )
                {
                    assert( pivotingRowIter != _pivotingRowLeftSide.end() );
                    while( !(*currentColumnIter).columnBegin() && (**currentColumnIter).rowNumber() > (*pivotingColumnIter).rowNumber() )
                    {
                        (*currentColumnIter).up();
                    }
                    while( !currentRowIter.rowBegin() && (*currentRowIter).columnNumber() > (**currentColumnIter).columnNumber() )
                    {
                        currentRowIter.left();
                    }
                    // Update the entry
                    if( (*currentColumnIter) == currentRowIter )
                    {
                        // Entry already exists, so update it only and maybe remove it.
                        T2& currentRowContent = (*currentRowIter).rContent();
                        #ifdef LRA_NO_DIVISION
                        currentRowContent += ca * (**pivotingRowIter).content();
                        #else
                        currentRowContent += (*pivotingColumnIter).content() * (**pivotingRowIter).content();
                        #endif
                        if( currentRowContent == 0 )
                        {
                            EntryID toRemove = currentRowIter.entryID();
                            (*currentColumnIter).down();
                            currentRowIter.right();
                            removeEntry( toRemove );
                        }
                    }
                    else
                    {
                        #ifdef LRA_NO_DIVISION
                        EntryID entryID = newTableauEntry( ca * (**pivotingRowIter).content() );
                        #else
                        EntryID entryID = newTableauEntry( (*pivotingColumnIter).content() * (**pivotingRowIter).content() );
                        #endif
                        TableauEntry<T2>& entry = (*mpEntries)[entryID];
                        // Set the position.
                        entry.setRowNumber( (*mpEntries)[currentRowIter.entryID()].rowNumber() );
                        entry.setColumnNumber( (*mpEntries)[(*currentColumnIter).entryID()].columnNumber() );
                        if( (**currentColumnIter).rowNumber() < (*pivotingColumnIter).rowNumber() )
                        {
                            // Entry vertically between two entries.
                            EntryID lowerEntryID = (**currentColumnIter).down();
                            if( lowerEntryID != LAST_ENTRY_ID )
                            {
                                (*mpEntries)[lowerEntryID].setUp( entryID );
                            }
                            (**currentColumnIter).setDown( entryID );
                            entry.setDown( lowerEntryID );
                            entry.setUp( (*currentColumnIter).entryID() );
                        }
                        else
                        {
                            (**currentColumnIter).setUp( entryID );
                            entry.setDown( (*currentColumnIter).entryID() );
                            entry.setUp( LAST_ENTRY_ID );
                        }
                        if( (*currentRowIter).columnNumber() < (**currentColumnIter).columnNumber() )
                        {
                            // Entry horizontally between two entries.
                            EntryID rightEntryID = (*currentRowIter).right();
                            if( rightEntryID != LAST_ENTRY_ID )
                            {
                                (*mpEntries)[rightEntryID].setLeft( entryID );
                            }
                            (*currentRowIter).setRight( entryID );
                            entry.setRight( rightEntryID );
                            entry.setLeft( currentRowIter.entryID() );
                        }
                        else
                        {
                            (*currentRowIter).setLeft( entryID );
                            entry.setRight( currentRowIter.entryID() );
                            entry.setLeft( LAST_ENTRY_ID );
                            mRows[entry.rowNumber()].mStartEntry = entryID;
                        }
                        // Set the content of the entry.
                        ++mRows[entry.rowNumber()].mSize;
                        ++mColumns[entry.columnNumber()].mSize;
                    }
                    ++pivotingRowIter;
                }
                currentRowIter = pivotingColumnIter;
                pivotingRowIter = _pivotingRowRightSide.begin();
                for( auto currentColumnIter = rightColumnIters.begin(); currentColumnIter != rightColumnIters.end(); ++currentColumnIter )
                {
                    assert( pivotingRowIter != _pivotingRowRightSide.end() );
                    while( !(*currentColumnIter).columnBegin() && (**currentColumnIter).rowNumber() > (*pivotingColumnIter).rowNumber() )
                    {
                        (*currentColumnIter).up();
                    }
                    while( !currentRowIter.rowEnd() && (*currentRowIter).columnNumber() < (**currentColumnIter).columnNumber() )
                    {
                        currentRowIter.right();
                    }
                    // Update the entry
                    if( (*currentColumnIter) == currentRowIter )
                    {
                        // Entry already exists, so update it only and maybe remove it.
                        T2& currentRowContent = (*currentRowIter).rContent();
                        #ifdef LRA_NO_DIVISION
                        currentRowContent += ca * (**pivotingRowIter).content();
                        #else
                        currentRowContent += (*pivotingColumnIter).content() * (**pivotingRowIter).content();
                        #endif
                        if( currentRowContent == 0 )
                        {
                            EntryID toRemove = currentRowIter.entryID();
                            (*currentColumnIter).down();
                            currentRowIter.left();
                            removeEntry( toRemove );
                        }
                    }
                    else
                    {
                        #ifdef LRA_NO_DIVISION
                        EntryID entryID = newTableauEntry( ca * (**pivotingRowIter).content() );
                        #else
                        EntryID entryID = newTableauEntry( (*pivotingColumnIter).content() * (**pivotingRowIter).content() );
                        #endif
                        TableauEntry<T2>& entry = (*mpEntries)[entryID];
                        // Set the position.
                        entry.setRowNumber( (*mpEntries)[currentRowIter.entryID()].rowNumber() );
                        entry.setColumnNumber( (*mpEntries)[(*currentColumnIter).entryID()].columnNumber() );
                        if( (**currentColumnIter).rowNumber() < (*pivotingColumnIter).rowNumber() )
                        {
                            // Entry vertically between two entries.
                            EntryID lowerEntryID = (**currentColumnIter).down();
                            if( lowerEntryID != LAST_ENTRY_ID )
                            {
                                (*mpEntries)[lowerEntryID].setUp( entryID );
                            }
                            (**currentColumnIter).setDown( entryID );
                            entry.setDown( lowerEntryID );
                            entry.setUp( (*currentColumnIter).entryID() );
                        }
                        else
                        {
                            // Entry will be the uppermost in this column.
                            (**currentColumnIter).setUp( entryID );
                            entry.setDown( (*currentColumnIter).entryID() );
                            entry.setUp( LAST_ENTRY_ID );
                        }
                        if( (*currentRowIter).columnNumber() > (**currentColumnIter).columnNumber() )
                        {
                            // Entry horizontally between two entries.
                            EntryID leftEntryID = (*currentRowIter).left();
                            if( leftEntryID != LAST_ENTRY_ID )
                            {
                                (*mpEntries)[leftEntryID].setRight( entryID );
                            }
                            (*currentRowIter).setLeft( entryID );
                            entry.setLeft( leftEntryID );
                            entry.setRight( currentRowIter.entryID() );
                        }
                        else
                        {
                            // Entry will be the rightmost in this row.
                            (*currentRowIter).setRight( entryID );
                            entry.setLeft( currentRowIter.entryID() );
                            entry.setRight( LAST_ENTRY_ID );
                        }
                        // Set the content of the entry.
                        ++mRows[entry.rowNumber()].mSize;
                        ++mColumns[entry.columnNumber()].mSize;
                    }
                    ++pivotingRowIter;
                }
                #ifdef LRA_NO_DIVISION
                (*pivotingColumnIter).rContent() = ca * (*mpEntries)[_pivotingElement].content();
                rowIter = Iterator( mRows[(*pivotingColumnIter).rowNumber()].mStartEntry, mpEntries );
                while( !(g == 1) )
                {
                    carl::gcd_here( g, (*rowIter).content() );
                    if( rowIter.rowEnd() ) break;
                    rowIter.right();
                }
                if( !(g == 1) )
                {
                    assert( g > 0 );
                    rowIter = Iterator( mRows[(*pivotingColumnIter).rowNumber()].mStartEntry, mpEntries );
                    while( true )
                    {
                        carl::div_here( (*rowIter).rContent(), g );
                        if( rowIter.rowEnd() ) break;
                        else rowIter.right();
                    }
                    carl::div_here( mRows[(*pivotingColumnIter).rowNumber()].mName->rFactor(), g );
                }
                #else
                (*pivotingColumnIter).rContent() *= (*mpEntries)[_pivotingElement].content();
                #endif
                #ifdef LRA_REFINEMENT
                rowRefinement( mRows[(*pivotingColumnIter).rowNumber()] );
                #endif
            }
        }

        #ifdef LRA_REFINEMENT
        template<typename T1, typename T2>
        void Tableau<T1,T2>::rowRefinement( const TableauHead& _row )
        {
            /*
             * Collect the bounds which form an upper resp. lower refinement.
             */
            std::vector<const Bound<T1, T2>*>* uPremise = NULL;
            if( _row.mName->supremum() > _row.mName->assignment() )
                uPremise = new std::vector<const Bound<T1, T2>*>();
            std::vector<const Bound<T1, T2>*>* lPremise = NULL;
            if( _row.mName->infimum() < _row.mName->assignment() )
                lPremise = new std::vector<const Bound<T1, T2>*>();
            if( uPremise == NULL && lPremise == NULL ) return;
            Iterator rowEntry = Iterator( _row.mStartEntry, mpEntries );
            #ifdef LRA_NO_DIVISION
            const T2& rowFactor = _row.mName->factor();
            #endif
            while( true )
            {
                #ifdef LRA_NO_DIVISION
                if( ((*rowEntry).content() > 0 && rowFactor > 0) || ((*rowEntry).content() < 0 && rowFactor < 0) )
                #else
                if( (*rowEntry).content() > 0 )
                #endif
                {
                    if( uPremise != NULL )
                    {
                        const Bound<T1, T2>* sup = mColumns[(*rowEntry).columnNumber()].mName->pSupremum();
                        if( sup->pLimit() != NULL )
                        {
                            uPremise->push_back( sup );
                        }
                        else
                        {
                            delete uPremise;
                            uPremise = NULL;
                            if( lPremise == NULL ) return;
                        }
                    }
                    if( lPremise != NULL )
                    {
                        const Bound<T1, T2>* inf = mColumns[(*rowEntry).columnNumber()].mName->pInfimum();
                        if( inf->pLimit() != NULL )
                        {
                            lPremise->push_back( inf );
                        }
                        else
                        {
                            delete lPremise;
                            lPremise = NULL;
                            if( uPremise == NULL ) return;
                        }
                    }
                }
                else
                {
                    if( uPremise != NULL )
                    {
                        const Bound<T1, T2>* inf = mColumns[(*rowEntry).columnNumber()].mName->pInfimum();
                        if( inf->pLimit() != NULL )
                        {
                            uPremise->push_back( inf );
                        }
                        else
                        {
                            delete uPremise;
                            uPremise = NULL;
                            if( lPremise == NULL ) return;
                        }
                    }
                    if( lPremise != NULL )
                    {
                        const Bound<T1, T2>* sup = mColumns[(*rowEntry).columnNumber()].mName->pSupremum();
                        if( sup->pLimit() != NULL )
                        {
                            lPremise->push_back( sup );
                        }
                        else
                        {
                            delete lPremise;
                            lPremise = NULL;
                            if( uPremise == NULL ) return;
                        }
                    }
                }
                if( rowEntry.rowEnd() ) break;
                else rowEntry.right();
            }
            if( uPremise != NULL )
            {
                /*
                 * Found an upper refinement.
                 */
                Value<T1>* newlimit = new Value<T1>();
                typename std::vector< const Bound<T1, T2>* >::iterator bound = uPremise->begin();
                Iterator rowEntry = Iterator( _row.mStartEntry, mpEntries );
                while( true )
                {
                    *newlimit += (*bound)->limit() * (*rowEntry).content();
                    ++bound;
                    if( !rowEntry.rowEnd() ) rowEntry.right();
                    else break;
                }
                /*
                 * Learn that the strongest weaker upper bound should be activated.
                 */
                Variable<T1, T2>& bvar = *_row.mName;
                const typename Bound<T1, T2>::BoundSet& upperBounds = bvar.upperbounds();
                auto ubound = upperBounds.begin();
                while( ubound != upperBounds.end() )
                {
                    #ifdef LRA_NO_DIVISION
                    if( **ubound > (*newlimit)/rowFactor && (*ubound)->type() != Bound<T1, T2>::EQUAL && !(*ubound)->deduced() )
                    #else
                    if( **ubound > *newlimit && (*ubound)->type() != Bound<T1, T2>::EQUAL && !(*ubound)->deduced() )
                    #endif
                    {
                        break;
                    }
                    if( *ubound == bvar.pSupremum() )
                    {
                        delete newlimit;
                        delete uPremise;
                        goto CheckLowerPremise;
                    }
                    ++ubound;
                }
                if( ubound != --upperBounds.end() )
                {
                    assert( ((*ubound)->type() != Bound<T1, T2>::EQUAL) );
                    LearnedBound learnedBound = LearnedBound();
                    learnedBound.nextWeakerBound = *ubound;
                    learnedBound.premise = uPremise;
                    #ifdef LRA_INTRODUCE_NEW_CONSTRAINTS
                    #ifdef LRA_NO_DIVISION
                    if( newlimit->mainPart() > (*ubound)->limit().mainPart()*rowFactor || (*ubound)->limit().deltaPart() == 0 )
                    #else
                    if( newlimit->mainPart() > (*ubound)->limit().mainPart() || (*ubound)->limit().deltaPart() == 0 )
                    #endif
                    {
                        #ifdef LRA_NO_DIVISION
                        smtrat::Polynomial lhs = (*ubound)->variable().expression()*(Rational)rowFactor - (Rational)newlimit->mainPart();
                        #else
                        smtrat::Polynomial lhs = (*ubound)->variable().expression() - (Rational)newlimit->mainPart();
                        #endif
                        smtrat::Relation rel = newlimit->deltaPart() != 0 ? smtrat::Relation::LESS : smtrat::Relation::LEQ;
                        const smtrat::Constraint* constraint = smtrat::Formula::newConstraint( lhs, rel );
                        learnedBound.newBound = bvar.addUpperBound( newlimit, mDefaultBoundPosition, constraint, true ).first;
                    }
                    else
                    {
                        learnedBound.newBound = NULL;
                    }
                    #else
                    delete newlimit;
                    learnedBound.newBound = NULL;
                    #endif
                    std::pair<typename std::map<Variable<T1, T2>*, LearnedBound>::iterator, bool> insertionResult = mLearnedUpperBounds.insert( std::pair<Variable<T1, T2>*, LearnedBound>( _row.mName, learnedBound ) );
                    if( !insertionResult.second )
                    {
                        if( *learnedBound.nextWeakerBound < *insertionResult.first->second.nextWeakerBound )
                        {
                            insertionResult.first->second.nextWeakerBound = learnedBound.nextWeakerBound;
                            delete insertionResult.first->second.premise;
                            insertionResult.first->second.premise = learnedBound.premise;
                            mNewLearnedBounds.push_back( insertionResult.first );
                        }
                    }
                    else
                    {
                        mNewLearnedBounds.push_back( insertionResult.first );
                    }
                }
                else
                {
                    delete newlimit;
                    delete uPremise;
                }
            }
    CheckLowerPremise:
            if( lPremise != NULL )
            {
                /*
                 * Found an lower refinement.
                 */
                Value<T1>* newlimit = new Value<T1>();
                typename std::vector< const Bound<T1, T2>* >::iterator bound = lPremise->begin();
                Iterator rowEntry = Iterator( _row.mStartEntry, mpEntries );
                while( true )
                {
                    *newlimit += (*bound)->limit() * (*rowEntry).content();
                    ++bound;
                    if( !rowEntry.rowEnd() ) rowEntry.right();
                    else break;
                }
                /*
                 * Learn that the strongest weaker lower bound should be activated.
                 */
                Variable<T1, T2>& bvar = *_row.mName;
                const typename Bound<T1, T2>::BoundSet& lowerBounds = bvar.lowerbounds();
                auto lbound = lowerBounds.rbegin();
                while( lbound != lowerBounds.rend() )
                {
                    #ifdef LRA_NO_DIVISION
                    if( **lbound < (*newlimit)/rowFactor && (*lbound)->type() != Bound<T1, T2>::EQUAL && !(*lbound)->deduced() )
                    #else
                    if( **lbound < *newlimit && (*lbound)->type() != Bound<T1, T2>::EQUAL && !(*lbound)->deduced() )
                    #endif
                    {
                        break;
                    }
                    if( *lbound == bvar.pInfimum()  )
                    {
                        delete newlimit;
                        delete lPremise;
                        return;
                    }
                    ++lbound;
                }
                if( lbound != --lowerBounds.rend() )
                {
                    assert( ((*lbound)->type() != Bound<T1, T2>::EQUAL) );
                    LearnedBound learnedBound = LearnedBound();
                    learnedBound.nextWeakerBound = *lbound;
                    learnedBound.premise = lPremise;
                    #ifdef LRA_INTRODUCE_NEW_CONSTRAINTS
                    #ifdef LRA_NO_DIVISION
                    if( newlimit->mainPart() > (*lbound)->limit().mainPart()*rowFactor || (*lbound)->limit().deltaPart() == 0 )
                    #else
                    if( newlimit->mainPart() > (*lbound)->limit().mainPart() || (*lbound)->limit().deltaPart() == 0 )
                    #endif
                    {
                        #ifdef LRA_NO_DIVISION
                        smtrat::Polynomial lhs = (*lbound)->variable().expression()*(Rational)rowFactor - (Rational)newlimit->mainPart();
                        #else
                        smtrat::Polynomial lhs = (*lbound)->variable().expression() - (Rational)newlimit->mainPart();
                        #endif
                        smtrat::Relation rel = newlimit->deltaPart() != 0 ? smtrat::Relation::GREATER : smtrat::Relation::GEQ;
                        const smtrat::Constraint* constraint = smtrat::Formula::newConstraint( lhs, rel );
                        learnedBound.newBound = bvar.addLowerBound( newlimit, mDefaultBoundPosition, constraint, true ).first;
                    }
                    else
                    {
                        learnedBound.newBound = NULL;
                    }
                    #else
                    delete newlimit;
                    learnedBound.newBound = NULL;
                    #endif
                    std::pair<typename std::map<Variable<T1, T2>*, LearnedBound>::iterator, bool> insertionResult = mLearnedLowerBounds.insert( std::pair<Variable<T1, T2>*, LearnedBound>( _row.mName, learnedBound ) );
                    if( !insertionResult.second )
                    {
                        if( *learnedBound.nextWeakerBound > *insertionResult.first->second.nextWeakerBound )
                        {
                            insertionResult.first->second.nextWeakerBound = learnedBound.nextWeakerBound;
                            delete insertionResult.first->second.premise;
                            insertionResult.first->second.premise = learnedBound.premise;
                            mNewLearnedBounds.push_back( insertionResult.first );
                        }
                    }
                    else
                    {
                        mNewLearnedBounds.push_back( insertionResult.first );
                    }
                }
                else
                {
                    delete newlimit;
                    delete lPremise;
                }
            }
        }
        #endif

        /**
         *
         * @return
         */
        template<typename T1, typename T2>
        size_t Tableau<T1,T2>::checkCorrectness() const
        {
            size_t rowNumber = 0;
            for( ; rowNumber < mRows.size(); ++rowNumber )
            {
                if( !rowCorrect( rowNumber ) ) return rowNumber;
            }
            return rowNumber;
        }

        /**
         *
         * @return
         */
        template<typename T1, typename T2>
        bool Tableau<T1,T2>::rowCorrect( size_t _rowNumber ) const
        {
            smtrat::Polynomial sumOfNonbasics = smtrat::ZERO_POLYNOMIAL;
            Iterator rowEntry = Iterator( mRows[_rowNumber].mStartEntry, mpEntries );
            while( !rowEntry.rowEnd() )
            {
                sumOfNonbasics += (*mColumns[(*rowEntry).columnNumber()].mName->pExpression()) * smtrat::Polynomial( (*rowEntry).content() );
                rowEntry.right();
            }
            sumOfNonbasics += (*mColumns[(*rowEntry).columnNumber()].mName->pExpression()) * smtrat::Polynomial( (*rowEntry).content() );
            #ifdef LRA_NO_DIVISION
            sumOfNonbasics += (*mRows[_rowNumber].mName->pExpression()) * smtrat::Polynomial( mRows[_rowNumber].mName->factor() ) * smtrat::MINUS_ONE_POLYNOMIAL;
            #else
            sumOfNonbasics += (*mRows[_rowNumber].mName->pExpression()) * smtrat::MINUS_ONE_POLYNOMIAL;
            #endif
//            if( !sumOfNonbasics.isZero() )
//            {
//                print();
//                std::cout << "Row number: " << _rowNumber << std::endl;
//            }
            if( !sumOfNonbasics.isZero() ) return false;
            return true;
        }
        
        #ifdef LRA_CUTS_FROM_PROOFS
        /**
         * Checks whether a constraint is a defining constraint. 
         * 
         * @return true,    if the constraint is a defining constraint
         *         false,   otherwise   
         */
        template<typename T1, typename T2>
        bool Tableau<T1,T2>::isDefining( size_t row_index, std::vector<size_t>& _variables, std::vector<T2>& _coefficients, T2& _lcmOfCoeffDenoms, T2& max_value ) const
        {
            const Variable<T1, T2>& basic_var = *mRows.at(row_index).mName;
            Iterator row_iterator = Iterator( mRows.at(row_index).mStartEntry, mpEntries );
            if( basic_var.infimum() == basic_var.assignment() || basic_var.supremum() == basic_var.assignment() )
            {
                /*
                 * The row represents a DC. Collect the nonbasics and the referring coefficients.
                 */
                while( true )
                {
                    _variables.push_back( (*row_iterator).columnNumber() );
                    _coefficients.push_back( (*row_iterator).content() );
                    _lcmOfCoeffDenoms = carl::lcm( _lcmOfCoeffDenoms, (*row_iterator).content().denom() );
                    if( !row_iterator.rowEnd() )
                    {
                        row_iterator.right();
                    }
                    else
                    {
                        break;
                    }
                }
                return true;
            }
            else
            {
                while( true )
                {
                    T2 abs_content = carl::abs((*row_iterator).content());
                    if(abs_content > max_value)
                    {
                        max_value = abs_content;                        
                    }
                    if( !row_iterator.rowEnd() )
                    {
                        row_iterator.right();
                    }
                    else
                    {
                        break;
                    }                    
                }                
            }
            return false;
        }
        
        /**
         * Checks whether the row with index row_index 
         * is defining. 
         * 
         * @return true,    if so
         *         false,   otherwise   
         */ 
        template<typename T1, typename T2>
        bool Tableau<T1,T2>::isDefining_Easy(std::vector<size_t>& dc_positions,size_t row_index)
        {
            auto vector_iterator = dc_positions.begin();
            while(vector_iterator != dc_positions.end())
            {
                if(*vector_iterator == row_index)
                {
                    return true;
                }
            }
            return false;
        }
        
        /**
         * Checks whether the column with index column_index 
         * is a diagonal column. 
         * 
         * @return true,    if the column with index column_index is a diagonal column
         *         false,   otherwise   
         */        
        template<typename T1, typename T2>
        bool Tableau<T1,T2>::isDiagonal(size_t column_index , std::vector<size_t>& diagonals)
        {
        size_t i=0;
        while(diagonals.at(i) != mColumns.size())
        {
            if(diagonals.at(i) == column_index)
            {
                return true;
            }
        ++i;    
        }
        return false;            
        }
        
        /**
         * Returns the row of the defining constraint with index row_index
         * in the Tableau containing this DC.
         * 
         */ 
        template<typename T1, typename T2>
        size_t Tableau<T1,T2>::position_DC(size_t row_index,std::vector<size_t>& dc_positions)
        {
            auto vector_iterator = dc_positions.begin();
            size_t i=0;
            while(vector_iterator != dc_positions.end())
            {
                if(*vector_iterator == row_index)
                {
                    return i;
                }
                ++i;
                ++vector_iterator;
            }
            return mRows.size();
        }
        
        /**
         * Returns the the actual index of the column with
         * index column_index in the permutated tableau.   
         */        
        template<typename T1, typename T2>
        size_t Tableau<T1,T2>::revert_diagonals(size_t column_index,std::vector<size_t>& diagonals)
        {
            size_t i=0;
            while(diagonals.at(i) != mColumns.size())   
            {
                if(diagonals.at(i) == column_index)
                {
                    return i;
                }
                ++i;
            }
            return mColumns.size();
        }
        
        /**
         * Multiplies all entries in the column with the index column_index by (-1). 
         * 
         * @return   
         */        
        template<typename T1, typename T2>
        void Tableau<T1,T2>::invertColumn(size_t column_index)
        {   
            Iterator column_iterator = Iterator(mColumns.at(column_index).mStartEntry, mpEntries);   
            while(true)
            {
                (*mpEntries)[column_iterator.entryID()].rContent() = (-1)*(((*mpEntries)[column_iterator.entryID()].rContent()).content());
                if(!column_iterator.columnBegin())
                {
                    column_iterator.up();            
                } 
                else 
                {
                    break;
                }
            }        
        }
        
        /**
         * Adds the column with index columnB_index multplied by multiple 
         * to the column with index columnA_index.
         * 
         * @return 
         */
        template<typename T1, typename T2>
        void Tableau<T1,T2>::addColumns( size_t columnA_index, size_t columnB_index, T2 multiple)
        {
            #ifdef LRA_DEBUG_CUTS_FROM_PROOFS
            std::cout << __func__ << "( " << columnA_index << ", " << columnB_index << ", " << multiple << " )" << std::endl;
            #endif
            Iterator columnA_iterator = Iterator(mColumns.at(columnA_index).mStartEntry, mpEntries);
            Iterator columnB_iterator = Iterator(mColumns.at(columnB_index).mStartEntry, mpEntries);
                
            while(true)
            {
            /* 
             * Make columnA_iterator and columnB_iterator neighbors. 
             */ 
            while((*columnA_iterator).rowNumber() > (*columnB_iterator).rowNumber() && !columnA_iterator.columnBegin())
            {
                columnA_iterator.up();
            }    
            EntryID ID1_to_be_Fixed,ID2_to_be_Fixed;            
            if((*columnA_iterator).rowNumber() == (*columnB_iterator).rowNumber())
            {
                T2 content = T2(((*columnA_iterator).content().content())+((multiple.content())*((*columnB_iterator).content().content())));  
                if(content == 0)
                {
                    EntryID to_delete = columnA_iterator.entryID();
                    if(!columnA_iterator.columnBegin())
                    {                        
                        columnA_iterator.up();
                    }    
                    removeEntry(to_delete);                
                 }                
                 else
                 {
                    (*columnA_iterator).rContent() = content;           
                 }    
              }
              else if((*columnA_iterator).rowNumber() < (*columnB_iterator).rowNumber()) 
              {
                  /*
                   * A new entry has to be created under the position of columnA_iterator
                   * and sideways to column_B_iterator.
                   */   
                  EntryID entryID = newTableauEntry(T2(((multiple.content())*((*columnB_iterator).content().content()))));
                  TableauEntry<T2>& entry = (*mpEntries)[entryID];
                  TableauEntry<T2>& entry_down = (*mpEntries)[(*columnA_iterator).down()];   
                  EntryID down = (*columnA_iterator).down();
                  entry.setColumnNumber((*columnA_iterator).columnNumber());
                  entry.setRowNumber((*columnB_iterator).rowNumber());
                  entry.setDown(down);
                  entry.setUp(columnA_iterator.entryID());
                  entry_down.setUp(entryID);
                  (*columnA_iterator).setDown(entryID);
                  TableauHead& columnHead = mColumns[entry.columnNumber()];
                  ++columnHead.mSize;
                  Iterator row_iterator = Iterator(columnB_iterator.entryID(), mpEntries);
                  ID2_to_be_Fixed = row_iterator.entryID();
                  if((*row_iterator).columnNumber() > entry.columnNumber())
                  {
                      /*
                       * The new entry is left from the added entry.
                       * Search for the entries which have to be modified.
                       */
                      while((*row_iterator).columnNumber() > entry.columnNumber() && !row_iterator.rowBegin())
                      {
                          ID1_to_be_Fixed = row_iterator.entryID();
                          row_iterator.left(); 
                          ID2_to_be_Fixed = row_iterator.entryID();
                      }
                      if((*row_iterator).columnNumber() > entry.columnNumber() && row_iterator.rowBegin())
                      {                          
                          (*mpEntries)[entryID].setLeft(LAST_ENTRY_ID);  
                          (*mpEntries)[entryID].setRight(ID2_to_be_Fixed);
                          (*mpEntries)[ID2_to_be_Fixed].setLeft(entryID);
                          TableauHead& rowHead = mRows[(*columnB_iterator).rowNumber()];
                          rowHead.mStartEntry = entryID;
                      }                     
                      else
                      {
                          (*mpEntries)[ID2_to_be_Fixed].setRight(entryID);
                          (*mpEntries)[ID1_to_be_Fixed].setLeft(entryID);
                          (*mpEntries)[entryID].setLeft(ID2_to_be_Fixed); 
                          (*mpEntries)[entryID].setRight(ID1_to_be_Fixed);
                      }
                  }    
                  else
                  {
                      /*
                       * The new entry is right from the added entry.
                       * Search for the entries which have to be modified.
                       */                      
                      while((*row_iterator).columnNumber() < entry.columnNumber() && !row_iterator.rowEnd())
                      {
                          ID1_to_be_Fixed = row_iterator.entryID();
                          row_iterator.right();
                          ID2_to_be_Fixed = row_iterator.entryID();
                      }
                      if((*row_iterator).columnNumber() < entry.columnNumber() && row_iterator.rowEnd())
                      {
                          (*mpEntries)[entryID].setRight(LAST_ENTRY_ID);  
                          (*mpEntries)[entryID].setLeft(ID2_to_be_Fixed);
                          (*mpEntries)[ID2_to_be_Fixed].setRight(entryID);
                      }    
                      else
                      {                          
                          (*mpEntries)[ID2_to_be_Fixed].setLeft(entryID);
                          (*mpEntries)[ID1_to_be_Fixed].setRight(entryID);
                          (*mpEntries)[entryID].setRight(ID2_to_be_Fixed); 
                          (*mpEntries)[entryID].setLeft(ID1_to_be_Fixed);
                      }
                  } 
                  TableauHead& rowHead = mRows[entry.rowNumber()];
                  ++rowHead.mSize;                      
                  if(columnHead.mStartEntry == columnA_iterator.entryID())
                  {
                      columnHead.mStartEntry = entryID;
                  }                  
              }
              else
              {
                  /*
                   * A new entry has to be created above the position of columnA_iterator
                   * and sideways to column_B_iterator.
                   */                   
                  EntryID entryID = newTableauEntry(T2(((multiple.content())*((*columnB_iterator).content().content()))));
                  TableauEntry<T2>& entry = (*mpEntries)[entryID];
                  entry.setColumnNumber((*columnA_iterator).columnNumber());
                  entry.setRowNumber((*columnB_iterator).rowNumber());
                  entry.setDown(columnA_iterator.entryID());
                  entry.setUp(LAST_ENTRY_ID);
                  (*columnA_iterator).setUp(entryID);
                  TableauHead& columnHead = mColumns[entry.columnNumber()];
                  ++columnHead.mSize;
                  Iterator row_iterator = Iterator(columnB_iterator.entryID(), mpEntries);
                  ID2_to_be_Fixed = row_iterator.entryID();
                  if((*row_iterator).columnNumber() > entry.columnNumber())
                  {
                      /*
                       * The new entry is left from the added entry.
                       * Search for the entries which have to be modified.
                       */                      
                      while((*row_iterator).columnNumber() > entry.columnNumber() && !row_iterator.rowBegin())
                      {
                          ID1_to_be_Fixed = row_iterator.entryID();
                          row_iterator.left();                     
                          ID2_to_be_Fixed = row_iterator.entryID();
                      }
                      if((*row_iterator).columnNumber() > entry.columnNumber() && row_iterator.rowBegin())
                      {
                          (*mpEntries)[entryID].setLeft(LAST_ENTRY_ID);
                          (*mpEntries)[entryID].setRight(ID2_to_be_Fixed);
                          (*mpEntries)[ID2_to_be_Fixed].setLeft(entryID);
                          TableauHead& rowHead = mRows[(*columnB_iterator).rowNumber()];
                          rowHead.mStartEntry = entryID;                          
                      }                     
                      else
                      {
                          (*mpEntries)[ID2_to_be_Fixed].setRight(entryID);
                          (*mpEntries)[ID1_to_be_Fixed].setLeft(entryID);
                          (*mpEntries)[entryID].setLeft(ID2_to_be_Fixed); 
                          (*mpEntries)[entryID].setRight(ID1_to_be_Fixed);
                      }  
                  }  
                  else
                  {
                      /*
                       * The new entry is right from the added entry.
                       * Search for the entries which have to be modified.
                       */                      
                      while((*row_iterator).columnNumber() < entry.columnNumber() && !row_iterator.rowEnd())
                      {                             
                          ID1_to_be_Fixed = row_iterator.entryID();
                          row_iterator.right();     
                          ID2_to_be_Fixed = row_iterator.entryID();
                      }
                      if((*row_iterator).columnNumber() < entry.columnNumber() && row_iterator.rowEnd())
                      {
                          (*mpEntries)[entryID].setRight(LAST_ENTRY_ID);  
                          (*mpEntries)[entryID].setLeft(ID2_to_be_Fixed);
                          (*mpEntries)[ID2_to_be_Fixed].setRight(entryID);
                      }    
                      else
                      {                          
                          (*mpEntries)[ID2_to_be_Fixed].setLeft(entryID);
                          (*mpEntries)[ID1_to_be_Fixed].setRight(entryID);
                          (*mpEntries)[entryID].setRight(ID2_to_be_Fixed); 
                          (*mpEntries)[entryID].setLeft(ID1_to_be_Fixed);
                      }                      
                  }   
               TableauHead& rowHead = mRows[entry.rowNumber()];
               ++rowHead.mSize;                  
               }
               if(!columnB_iterator.columnBegin())
               {
                   columnB_iterator.up();
               }
               else
               { 
                   break;
               }    
           }
        }
        
        /**
         * Multiplies the row with index row_index by multiple.
         * 
         * @return 
         */        
        template<typename T1, typename T2> 
        void Tableau<T1,T2>::multiplyRow(size_t row_index,T2 multiple)
        {            
            Iterator row_iterator = Iterator(mRows.at(row_index).mStartEntry, mpEntries);
            while(true)
            { 
                T2 content = T2(((*row_iterator).content().content())*(multiple.content()));
                (*row_iterator).rContent() = content;
                if(!row_iterator.rowEnd())
                {
                    row_iterator.right();
                }
                else
                {
                    break;
                }
            }
        }
        
        /**
         * Calculates the scalarproduct of the row with index rowA from Tableau A with the column
         * with index columnB from Tableau B considering that the columns in B are permutated. 
         * 
         * @return   the value (T) of the scalarproduct.
         */        
        template<typename T1, typename T2> 
        T2 Tableau<T1,T2>::Scalar_Product(Tableau<T2>& A, Tableau<T2>& B,size_t rowA, size_t columnB, T2 lcm,std::vector<size_t>& diagonals,std::vector<size_t>& dc_positions) 
        {
            Iterator rowA_iterator = Iterator(A.mRows.at(rowA).mStartEntry,A.mpEntries);
            T2 result = T2(0);
            while(true)
            {
                Iterator columnB_iterator = Iterator(B.mColumns.at(columnB).mStartEntry,B.mpEntries);
                size_t actual_column = revert_diagonals((*rowA_iterator).columnNumber(),diagonals); 
                while(true)
                {
                    if(actual_column == position_DC((*columnB_iterator).rowNumber(),dc_positions))
                    {
                        result += (*rowA_iterator).content()*(*columnB_iterator).content()*lcm;
                        break;
                    }
                    if(columnB_iterator.columnBegin())
                    {
                        break;
                    }
                    else
                    {
                        columnB_iterator.up();
                    }
                }    
                if(rowA_iterator.rowEnd())
                {
                    break;
                }
                else
                {
                    rowA_iterator.right();
                }
            }
            #ifdef LRA_DEBUG_CUTS_FROM_PROOFS
            std::cout << result << std::endl;
            #endif
            return result;    
        }
        
        /**
         * Calculate the Hermite normal form of the calling Tableau. 
         * 
         * @return   the vector containing the indices of the diagonal elements.
         */        
        template<typename T1, typename T2> 
        void Tableau<T1,T2>::calculate_hermite_normalform(std::vector<size_t>& diagonals)
        { 
            for(size_t i=0;i<mColumns.size();i++)
            {
                diagonals.push_back(mColumns.size());
            }       
            Iterator row_iterator = Iterator(mRows.at(0).mStartEntry, mpEntries);
            bool first_free;
            bool first_loop;
            bool just_deleted; 
            /*
             * Iterate through all rows in order to construct the HNF.
             */
            for(size_t i=0;i<mRows.size();i++)
            {
                size_t elim_pos=mColumns.size(),added_pos=mColumns.size();
                EntryID added_entry,elim_entry;
                T2 elim_content, added_content;     
                row_iterator = Iterator(mRows.at(i).mStartEntry, mpEntries);
                size_t number_of_entries = mRows.at(i).mSize;
                first_loop = true;
                first_free = true;
                just_deleted = false;
                /*
                 * Count how many zero entries of diagonal columns are in the
                 * current row.
                 */
                size_t diag_zero_entries=0;
                for(size_t j=0;j<i;j++)
                {
                    Iterator diagonal_iterator = Iterator(mColumns.at(diagonals.at(j)).mStartEntry, mpEntries);
                    while((*diagonal_iterator).rowNumber() > i && !diagonal_iterator.columnBegin())
                    {
                        diagonal_iterator.up();
                    }
                    if((*diagonal_iterator).rowNumber() != i)
                    {
                        diag_zero_entries++;
                    }
                }
                /*
                 * Eliminate as many entries as necessary.
                 */
                while(number_of_entries + diag_zero_entries > i + 1)
                {    
                    if(just_deleted)
                    {
                        /*
                         * Move the iterator to the correct position if an entry
                         * has been deleted in the last loop run.
                         */
                        row_iterator = Iterator(added_entry, mpEntries);
                    }    
                    else if (!first_loop)
                    {
                        /*
                         * If no entry was deleted during the last loop run and it is not 
                         * the first loop run, correct the position of the iterators.
                         */                        
                        if((*mpEntries)[added_entry].columnNumber() > (*mpEntries)[elim_entry].columnNumber())
                        {
                            row_iterator = Iterator(elim_entry,mpEntries);
                        }    
                        else
                        {
                            row_iterator = Iterator(added_entry,mpEntries);
                        }    
                    }
                    /*
                     * Make all entries in the current row positive.
                     */
                    Iterator help_iterator = Iterator(mRows.at(i).mStartEntry, mpEntries);
                    while(true)
                    {
                        if((*help_iterator).content() < 0 && !isDiagonal((*help_iterator).columnNumber(),diagonals))
                        {
                            invertColumn((*help_iterator).columnNumber());
                        }
                        if(!help_iterator.rowEnd())
                        {
                            help_iterator.right();
                        }
                        else
                        {
                            break;
                        }
                    }
                    
                    while(elim_pos == added_pos)
                    { 
                        T2 content = (*mpEntries)[row_iterator.entryID()].content();
                        size_t column = (*mpEntries)[row_iterator.entryID()].columnNumber();   
                        if(!isDiagonal(column,diagonals))
                        {    
                            if(first_free)
                            {                                
                                elim_pos = column;
                                elim_content = content; 
                                added_pos = column;
                                added_content = content;
                                first_free = false;
                                added_entry = row_iterator.entryID();
                                elim_entry = row_iterator.entryID();
                            }
                            else
                            {
                                if(elim_content <= content)
                                {
                                    elim_pos = column;
                                    elim_content = content;  
                                    elim_entry = row_iterator.entryID();
                                }
                                else
                                {
                                    added_pos = column;
                                    added_content = content; 
                                    added_entry = row_iterator.entryID();
                                }
                             }
                        }                        
                        if(elim_pos == added_pos && !row_iterator.rowEnd())
                        {
                            row_iterator.right();  
                        }    
                    }
                    T2 floor_value = T2( elim_content / added_content ).floor();
                    #ifdef LRA_DEBUG_CUTS_FROM_PROOFS
                    std::cout << "floor_value = " << floor_value << std::endl;
                    std::cout << "added_content = " << added_content << std::endl;
                    std::cout << "elim_content = " << elim_content << std::endl;
                    std::cout << "T2((-1)*floor_value.content()*added_content.content()) = " << T2((-1)*floor_value.content()*added_content.content()) << std::endl;
                    #endif
                    addColumns(elim_pos,added_pos,T2((-1)*floor_value.content()));
                    #ifdef LRA_DEBUG_CUTS_FROM_PROOFS
                    std::cout << "Add " << (added_pos+1) << ". column to " << (elim_pos+1) << ". column:" << std::endl;
                    print();
                    #endif
                    number_of_entries = mRows.at(i).mSize; 
                    first_loop = false;
                    if(mod( elim_content, added_content ) == 0)
                    {
                        /*
                         * If the remain of the division is zero,
                         * the following addition will delete
                         * the entry with the ID elim_entry
                         */
                        just_deleted = true; 
                        first_free = true;
                        elim_pos = added_pos;
                        elim_entry = added_entry;
                    }    
                    else
                    {
                         just_deleted = false;  
                         first_free = true;
                         if(elim_pos < added_pos)
                         {
                             added_pos = elim_pos;
                         }    
                         else
                         {
                             elim_pos = added_pos;
                         }         
                    }
                }
                if(first_loop)
                {
                    /*
                     * The current row does not need any eliminations.
                     * So search manually for the diagonal element.
                     */
                    while(isDiagonal((*row_iterator).columnNumber(),diagonals))
                    {
                        row_iterator.right();                        
                    }
                    added_content = (*row_iterator).content();
                    added_pos = (*row_iterator).columnNumber();
                } 
                diagonals.at(i) = added_pos;                
                /*
                 *  Normalize row.
                 */
                row_iterator = Iterator(mRows.at(i).mStartEntry, mpEntries);
                while(true)
                {                  
                    if( ( (*row_iterator).columnNumber() != added_pos ) && ( isDiagonal((*row_iterator).columnNumber(),diagonals) ) && ( added_content <= carl::abs( (*row_iterator).content() ) ) )
                    {
                       /*
                        * The current entry has to be normalized because it´s
                        * in a diagonal column and greater or equal than the
                        * diagonal entry in the current row.
                        */
                        #ifdef LRA_DEBUG_CUTS_FROM_PROOFS
                        std::cout << "Normalize" << std::endl;
                        std::cout << (*mpEntries)[row_iterator.entryID()].columnNumber() << std::endl;
                        std::cout << diagonals.at(i) << std::endl;
                        #endif
                        T2 floor_value = T2( (*row_iterator).content() / added_content ).floor();
                        addColumns((*mpEntries)[row_iterator.entryID()].columnNumber(),
                                  diagonals.at(i),
                                  (-1)*(floor_value));
                        #ifdef LRA_DEBUG_CUTS_FROM_PROOFS
                        print();
                        #endif
                    }
                    if(!row_iterator.rowEnd())
                    {
                        row_iterator.right(); 
                    }
                    else
                    {
                        break;
                    }
                }                
            }  
        }   
        
        /*
         * Inverts the HNF matrix.
         * 
         * @return 
         */
        template<typename T1, typename T2> 
        void Tableau<T1,T2>::invert_HNF_Matrix(std::vector<size_t> diagonals)
        {
            /*
             * Iterate through the tableau beginning in the the last
             * column which only contains one element.
             */            
            for(int i=mRows.size()-1;i>=0;--i)
            {
                /*
                 * Move the iterator to the diagonal element in the current column
                 * and calculate the new content for it.
                 */
                Iterator column_iterator = Iterator(mColumns.at(diagonals.at(i)).mStartEntry, mpEntries);  
                while(!column_iterator.columnBegin())
                {
                    column_iterator.up();                    
                }
                (*column_iterator).rContent() = 1/(*column_iterator).content();
                bool entry_changed=false;
                /*
                 * Now change the other entries in the current column if necessary.
                 */
                if(!column_iterator.columnEnd())
                {
                    column_iterator.down();
                    entry_changed = true;
                }
                if(entry_changed)
                {
                    while(true)
                    {
                        entry_changed = false;
                        size_t j = i + 1;
                        T2 new_value = T2(0);
                        while(j < mRows.size())
                        {
                            Iterator column_iterator2 = Iterator(mColumns.at(diagonals.at(j)).mStartEntry, mpEntries);
                            while((*column_iterator2).rowNumber() > (*column_iterator).rowNumber() && !column_iterator2.columnBegin())
                            {
                                column_iterator2.up();
                            }
                            if((*column_iterator2).rowNumber() == (*column_iterator).rowNumber())
                            {
                                new_value -= (*column_iterator2).content();
                                entry_changed = true;
                            }
                            j++;
                        }
                        if(entry_changed)
                        {
                            (*column_iterator).rContent() = new_value;
                        }    
                        if(!column_iterator.columnEnd())
                        {
                            column_iterator.down();
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
        }
        
        /**
         * Checks whether a cut from proof can be constructed with the row with index row_index
         * in the DC_Tableau. 
         * 
         * @return true,    if the proof can be constructed.
         *         false,   otherwise   
         */
        template<typename T1, typename T2>
        bool Tableau<T1,T2>::create_cut_from_proof(Tableau<T2>& Inverted_Tableau, Tableau<T2>& DC_Tableau, size_t& row_index, T2& _lcm,std::vector<T2>& coefficients,std::vector<bool>& non_basics_proof, smtrat::Polynomial& cut,std::vector<size_t>& diagonals,std::vector<size_t>& dc_positions, Bound<T1, T2>*& upper_lower)
        {
            Value<T1> result = T2(0);
            Iterator row_iterator = Iterator(mRows.at(row_index).mStartEntry,mpEntries); 
            /*
             * Calculate H^(-1)*b 
             */
            size_t i;
            while(true)
            {
                i = revert_diagonals((*row_iterator).columnNumber(),diagonals);
                const Variable<T1, T2>& basic_var = *(DC_Tableau.mRows)[dc_positions.at(i)].mName;
                const Value<T1>& basic_var_assignment = basic_var.assignment();
                result += basic_var_assignment * (*row_iterator).content() * _lcm;                    
                if(row_iterator.rowEnd())
                {
                    break;
                }
                else
                {
                    row_iterator.right();
                }                
            }
            if( !result.mainPart().isInteger() )
            {
               // Calculate the lcm of all entries in the row with index row_index in the DC_Tableau
               Iterator row_iterator = Iterator(DC_Tableau.mRows.at(dc_positions.at(row_index)).mStartEntry,DC_Tableau.mpEntries);
               T2 lcm_row = T2(1);
               while(true)
               {
                   _lcm  = carl::lcm( _lcm, (*row_iterator).content() );
                   if(!row_iterator.rowEnd())
                   {
                       row_iterator.right();
                   }
                   else
                   {
                       break;
                   }                   
               }
               // Construct the Cut
               T2 product = T2(0);
               size_t i=0;
               while(i < Inverted_Tableau.mRows.size())
               {
                   product = Scalar_Product(Inverted_Tableau,DC_Tableau,row_index,i,_lcm,diagonals,dc_positions);
                   const Variable<T1, T2>& non_basic_var = *mColumns[diagonals.at(i)].mName;
                   if(product != 0)
                   {
                       cut += non_basic_var.expression() * (product.content() * (result.mainPart().denom().content() / lcm_row.content()));
                       coefficients.push_back( product/lcm_row );
                       non_basics_proof.push_back(true);
                   }
                   else
                   {
                       non_basics_proof.push_back(false);
                   }
                   ++i;
               }
               return true; 
            }
            else
            {                
                return false;                
            }
        }
        #endif
        
        #ifdef LRA_GOMORY_CUTS
        enum GOMORY_SET
        {
            J_PLUS,
            J_MINUS,
            K_PLUS,
            K_MINUS
        };

        /**
         * Creates a constraint referring to Gomory Cuts, if possible. 
         * 
         * @return NULL,    if the cut can´t be constructed;
         *         otherwise the valid constraint is returned.   
         */ 
        template<typename T1, typename T2>
        const smtrat::Constraint* Tableau<T1,T2>::gomoryCut( const T2& _ass, size_t _rowPosition, vector<const smtrat::Constraint*>& _constrVec )
        {     
            Iterator row_iterator = Iterator( mRows.at(_rowPosition).mStartEntry, mpEntries );
            std::vector<GOMORY_SET> splitting = std::vector<GOMORY_SET>();
            // Check, whether the conditions of a Gomory Cut are satisfied
            while( !row_iterator.rowEnd() )
            { 
                const Variable<T1, T2>& nonBasicVar = *mColumns[row_iterator->columnNumber()].mName;
                if( nonBasicVar.infimum() == nonBasicVar.assignment() || nonBasicVar.supremum() == nonBasicVar.assignment() )
                {
                    if( nonBasicVar.infimum() == nonBasicVar.assignment() )
                    {
                        if( (*row_iterator).content() < 0 ) splitting.push_back( J_MINUS );
                        else splitting.push_back( J_PLUS );         
                    }
                    else
                    {
                        if( (*row_iterator).content() < 0 ) splitting.push_back( K_MINUS );
                        else splitting.push_back( K_PLUS );
                    }
                }                                 
                else
                {
                    return NULL;
                }                               
                row_iterator.right();
            }
            // A Gomory Cut can be constructed              
            std::vector<T2> coeffs = std::vector<T2>();
            T2 coeff;
            T2 f_zero = _ass - T2( cln::floor1( cln::the<cln::cl_RA>( _ass.toCLN() ) ) );
            ex sum = ex();
            // Construction of the Gomory Cut 
            std::vector<GOMORY_SET>::const_iterator vec_iter = splitting.begin();
            row_iterator = Iterator( mRows.at(_rowPosition).mStartEntry, mpEntries );
            while( !row_iterator.rowEnd() )
            {                 
                const Variable<T1, T2>& nonBasicVar = *mColumns[row_iterator->columnNumber()].mName;
                if( (*vec_iter) == J_MINUS )
                {
                    T2 bound = nonBasicVar.infimum().limit().mainPart();
                    coeff = -( row_iterator->content() / f_zero);
                    _constrVec.push_back( nonBasicVar.infimum().pAsConstraint() );                    
                    sum += coeff*( nonBasicVar.expression() - bound );                   
                }                 
                else if( (*vec_iter) == J_PLUS )
                {
                    T2 bound = nonBasicVar.infimum().limit().mainPart();
                    coeff = row_iterator->content()/( 1 - f_zero );
                    _constrVec.push_back( nonBasicVar.infimum().pAsConstraint() );
                    sum += coeff*( nonBasicVar.expression() - bound );                   
                }
                else if( (*vec_iter) == K_MINUS )
                {
                    T2 bound = nonBasicVar.supremum().limit().mainPart();
                    coeff = -( row_iterator->content()/( 1 - f_zero ) );
                    _constrVec.push_back( nonBasicVar.supremum().pAsConstraint() );
                    sum += coeff * ( bound - nonBasicVar.expression() );                   
                }
                else if( (*vec_iter) == K_PLUS ) 
                {
                    T2 bound = nonBasicVar.supremum().limit().mainPart();
                    coeff = (*row_iterator).content()/f_zero;
                    _constrVec.push_back( nonBasicVar.supremum().pAsConstraint() );
                    sum += coeff * ( bound - nonBasicVar.expression() );
                }     
                coeffs.push_back( coeff );
                row_iterator.right();
                ++vec_iter;
            }            
            const smtrat::Constraint* gomory_constr = smtrat::Formula::newConstraint( sum-1, smtrat::CR_GEQ, smtrat::Formula::constraintPool().realVariables() );
            ex *psum = new ex( sum - gomory_constr->constantPart() );
            Value<T1>* bound = new Value<T1>( gomory_constr->constantPart() );
            Variable<T1, T2>* var = new Variable<T1, T2>( mHeight++, true, psum, mDefaultBoundPosition );
            (*var).addLowerBound( bound, mDefaultBoundPosition, gomory_constr );
            typename std::vector<T2>::const_iterator coeffs_iter = coeffs.begin();
            row_iterator = Iterator( mRows.at(_rowPosition).mStartEntry, mpEntries );
            mRows.push_back( TableauHead() );
            EntryID currentStartEntryOfRow = LAST_ENTRY_ID;
            EntryID leftID;            
            while( coeffs_iter != coeffs.end() )
            {
                const Variable<T1, T2>& nonBasicVar = *mColumns[row_iterator->columnNumber()].mName;
                EntryID entryID = newTableauEntry( *coeffs_iter );
                TableauEntry<T2>& entry = (*mpEntries)[entryID];
                entry.setColumnNumber( nonBasicVar.position() );
                entry.setRowNumber( mHeight - 1 );
                TableauHead& columnHead = mColumns[entry.columnNumber()];
                EntryID& columnStart = columnHead.mStartEntry;
                (*mpEntries)[columnStart].setDown( entryID );
                entry.setUp( columnStart );                
                columnStart = entryID;
                ++columnHead.mSize;
                if( currentStartEntryOfRow == LAST_ENTRY_ID )
                {
                    currentStartEntryOfRow = entryID;
                    entry.setLeft( LAST_ENTRY_ID );
                    leftID = entryID;
                }  
                else 
                {
                    (*mpEntries)[entryID].setLeft( leftID );
                    (*mpEntries)[leftID].setRight( entryID ); 
                    leftID = entryID;
                }
                ++coeffs_iter;
                row_iterator.right();
            }            
            (*mpEntries)[leftID].setRight( LAST_ENTRY_ID );
            TableauHead& rowHead = mRows[mHeight-1];
            rowHead.mStartEntry = currentStartEntryOfRow;
            rowHead.mSize = coeffs.size();
            rowHead.mName = var; 
            return gomory_constr;     
        }
        #endif

        /**
         *
         * @param _out
         * @param _maxEntryLength
         * @param _init
         */
        template<typename T1, typename T2>
        void Tableau<T1,T2>::printHeap( std::ostream& _out, int _maxEntryLength, const std::string _init ) const
        {
            for( EntryID pos = 1; pos < mpEntries->size(); ++pos )
            {
                std::cout << _init;
                printEntry( pos, _out, _maxEntryLength );
                _out << std::endl;
            }
        }

        /**
         *
         * @param _out
         * @param _entry
         * @param _maxEntryLength
         */
        template<typename T1, typename T2>
        void Tableau<T1,T2>::printEntry( EntryID _entry, std::ostream& _out, int _maxEntryLength ) const
        {
            _out << std::setw( 4 ) << _entry << ": ";
            std::stringstream out;
            if( _entry != LAST_ENTRY_ID )
            {
                out << (*mpEntries)[_entry].content();
                _out << std::setw( _maxEntryLength ) << out.str();
            }
            else
            {
                _out << std::setw( _maxEntryLength ) << "NULL";
            }
            _out << " at (";
            _out << std::setw( 4 ) << (*mpEntries)[_entry].rowNumber();
            _out << ",";
            _out << std::setw( 4 ) << (*mpEntries)[_entry].columnNumber();
            _out << ") [up:";
            _out << std::setw( 4 ) << (*mpEntries)[_entry].up();
            _out << ", down:";
            _out << std::setw( 4 ) << (*mpEntries)[_entry].down();
            _out << ", left:";
            _out << std::setw( 4 ) << (*mpEntries)[_entry].left();
            _out << ", right:";
            _out << std::setw( 4 ) << (*mpEntries)[_entry].right();
            _out << "]";
        }

        /**
         *
         * @param _out
         * @param _init
         */
        template<typename T1, typename T2>
        void Tableau<T1,T2>::printVariables( bool _allBounds, std::ostream& _out, const std::string _init ) const
        {
            _out << _init << "Basic variables:" << std::endl;
            for( typename std::vector<TableauHead>::const_iterator row = mRows.begin(); row != mRows.end(); ++row )
            {
                _out << _init << "  ";
                row->mName->print( _out );
                _out << "(" << row->mActivity << ")" << std::endl;
                if( _allBounds ) row->mName->printAllBounds( _out, _init + "                    " );
            }
            _out << _init << "Nonbasic variables:" << std::endl;
            for( typename std::vector<TableauHead>::const_iterator column = mColumns.begin(); column != mColumns.end(); ++column )
            {
                _out << _init << "  ";
                column->mName->print( _out );
                _out << "(" << column->mActivity << ")" << std::endl;
                if( _allBounds ) column->mName->printAllBounds( _out, _init + "                    " );
            }
        }

        #ifdef LRA_REFINEMENT
        /**
         *
         * @param _out
         * @param _init
         */
        template<typename T1, typename T2>
        void Tableau<T1,T2>::printLearnedBounds( const std::string _init, std::ostream& _out  ) const
        {
            for( auto learnedBound = mLearnedLowerBounds.begin(); learnedBound != mLearnedLowerBounds.end(); ++learnedBound )
            {
                for( auto premiseBound = learnedBound->second->premise->begin(); premiseBound != learnedBound->second->premise->end(); ++premiseBound )
                {
                    _out << _init;
                    _out << *(*premiseBound)->variable().pExpression();
                    (*premiseBound)->print( true, _out, true );
                    _out << std::endl;
                }
                _out << _init << "               | " << std::endl;
                _out << _init << "               V " << std::endl;
                _out << _init << *learnedBound->first->pExpression();
                learnedBound->second->nextWeakerBound->print( true, _out, true );
                _out << std::endl;
                _out << _init << *learnedBound->first->pExpression();
                learnedBound->second->newBound->print( true, _out, true );
                _out << std::endl << std::endl;
            }
            for( auto learnedBound = mLearnedUpperBounds.begin(); learnedBound != mLearnedUpperBounds.end(); ++learnedBound )
            {
                for( auto premiseBound = learnedBound->second->premise->begin(); premiseBound != learnedBound->second->premise->end(); ++premiseBound )
                {
                    _out << _init;
                    _out << *(*premiseBound)->variable().pExpression();
                    (*premiseBound)->print( true, _out, true );
                    _out << std::endl;
                }
                _out << _init << "               | " << std::endl;
                _out << _init << "               V " << std::endl;
                _out << _init << *learnedBound->first->pExpression();
                learnedBound->second->nextWeakerBound->print( true, _out, true );
                _out << std::endl;
                _out << _init << *learnedBound->first->pExpression();
                learnedBound->second->newBound->print( true, _out, true );
                _out << std::endl << std::endl;
            }
        }
        #endif

        /**
         *
         * @param _out
         * @param _maxEntryLength
         * @param _init
         */
        template<typename T1, typename T2>
        void Tableau<T1,T2>::print( std::ostream& _out, int _maxEntryLength, const std::string _init ) const
        {
            char     frameSign     = '-';
            int width = mWidth >= (unsigned) INT_MAX ? INT_MAX - 1 : (int) mWidth; 
            _out << _init << std::setw( _maxEntryLength * (width + 1) ) << std::setfill( frameSign ) << "" << std::endl;
            _out << _init << std::setw( _maxEntryLength ) << std::setfill( ' ' ) << "#";
            for( typename std::vector<TableauHead>::const_iterator column = mColumns.begin(); column != mColumns.end(); ++column )
            {
                std::stringstream out;
                out << *column->mName->pExpression();
                _out << std::setw( _maxEntryLength ) << out.str() + " #";
            }
            _out << std::endl;
            _out << _init << std::setw( _maxEntryLength * (width + 1) ) << std::setfill( '#' ) << "" << std::endl;
            _out << std::setfill( ' ' );
            for( typename std::vector<TableauHead>::const_iterator row = mRows.begin(); row != mRows.end(); ++row )
            {
                _out << _init;
                std::stringstream out;
                #ifdef LRA_NO_DIVISION
                if( !(row->mName->factor() == 1) )
                    out << "(" << row->mName->factor() << ")*(";
                #endif
                out << *row->mName->pExpression();
                #ifdef LRA_NO_DIVISION
                if( !(row->mName->factor() == 1) )
                    out << ")";
                #endif
                _out << std::setw( _maxEntryLength ) << out.str() + " #";
                Iterator rowIter = Iterator( row->mStartEntry, mpEntries );
                size_t currentColumn = 0;
                while( true )
                {
                    for( size_t i = currentColumn; i < (*rowIter).columnNumber(); ++i )
                    {
                        _out << std::setw( _maxEntryLength ) << "0 #";
                    }
                    std::stringstream out;
                    out << (*rowIter).content();
                    _out << std::setw( _maxEntryLength ) << out.str() + " #";
                    currentColumn = (*rowIter).columnNumber()+1;
                    if( rowIter.rowEnd() )
                    {
                        for( size_t i = currentColumn; i < mWidth; ++i )
                        {
                            _out << std::setw( _maxEntryLength ) << "0 #";
                        }
                        _out << std::endl;
                        break;
                    }
                    rowIter.right();
                }
            }
            _out << _init << std::setw( _maxEntryLength * (width + 1) ) << std::setfill( frameSign ) << "" << std::endl;
            _out << std::setfill( ' ' );
        }
    }    // end namspace lra
}    // end namspace smtrat

#endif   /* LRA_TABLEAU_H */
