/**
 * @file CADVOStatistics.h
 * @author kristian covic <kristian@krisnet.de>
 * 
 * Statistics object for measuring values to evaluate variable ordering heuristics (for my bachelors thesis.)
 * I originally did not want to create my own statistics object, since there is already NewCADStatistics in
 * smtrat-modules/NewCADModule/NewCADStatistics.h (which this file is based on). However, there were a few minor
 * issues (most importantly, that the name of the statistics object was the module name, so I would have to either)
 * pass down the module name or the complete statistics object into my variable ordering code.
 *
 * @version 2022-11-15
 * Created on 2022-11-15.
 */
#pragma once
#include <smtrat-common/smtrat-common.h>

#ifdef SMTRAT_DEVOPTION_Statistics
#include <chrono>
#include <smtrat-common/statistics/Statistics.h>
#include <smtrat-cad/lifting/LiftingTree.h>
#include <smtrat-cad/projection/BaseProjection.h>

namespace smtrat::cad {
    template <typename Settings>
    class CAD;
}
namespace smtrat::cad::variable_ordering
{

	class CADVOStatistics : public Statistics {
	private:
		/**
		 * @brief Index of the current run.
		 * Used to generate a prefix for the collected per-run statistics.
		 */
		std::size_t mCurrentRun = 0;
        std::map<std::string, std::chrono::time_point<std::chrono::high_resolution_clock>> start_times;
        typedef std::chrono::milliseconds timer_tick_interval;

        std::map<std::string, unsigned long long> choices;


        inline std::string statFullName(std::string const& name) {
            return (std::stringstream() << mCurrentRun << ':' << name).str();
        }
	public:
        enum DetailLevel {NORMAL = 0, HIGH = 1};

        constexpr static DetailLevel detail_level = HIGH;
		template <typename T>
		void _add(std::string const& key, T&& value) {
            SMTRAT_LOG_DEBUG("CADVOStatistics", "CADVOStatistics storing " << statFullName(key) << ":" << value);
			addKeyValuePair(statFullName(key), value);
		}

        template <typename T>
        void _add(char const* key, T&& value) {
            std::string temp(key);
            this->_add(temp, value);
        }

		void nextRun() {
            for (auto [key, value] : choicemap) {
                _add("choices." + key, value);
            }
            
			mCurrentRun++;
            choicemap.clear();
		}

        void startTimer(std::string const& _name) {
            std::string name = statFullName(_name);
            if (start_times.find(name) != start_times.end()) {
                throw std::invalid_argument("A timer with that name is already running");
            } else {
                start_times[name] = std::chrono::high_resolution_clock::now();
            }
        }

        void stopTimer(std::string const& _name) {
            std::string name = statFullName(_name);
            auto start = start_times.find(name);
            if (start == start_times.end()) {
                throw std::invalid_argument("No timer with that name has been started");
            } else {
                this->addKeyValuePair(name, std::chrono::duration_cast<timer_tick_interval>(std::chrono::high_resolution_clock::now() - start->second).count());
            }
        }

        void add(std::vector<carl::Variable> const& vo) {
            std::stringstream ss;
            bool first = true;
            for (carl::Variable const& v : vo) {
                if (!first) ss << ",";
                ss << v;
                first = false;
            }
            this->_add("ordering", ss.str());
        }

        void recordChoices(std::string name, unsigned long long choice) {
            if (!choicemap.contains(name)) { choicemap[name] = 1;}

            unsigned long long out = choicemap[name];
            if (__builtin_umulll_overflow(choice, this->choices, &out)) {
                choicemap[name] = 0;
            } else {
                choicemap[name] = out;
            }
        }

        template <typename Settings>
        void collectLiftingInformation(smtrat::cad::CAD<Settings> const& cad);

        template <typename Settings>
        void collectProjectionInformation(smtrat::cad::CAD<Settings> const& cad);

        void evaluateOrdering(std::vector<Poly> const& polys, std::vector<carl::Variable> const& ordering);
	};

    extern CADVOStatistics& cadVOStatistics;
}
#else

namespace smtrat::cad {
    template <typename Settings>
    class CAD;
}
namespace smtrat::cad::variable_ordering
{

	class CADVOStatistics {
	public:
        enum DetailLevel {NORMAL = 0, HIGH = 1};

        constexpr static DetailLevel detail_level = HIGH;
		template <typename T>
		inline constexpr void _add(std::string const& key, T&& value) {}

        template <typename T>
        inline constexpr void _add(char const* key, T&& value) {}

		inline constexpr void nextRun() {}

        inline constexpr void startTimer(std::string const& _name) {}

        inline constexpr void stopTimer(std::string const& _name) {}

        inline constexpr void add(std::vector<carl::Variable> const& vo) {}

        inline constexpr void recordChoices(unsigned long long choice) {}

        template <typename Settings>
        inline constexpr void collectLiftingInformation(smtrat::cad::CAD<Settings> const& cad) {}

        template <typename Settings>
        inline constexpr void collectProjectionInformation(smtrat::cad::CAD<Settings> const& cad) {}
        
        template <typename Settings>
        inline constexpr void evaluateOrdering(std::vector<carl::Variable> const& ordering);
	};

    extern CADVOStatistics& cadVOStatistics;
}
#endif
