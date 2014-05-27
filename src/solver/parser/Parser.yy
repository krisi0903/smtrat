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
 * @file Parser.yy
 *
 * @author Florian Corzilius
 * @author Ulrich Loup
 * @since 2012-03-19
 * @version 2013-05-05
 */

%{ /* C/C++ Declarations */

#include <stdio.h>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <lib/Formula.h>
CLANG_WARNING_DISABLE("-Wsign-conversion")
CLANG_WARNING_DISABLE("-Wshorten-64-to-32")
CLANG_WARNING_DISABLE("-Wconversion")

%}

/*
 * yacc/bison Declarations
 */

// Require bison 2.3 or later
%require "2.3"

// Add debug output code to generated parser. disable this for release versions.
%debug

// Start symbol is named "start"
%start start

// write out a header file containing the token defines
%defines

// use newer C++ skeleton file
%skeleton "lalr1.cc"

// namespace to enclose parser in
%name-prefix="smtrat"

// Set the parser's class identifier
%define "parser_class_name" "Parser"

// Keep track of the current position within the input
%locations
%initial-action
{
    // initialize the initial location object
    @$.begin.filename = @$.end.filename = dv.pStreamname();
    struct YYLTYPE
    {
        int first_line;
        int first_column;
        int last_line;
        int last_column;
    };
};

/*
 * The driver is passed by reference to the parser and to the scanner. This
 * provides a simple but effective pure interface, not relying on global
 * variables.
 */
%parse-param { class Driver& dv }

// Verbose error messages
%error-verbose

// Symbols.

%union
{
   unsigned                                                       eval;
   std::string*                                                   sval;
   std::vector<std::string*>*                                     vsval;
   std::vector<std::pair<std::string, std::string>>*              vspval;
   const class Formula*                                           fval;
   smtrat::PointerSet<class Formula>*                             sfval;
   smtrat::PointerMultiSet<class Formula>*                        msfval;
   Polynomial*                                                    pval;
}

%token END	0	"end of file"
%token ASSERT CHECK_SAT PUSH POP
%token PLUS MINUS TIMES DIV
%token EQ LEQ GEQ LESS GREATER NEQ
%token AND OR NOT IFF XOR IMPLIES ITE LET AS TRUE FALSE
%token DECLARE_CONST DECLARE_FUN DECLARE_SORT
%token DEFINE_CONST DEFINE_FUN DEFINE_SORT
%token SET_INFO SET_OPTION SET_LOGIC
%token GET_INFO GET_OPTION
%token GET_VALUE GET_ASSIGNMENT GET_PROOF GET_UNSAT_CORE GET_ASSERTIONS
%token EXIT
%token <sval> SYM THEORY_VAR BOOLEAN_VAR NUM DEC KEY BIT HEX BIN

%token OB CB DB

%type <sval>   value
%type <pval>   poly polylistPlus polylistMinus polylistTimes polyOp
%type <fval>   form equation
%type <sfval>  formlist
%type <msfval> formmlist
%type <vsval>  symlist
%type <vspval> varlist
%type <eval>   relation naryOp

%{

#include "Driver.h"
#include "Scanner.h"
CLANG_WARNING_RESET

#undef yylex
#define yylex dv.pLexer()->lex

%}

%% /* SMT_RAT Grammar Rules */

start:
	commandlist

commandlist:
		commandlist command
	|	command

command:
		OB ASSERT form CB                       { dv.add( $3 ); }
	|	OB CHECK_SAT CB                         { dv.check(); }
	|	OB PUSH NUM CB                          { dv.push( *$3 ); delete $3; }
	|	OB POP NUM CB                           { dv.pop( *$3 ); delete $3; }
	| 	OB SET_LOGIC SYM CB                     { dv.setLogic( @3, *$3 ); delete $3; }
	|	OB SET_INFO KEY value CB                { dv.setInfo( *$3, *$4 ); delete $4; delete $3; }
	|	OB GET_INFO KEY CB                      { dv.getInfo( *$3 ); delete $3; }
	|	OB SET_OPTION KEY value CB              { dv.setOption( *$3, *$4 ); delete $4; delete $3; }
	|	OB GET_OPTION KEY CB                    { dv.getOption( *$3 ); delete $3; }
	|	OB GET_VALUE OB varlist CB CB           { dv.getValue( $4 ); }
	|	OB GET_ASSIGNMENT CB                    { dv.getAssignment(); }
	|	OB GET_PROOF CB                         { dv.getProof(); }
	|	OB GET_UNSAT_CORE CB                    { dv.getUnsatCore(); }
	|	OB GET_ASSERTIONS CB                    { dv.getAssertions(); }
	|	OB DECLARE_CONST SYM SYM CB             { dv.addVariable( @3, $3, $4 ); }
	| 	OB DECLARE_FUN SYM OB CB SYM CB         { dv.addVariable( @3, $3, $6 ); }
	| 	OB DECLARE_FUN SYM OB symlist CB SYM CB { error( @3, "Declaration of function with arguments is not allowed in supported logics!" );
                                                  delete $3; delete $7; dv.free( $5 ); }
    |   OB DECLARE_SORT SYM NUM CB              { error( @3, "Declaration of types are not allowed in supported logics!" );
                                                  delete $3; delete $4; }
	| 	OB DEFINE_FUN SYM OB CB SYM CB          { error( @3, "Definition of functions are not allowed in supported logics!" );
                                                  delete $3; delete $6; }
	| 	OB DEFINE_FUN SYM OB symlist CB SYM CB  { error( @3, "Definition of functions are not allowed in supported logics!" );
                                                  delete $3; delete $7; dv.free( $5 ); }
	| 	OB DEFINE_SORT SYM OB CB SYM CB         { error( @3, "Definition of types are not allowed in supported logics!" );
                                                  delete $3; delete $6; }
	| 	OB DEFINE_SORT SYM OB symlist CB SYM CB { error( @3, "Definition of types are not allowed in supported logics!" );
                                                  delete $3; delete $7; dv.free( $5 ); }
	|	OB EXIT CB

symlist:
        SYM         { vector< string* >* syms = new vector< string* >(); syms->push_back( $1 ); $$ = syms; }
    |   symlist SYM { $1->push_back( $2 ); $$ = $1; }
    
varlist:
        THEORY_VAR          { vector<pair<string, string>>* vars = new vector<pair<string, string>>();
                              vars->push_back( pair<string, string>( *$1, "Theory" ) ); delete $1; $$ = vars; }
    |   BOOLEAN_VAR         { vector< pair<string, string>>* vars = new vector<pair<string, string>>();
                              vars->push_back( pair<string, string>( *$1, "Boolean" ) ); delete $1; $$ = vars; }
    |   varlist THEORY_VAR  { $1->push_back( pair<string, string>( *$2, "Theory" ) ); delete $2; $$ = $1; }
    |   varlist BOOLEAN_VAR { $1->push_back( pair<string, string>( *$2, "Boolean" ) ); delete $2; $$ = $1; }

value:
        SYM   { $$ = $1; }
    |   NUM   { $$ = $1; }
    |   DEC   { $$ = $1; }
    |   TRUE  { $$ = new string( "true" ); }
    |   FALSE { $$ = new string( "false" ); }

form:
        BOOLEAN_VAR                   { $$ = dv.mkBoolean( @1, $1 ); }
    |   TRUE                          { $$ = dv.mkTrue(); }
    |   FALSE                         { $$ = dv.mkFalse(); }
    |   equation                      { $$ = $1; }
    |   OB relation poly poly CB      { $$ = dv.mkConstraint( $3, $4, $2 ); }
    |   OB AS SYM SYM CB              { error( @0, "\"as\" is not allowed in supported logics!" ); }
	|	OB NOT form CB                { $$ = dv.mkNegation( $3 ); }
	|	OB IMPLIES form form CB       { $$ = dv.mkImplication( $3, $4 ); }
    |   OB XOR formmlist CB           { $$ = dv.mkXor( $3 ); }
	|	OB naryOp formlist CB         { $$ = dv.mkFormula( $2, $3 ); }
    |   OB let OB bindlist CB form CB { $$ = $6; dv.popVariableStack(); }
    |   OB ITE form form form CB      { $$ = dv.mkIteInFormula( $3, $4, $5 ); }

formlist:
		form          { $$ = new PointerSet<Formula>(); $$->insert( $1 ); }
    |	formlist form { $1->insert( $2 ); $$ = $1; }
    
formmlist:
		form           { $$ = new PointerMultiSet<Formula>(); $$->insert( $1 ); }
    |	formmlist form { $1->insert( $2 ); $$ = $1; }

equation:
       OB EQ form form CB { PointerSet<Formula>* fs = new PointerSet<Formula>(); fs->insert( $3 ); fs->insert( $4 ); $$ = dv.mkFormula( smtrat::IFF, fs ); }
    |  OB EQ poly poly CB { $$ = dv.mkConstraint( $3, $4, (unsigned) Relation::EQ ); }


relation:
		LEQ     { $$ = (unsigned) Relation::LEQ; }
    |	GEQ     { $$ = (unsigned) Relation::GEQ; }
    |	LESS    { $$ = (unsigned) Relation::LESS; }
    |	GREATER { $$ = (unsigned) Relation::GREATER; }
    |	NEQ     { $$ = (unsigned) Relation::NEQ; }

naryOp:
		AND { $$ = smtrat::AND; }
    |	OR  { $$ = smtrat::OR; }
    |	IFF { $$ = smtrat::IFF; }

let:
        LET { dv.pushVariableStack(); }

bindlist:
		bind          { }
	|	bind bindlist { }

bind:
        OB SYM poly CB { dv.addTheoryBinding( @2, $2, $3 ); }
	|	OB SYM form CB { dv.booleanBinding( @2, $2, $3 ); }

poly:
        THEORY_VAR                 { $$ = dv.mkPolynomial( @1, $1 ); }
    |   DEC                        { $$ = new smtrat::Polynomial( dv.getRational( $1 ) ); }
    | 	NUM                        { $$ = new smtrat::Polynomial( smtrat::Rational( $1->c_str() ) ); delete $1; }
    |  	polyOp                     { $$ = $1; }
    |   OB ITE form poly poly CB   { $$ = dv.mkIteInExpr( @3, $3, $4, $5 ); }

polyOp:
		OB DIV poly poly CB       { assert( $4->isConstant() ); (*$3) *= smtrat::Polynomial( Rational( 1 ) / $4->trailingTerm()->coeff() ); $$ = $3; delete $4; }
	|	OB MINUS poly CB          { (*$3) *= smtrat::Polynomial( smtrat::Rational( -1 ) ); $$ = $3; }
	|	OB PLUS polylistPlus CB   { $$ = $3; }
	|	OB MINUS polylistMinus CB { $$ = $3; }
	|	OB TIMES polylistTimes CB { $$ = $3; }

polylistPlus:
		poly polylistPlus { (*$1) += (*$2); $$ = $1; delete $2; }
	|	poly              { $$ = $1; }

polylistMinus:
		poly polylistMinus { (*$1) -= (*$2); $$ = $1; delete $2; }
	|	poly poly          { (*$1) -= (*$2); $$ = $1; delete $2; }

polylistTimes:
		poly polylistTimes  { (*$1) *= (*$2); $$ = $1; delete $2; }
	|	poly                { $$ = $1; }

%% /* Additional Code */

void smtrat::Parser::error( const Parser::location_type& _loc, const std::string& _message )
{
    dv.error( _loc, _message );
}
