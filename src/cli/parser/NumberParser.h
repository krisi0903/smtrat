#pragma once

#include "../config.h"
#define BOOST_SPIRIT_USE_PHOENIX_V3
#ifdef __VS
#pragma warning(push, 0)
#include <boost/spirit/include/qi.hpp>
#pragma warning(pop)
#else
#include <boost/spirit/include/qi.hpp>
#endif

#include "Common.h"
#include "ParserTypes.h"

namespace smtrat {
namespace parser {

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;
namespace px = boost::phoenix;

struct RationalPolicies : qi::real_policies<smtrat::Rational> {
    template <typename It, typename Attr>
    static bool parse_nan(It&, It const&, Attr&) { return false; }
    template <typename It, typename Attr>
    static bool parse_inf(It&, It const&, Attr&) { return false; }
};

struct IntegralParser : public qi::grammar<Iterator, Rational(), Skipper> {
    IntegralParser();
private:
    qi::rule<Iterator, Rational(), Skipper> integral;
    qi::uint_parser<Rational,2,1,-1> binary;
    qi::uint_parser<Rational,10,1,-1> numeral;
    qi::uint_parser<Rational,16,1,-1> hexadecimal;
};
  
struct DecimalParser : qi::real_parser<Rational, RationalPolicies> {};

}
}

namespace boost { namespace spirit { namespace traits {
    template<> inline void scale(int exp, smtrat::Rational& r) {
        if (exp >= 0)
            r *= carl::pow(smtrat::Rational(10), (unsigned)exp);
        else
            r /= carl::pow(smtrat::Rational(10), (unsigned)(-exp));
    }
    template<> inline bool is_equal_to_one(const smtrat::Rational& value) {
        return value == 1;
    }
	/**
	 * Specialization of standard implementation to fix compilation errors.
	 * Standard implementation looks like this:
	 * <code>return neg ? -n : n;</code>
	 * However, if using gmpxx <code>-n</code> and <code>n</code> have different types.
	 * This issue is fixed in this implementation.
	 */
	template<> inline smtrat::Rational negate(bool neg, const smtrat::Rational& n) {
		return neg ? smtrat::Rational(-n) : n;
	}
}}}