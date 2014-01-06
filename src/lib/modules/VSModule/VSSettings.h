/* 
 * File:   VSSettings.h
 * Author: florian
 *
 * Created on 02 July 2013, 10:57
 */

#ifndef VSSETTINGS_H
#define	VSSETTINGS_H

#include "../../config.h"


namespace smtrat
{   
    struct VSSettings1
    {
        static const bool elimination_with_factorization                        = false;
        static const bool local_conflict_search                                 = false;
        static const bool use_strict_inequalities_for_test_candidate_generation = true;
        #ifdef SMTRAT_VS_VARIABLEBOUNDS
        static const bool use_variable_bounds                                   = true;
        #else
        static const bool use_variable_bounds                                   = false;
        #endif
        static const bool sturm_sequence_for_root_check                         = use_variable_bounds && false;
        static const bool check_conflict_for_side_conditions                    = false;
        static const bool incremental_solving                                   = true;
        static const bool infeasible_subset_generation                          = true;
        static const bool virtual_substitution_according_paper                  = false;
        static const bool prefer_equation_over_all                              = false;
        static const bool integer_variables                                     = false;
        static const bool real_variables                                        = true;
        static const bool assure_termination                                    = true;
    };
    
    struct VSSettings2 : VSSettings1
    {
        static const bool elimination_with_factorization                        = true;
    };
    
    struct VSSettings23 : VSSettings2
    {
        static const bool local_conflict_search                                 = true;
    };
    
    struct VSSettings234 : VSSettings23
    {
        static const bool check_conflict_for_side_conditions                    = true;
        static const bool prefer_equation_over_all                              = true;
    };
    
    struct VSSettings2346 : VSSettings234
    {
        static const bool check_conflict_for_side_conditions                    = true;
        static const bool prefer_equation_over_all                              = true;
        static const bool integer_variables                                     = true;
        static const bool real_variables                                        = false;
    };
    
    struct VSSettings2347 : VSSettings234
    {
        static const bool check_conflict_for_side_conditions                    = true;
        static const bool prefer_equation_over_all                              = true;
        static const bool integer_variables                                     = true;
        static const bool real_variables                                        = true;
        static const bool assure_termination                                    = false;
    };
    
    struct VSSettings2345 : VSSettings234
    {
        static const bool sturm_sequence_for_root_check                         = use_variable_bounds && true;
    };
    
    struct VSSettings235 : VSSettings23
    {
        static const bool sturm_sequence_for_root_check                         = use_variable_bounds && true;
    };
    
    struct VSSettings24 : VSSettings2
    {
        static const bool check_conflict_for_side_conditions                    = true;
        static const bool prefer_equation_over_all                              = true;
    };
    
    struct VSSettings245 : VSSettings24
    {
        static const bool sturm_sequence_for_root_check                         = use_variable_bounds && true;
    };
    
    struct VSSettings25 : VSSettings2
    {
        static const bool sturm_sequence_for_root_check                         = use_variable_bounds && true;
    };
    
    struct VSSettings3 : VSSettings1
    {
        static const bool local_conflict_search                                 = true;
    };
    
    struct VSSettings34 : VSSettings3
    {
        static const bool check_conflict_for_side_conditions                    = true;
        static const bool prefer_equation_over_all                              = true;
    };
    
    struct VSSettings345 : VSSettings34
    {
        static const bool sturm_sequence_for_root_check                         = use_variable_bounds && true;
    };
    
    struct VSSettings35 : VSSettings3
    {
        static const bool sturm_sequence_for_root_check                         = use_variable_bounds && true;
    };
    
    struct VSSettings4 : VSSettings1
    {
        static const bool check_conflict_for_side_conditions                    = true;
        static const bool prefer_equation_over_all                              = true;
    };
    
    struct VSSettings45 : VSSettings4
    {
        static const bool sturm_sequence_for_root_check                         = use_variable_bounds && true;
    };
    
    struct VSSettings5 : VSSettings1
    {
        static const bool sturm_sequence_for_root_check                         = use_variable_bounds && true;
    };
}

#endif	/* VSSETTINGS_H */
