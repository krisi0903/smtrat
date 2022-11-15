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
#include <smtrat-common/statistics/Statistics.h>
#include <smtrat-cad/lifting/LiftingTree.h>
#include <smtrat-cad/projection/BaseProjection.h>

namespace smtrat
{
namespace cad
{
	class CADVOStatistics : public Statistics {
	private:
		/**
		 * @brief Index of the current run.
		 * Used to generate a prefix for the collected per-run statistics.
		 */
		std::size_t mCurrentRun = 0;
	public:
		template <typename T>
		void _add(std::string const& key, T&& value) {
			std::stringstream fullyQualifiedKey;
			fullyQualifiedKey << mCurrentRun << ":" << key;
			addKeyValuePair(fullyQualifiedKey.str(), value);
		}

        template <typename T>
        void _add(char const* key, T&& value) {
            std::string temp(key);
            this->_add(temp, value);
        }

		void nextRun() {
			mCurrentRun++;
		}

        template <typename Settings>
        void collectLiftingInformation(smtrat::cad::LiftingTree<Settings> const& tree) {
            size_t size = 0;
            for (auto iter = tree.mCheckingQueue.begin(); iter != tree.mCheckingQueue.end(); iter++) size++;
            _add("liftingPoints", size);
        }

        template <typename Settings>
        void collectProjectionInformation(smtrat::cad::BaseProjection<Settings> const& proj) {
 
            for (std::size_t level = 1; level <= proj.dim(); ++level) {
                std::stringstream key;
                key << "projection." << level << ".size";
                this->_add(key.str(), proj.size(level));
            }
            /*
            std::size_t sum = 0;
            DegreeCollector dc;
            for (std::size_t level = 1; level <= projection.mProjection.dim(); ++level) {
                sum += proj.size(level);
                for (std::size_t pid = 0; pid < projection.mProjection.size(level); ++pid) {
                    if (projection.mProjection.hasPolynomialById(level, pid)) {
                        const auto& p = projection.mProjection.getPolynomialById(level, pid);
                        dc(p);
                    }
                }
            }
            stats.add(prefix + "_size", sum);
            stats.add(prefix + "_deg_max", dc.degree_max);
            stats.add(prefix + "_deg_sum", dc.degree_sum);
            stats.add(prefix + "_tdeg_max", dc.tdegree_max);
            stats.add(prefix + "_tdeg_sum", dc.tdegree_sum);
            */
        }
	};
}
}

#endif
