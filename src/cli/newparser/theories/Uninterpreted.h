#pragma once

#include "../Common.h"
#include "AbstractTheory.h"
#include "ParserState.h"

namespace smtrat {
namespace parser {

struct UninterpretedTheory: public AbstractTheory {
	
	static bool convertTerm(const types::TermType& term, types::UninterpretedTheory::TermType& result);
	static bool convertArguments(const std::string& op, const std::vector<types::TermType>& arguments, std::vector<types::UninterpretedTheory::TermType>& result, TheoryError& errors);
	
	UninterpretedTheory(ParserState* state);
	
	bool declareVariable(const std::string& name, const carl::Sort& sort);
	bool declareFunction(const std::string& name, const std::vector<carl::Sort>& args, const carl::Sort& sort);
	
	bool handleITE(const FormulaT& ifterm, const types::TermType& thenterm, const types::TermType& elseterm, types::TermType& result, TheoryError& errors);
	
	struct EqualityGenerator: public boost::static_visitor<FormulaT> {
		FormulaT operator()(const carl::UVariable& lhs, const carl::UVariable& rhs) {
			return FormulaT(lhs, rhs, false);
		}
		FormulaT operator()(const carl::UVariable& lhs, const carl::UFInstance& rhs) {
			return FormulaT(lhs, rhs, false);
		}
		FormulaT operator()(const carl::UFInstance& lhs, const carl::UVariable& rhs) {
			return FormulaT(lhs, rhs, false);
		}
		FormulaT operator()(const carl::UFInstance& lhs, const carl::UFInstance& rhs) {
			return FormulaT(lhs, rhs, false);
		}
	};
	bool handleFunctionInstantiation(const carl::UninterpretedFunction& f, const std::vector<types::TermType>& arguments, types::TermType& result, TheoryError& errors);
	
	bool functionCall(const Identifier& identifier, const std::vector<types::TermType>& arguments, types::TermType& result, TheoryError& errors);
};
	
}
}