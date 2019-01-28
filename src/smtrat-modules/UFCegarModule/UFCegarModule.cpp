/**
 * @file UFCegarModule.cpp
 * @author Henrich Lauko <xlauko@mail.muni.cz>
 * @author Dominika Krejci <dominika.krejci@rwth-aachen.de>
 *
 * @version 2018-11-18
 * Created on 2018-11-18.
 */

#include "UFCegarModule.h"

#include <carl/formula/uninterpreted/UFInstanceManager.h>

namespace smtrat
{
    using carl::freshUninterpretedVariable;
    using carl::overloaded;
    using carl::SortManager;

    template<class Settings>
    UFCegarModule<Settings>::UFCegarModule(const ModuleInput* _formula, Conditionals& _conditionals, Manager* _manager):
        Module( _formula, _conditionals, _manager )
#ifdef SMTRAT_DEVOPTION_Statistics
        , mStatistics(Settings::moduleName)
#endif
    {
        const std::string sort_name = "_S";
        my_sort = SortManager::getInstance().addSort( sort_name );
    }

    template<class Settings>
    UFCegarModule<Settings>::~UFCegarModule()
    {}

    template<class Settings>
    bool UFCegarModule<Settings>::informCore( const FormulaT& /*_constraint*/ )
    {
        // Your code.
        return true; // This should be adapted according to your implementation.
    }

    template<class Settings>
    void UFCegarModule<Settings>::init()
    {
    }

    template<class Settings>
    auto UFCegarModule<Settings>::flatten(const FormulaT& formula) noexcept -> FormulaT
    {
        if (auto res = formula_store.find(formula); res != formula_store.end()) {
            return res->second;
        }

        const auto& ueq = formula.uequality();
        FormulaT eq{flatten(ueq.lhs()), flatten(ueq.rhs()), ueq.negated()};
        formula_store.emplace(formula, eq);
        return eq;
    }

    std::string flatten_name(const carl::UTerm& term);

    std::string flatten_name_impl(const carl::UVariable& var) {
        return var.variable().name();
    }

    std::string flatten_name_impl(const carl::UFInstance& ufi) {
        const auto& fn = ufi.uninterpretedFunction();

        auto concat = [&] (std::string res, const carl::UTerm& term) {
            return std::move(res) + '.' + flatten_name(term);
        };

        const auto &args = ufi.args();
        auto suffix = std::accumulate(args.begin(), args.end(), std::string(""), concat);

        return '(' + fn.name() + suffix + ')';
    }


    std::string flatten_name(const carl::UTerm& term) {
        return std::visit( [&] (const auto& var) -> std::string {
            return flatten_name_impl(var);
        }, term.asVariant());
    }

    template<class Settings>
    auto UFCegarModule<Settings>::flatten(const UTerm& term) noexcept -> UTerm
    {
        if (auto res = term_store.find(term); res != term_store.end()) {
            return res->second;
        }

        auto name = flatten_name(term);

        if (term.isUFInstance()) {
            const auto& ufi = term.asUFInstance();
            instances[ufi.uninterpretedFunction()].emplace(ufi);
        }

        UTerm flattened{ UVariable(freshUninterpretedVariable(name), my_sort) };
        term_store.emplace(term, flattened);
        term_store.emplace(flattened, flattened);
        return flattened;
    }

    template<class Settings>
    bool UFCegarModule<Settings>::addCore( ModuleInput::const_iterator _subformula )
    {
        carl::FormulaVisitor<FormulaT> visitor;

        auto flattened = visitor.visitResult( _subformula->formula(), [&] (const auto& formula) {
            if (formula.getType() == carl::UEQ) {
                return flatten(formula);
            } else {
                return formula;
            }
        } );

        addSubformulaToPassedFormula(flattened, _subformula->formula());
        return true;
    }

    template<class Settings>
    void UFCegarModule<Settings>::removeCore( ModuleInput::const_iterator /*_subformula*/ )
    {
    }

    template<class Settings>
    void UFCegarModule<Settings>::updateModel() const
    {
        mModel.clear();
        if( solverState() == Answer::SAT )
        {
            getBackendsModel();
        }
    }

    template<typename Settings>
    FormulaT UFCegarModule<Settings>::create_functional_contraint( FormulasT &&eqs,
                                                                   const carl::UTerm &lhs,
                                                                   const carl::UTerm &rhs ) noexcept
    {
        using carl::FormulaType;
        FormulaT consequence{ flatten(lhs), flatten(rhs), false };
        FormulaT conditions( FormulaType::AND, eqs );
        return FormulaT{ FormulaType::IMPLIES, conditions, consequence };
    }

    template<class Settings>
    bool UFCegarModule<Settings>::refine(const UFInstance& a, const UFInstance& b) noexcept
    {
        using carl::FormulaType;

        if (refined.count({a, b}))
            return false;

        auto args = std::make_pair(a.args().begin(), b.args().begin());
        auto end = a.args().end();

        auto record_instance = [&] (const auto& arg) {
            if (arg.isUFInstance()) {
                const auto& ufi = arg.asUFInstance();
                if (!instances[ufi.uninterpretedFunction()].count(ufi)) {
                    pending.emplace(ufi);
                }
            }
        };

        FormulasT eqs;
        for ( ; args.first != end; ++args.first, ++args.second ) {
            record_instance(*args.first);
            record_instance(*args.second);
            eqs.emplace_back(flatten(*args.first), flatten(*args.second), false);
        }

        addSubformulaToPassedFormula(create_functional_contraint( std::move(eqs), a, b ));

        refined.emplace(a, b);
        return true;
    }

    template<class Settings>
    bool UFCegarModule<Settings>::refine() noexcept {
        bool added_constraint = false;

        // generate functional consistency
        for (const auto& [function, list] : instances) {
            if (list.size() <= 1)
                continue;

            for (auto i = list.begin(); i != std::prev(list.end()); ++i) {
                for (auto j = std::next(list.begin()); j != list.end(); ++j) {
                    added_constraint |= refine(*i, *j);
                }
            }
        }

        for (auto&& ufi : pending) {
            instances[ufi.uninterpretedFunction()].insert(std::move(ufi));
        }

        pending.clear();

        return added_constraint;
    }

    template<class Settings>
    bool UFCegarModule<Settings>::refine_n_args(int count) noexcept {
        // Expects flattened input without nested function calls

        // TODO idea partial unrolling of arguments -
        // extend consistency only by one argument at a time
        // TODO expand according to previous model (functions satisfied in model)
        // TODO filter combinations

        bool constrained = false;
        for (const auto& [function, list] : instances) {
            if (list.size() <= 1)
                continue;

            for (auto i = list.begin(); i != std::prev(list.end()); ++i) {
                for (auto j = std::next(list.begin()); j != list.end(); ++j) {
                    auto a = *i;
                    auto b = *j;
                    if (refined.count({a, b}))
                        return false;

                    auto args = std::make_pair(a.args().begin(), b.args().begin());
                    auto end = a.args().end();

                    FormulasT eqs;

                    for (int n = 0; args.first != end && n < count; ++args.first, ++args.second, ++n) {
                        eqs.emplace_back(flatten(*args.first), flatten(*args.second), false);
                    }

                    addSubformulaToPassedFormula(create_functional_contraint( std::move(eqs), a, b ));

                    refined.emplace(a, b);
                    constrained = true;
                }
            }
        }

        return constrained;
    }

    template<typename Instances>
    bool flattened_function_calls(const Instances& instances) noexcept {
        for (const auto& [function, list] : instances) {
            for (const auto& inst : list) {
                for (const auto& arg : inst.args()) {
                    if (arg.isUFInstance()) {
                        return false; // not flattened call
                    }
                }
            }
        }

        return true;
    }

    template<class Settings>
    Answer UFCegarModule<Settings>::checkCore()
    {
        Answer result = runBackends();

        assert(flattened_function_calls(instances));
        if ( result == Answer::SAT ) {                          // expect flattened input
            if (refine_n_args(std::numeric_limits<int>::max())) // constrain all args
                result = runBackends();
        }

        /*if ( result == Answer::SAT ) { // expect flattened input
            int i = 1;
            bool refinable = true;

            do {
                refinable = refine_n_args(i++);
                if ( refinable )
                    result = runBackends();
            } while (result == Answer::UNSAT && refinable);
        }*/

        /* CEGAR iteration when input is not flattened:
        bool refinable = true;
        while (result == Answer::SAT && refinable) {
            if ( refinable = refine() )
                result = runBackends();
        }
        */

        if (result == Answer::UNSAT) {
            getInfeasibleSubsets();
        }

        return result;
    }
}

#include "Instantiation.h"
