#pragma once

#include "../Common.h"

namespace smtrat {
namespace parser {
namespace types {

	struct ArithmeticTheory  {
		typedef mpl::vector<Rational> ConstTypes;
		typedef mpl::vector<carl::Variable> VariableTypes;
		typedef mpl::vector<carl::Variable, Rational, Poly> ExpressionTypes;
		typedef mpl::vector<carl::Variable, Rational, Poly> TermTypes;
		typedef carl::mpl_variant_of<TermTypes>::type TermType;
	};
#ifdef PARSER_BITVECTOR
	struct BitvectorTheory {
		typedef mpl::vector<carl::BVTerm> ConstTypes;
		typedef mpl::vector<carl::BVVariable> VariableTypes;
		typedef mpl::vector<carl::BVTerm> ExpressionTypes;
		typedef mpl::vector<carl::BVTerm> TermTypes;
		typedef carl::mpl_variant_of<TermTypes>::type TermType;
	};
#endif
	struct CoreTheory {
		typedef mpl::vector<FormulaT, std::string> ConstTypes;
		typedef mpl::vector<carl::Variable> VariableTypes;
		typedef mpl::vector<FormulaT, std::string> ExpressionTypes;
		typedef mpl::vector<FormulaT, std::string> TermTypes;
		typedef carl::mpl_variant_of<TermTypes>::type TermType;
	};
	struct UninterpretedTheory {
		typedef mpl::vector<carl::UVariable, carl::UFInstance> ConstTypes;
		typedef mpl::vector<carl::UVariable> VariableTypes;
		typedef mpl::vector<carl::UVariable, carl::UFInstance> ExpressionTypes;
		typedef mpl::vector<carl::UVariable, carl::UFInstance> TermTypes;
		typedef carl::mpl_variant_of<TermTypes>::type TermType;
	};

	typedef carl::mpl_concatenate<
			ArithmeticTheory::ConstTypes,
#ifdef PARSER_BITVECTOR
			BitvectorTheory::ConstTypes,
#endif
			CoreTheory::ConstTypes,
			UninterpretedTheory::ConstTypes
		>::type ConstTypes;
	typedef carl::mpl_variant_of<ConstTypes>::type ConstType;
	
	typedef carl::mpl_concatenate<
			ArithmeticTheory::VariableTypes,
#ifdef PARSER_BITVECTOR
			BitvectorTheory::VariableTypes,
#endif
			CoreTheory::VariableTypes,
			UninterpretedTheory::VariableTypes
		>::type VariableTypes;
	typedef carl::mpl_variant_of<VariableTypes>::type VariableType;
	
	typedef carl::mpl_concatenate<
			ArithmeticTheory::ExpressionTypes,
#ifdef PARSER_BITVECTOR
			BitvectorTheory::ExpressionTypes,
#endif
			CoreTheory::ExpressionTypes,
			UninterpretedTheory::ExpressionTypes
		>::type ExpressionTypes;
	typedef carl::mpl_variant_of<ExpressionTypes>::type ExpressionType;
	
	typedef carl::mpl_concatenate<
			ArithmeticTheory::TermTypes,
#ifdef PARSER_BITVECTOR
			BitvectorTheory::TermTypes,
#endif
			CoreTheory::TermTypes,
			UninterpretedTheory::TermTypes
		>::type TermTypes;
	typedef carl::mpl_variant_of<TermTypes>::type TermType;
	
	typedef boost::variant<bool, std::string, carl::Variable, Integer, Rational, Poly, FormulaT, SExpressionSequence<types::ConstType>, carl::UVariable, carl::UFInstance, boost::spirit::qi::unused_type> AttributeValue;
	
	struct FunctionInstantiator {
		bool operator()(const std::vector<TermType>&, TermType&) {
			return false;
		}
	};
}
}
}