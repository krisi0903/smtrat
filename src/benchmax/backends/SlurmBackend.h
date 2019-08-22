#pragma once

#include "Backend.h"

#include <benchmax/logging.h>
#include <benchmax/utils/execute.h>

#include "slurm/SlurmSettings.h"
#include "slurm/SlurmUtilities.h"

#include <filesystem>
#include <future>
#include <mutex>
#include <regex>

namespace benchmax {

/**
 * Backend for the Slurm workload manager.
 * 
 * The execution model is as follows:
 * We create multiple jobs that each consists of multiple array jobs that each execute one slice of the task list.
 * One array job executes Settings::slice_size entries of the task list.
 * One job consists of Settings::array_size array jobs.
 * We start as many jobs as necessary.
 */
class SlurmBackend: public Backend {
private:
	/// A job consists of a tool, an input file, a base dir and results.
	using JobData = std::tuple<
		const Tool*,
		std::filesystem::path,
		BenchmarkResult
	>;
	
	/// All jobs.
	std::vector<JobData> mResults;
	/// Mutex for submission delay.
	std::mutex mSubmissionMutex;

	/// Parse the content of an output file.
	void parse_result_file(const std::filesystem::path& file) {
		BENCHMAX_LOG_DEBUG("benchmax.slurm", "Processing file " << file);
		std::ifstream in(file);
		std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		auto extension = file.extension();
		std::regex filere("# START ([0-9]+) #([^#]*)# END \\1 #(?:([^#]*)# END DATA \\1 #)?");

		auto reBegin = std::sregex_iterator(content.begin(), content.end(), filere);
		auto reEnd = std::sregex_iterator();
		for (auto i = reBegin; i != reEnd; ++i) {
			assert(std::stoi((*i)[1]) > 0);
			std::size_t id = std::stoull((*i)[1]) - 1;
			if (id >= mResults.size()) continue;
			auto& res = std::get<2>(mResults[id]);
			if (extension == ".out") {
				res.stdout = (*i)[2];
				res.exitCode = std::stoi(slurm::parse_result_info((*i)[3], "exitcode"));
				res.time = std::chrono::milliseconds(std::stoi(slurm::parse_result_info((*i)[3], "time")));
				BENCHMAX_LOG_DEBUG("benchmax.slurm", "Got " << res << " for task " << id << " from stdout");
			} else if (extension == ".err") {
				res.stderr = (*i)[2];
				BENCHMAX_LOG_DEBUG("benchmax.slurm", "Got " << res << " for task " << id << " from stderr");
			} else {
				BENCHMAX_LOG_WARN("benchmax.slurm", "Trying to parse output file with unexpected extension " << extension);
			}
		}
	}

	std::pair<std::size_t,std::size_t> get_job_range(std::size_t n) const {
		std::size_t job_size = settings_slurm().array_size * settings_slurm().slice_size;
		return std::make_pair(
			job_size * n,
			std::min(job_size * (n + 1), mResults.size())
		);
	}

	void store_job_id(int jobid) {
		std::ofstream out(settings_slurm().tmp_dir + "/slurmjob");
		out << jobid;
		out.close();
	}

	int load_job_id() {
		std::ifstream in(settings_slurm().tmp_dir + "/slurmjob");
		if (!in) {
			return -1;
		}
		int jobid;
		in >> jobid;
		in.close();
		return jobid;
	}

	void remove_job_id() {
		if( std::remove( (settings_slurm().tmp_dir + "/slurmjob").c_str() ) != 0 ){
			BENCHMAX_LOG_WARN("benchmax.slurm", settings_slurm().tmp_dir + "/slurmjob file could not be deleted");
		}
	}
 
	void run_job_async(std::size_t n, bool wait_for_termination) {
		if (load_job_id() != -1) {
			BENCHMAX_LOG_ERROR("benchmax.slurm", "Another job " << load_job_id() << " is still running in the specified temporary directory! If this is not the case, please delete " + settings_slurm().tmp_dir + "/slurmjob");
			return;
		}

		std::string jobsfilename = settings_slurm().tmp_dir + "/jobs-" + std::to_string(settings_core().start_time) + "-" + std::to_string(n+1) + ".jobs";
		slurm::generate_jobs_file(jobsfilename, get_job_range(n), mResults);

		auto submitfile = slurm::generate_submit_file_chunked({
			std::to_string(settings_core().start_time) + "-" + std::to_string(n),
			jobsfilename,
			settings_slurm().tmp_dir,
			settings_benchmarks().limit_time,
			settings_benchmarks().grace_time,
			settings_benchmarks().limit_memory,
			settings_slurm().array_size,
			settings_slurm().slice_size
		});

		BENCHMAX_LOG_INFO("benchmax.slurm", "Delaying for " << settings_slurm().submission_delay);
		{
			std::lock_guard<std::mutex> guard(mSubmissionMutex);
			std::this_thread::sleep_for(settings_slurm().submission_delay);
		}
		BENCHMAX_LOG_INFO("benchmax.slurm", "Submitting job now.");

		std::stringstream cmd;
		cmd << "sbatch";
		if (wait_for_termination) cmd << " --wait";
		cmd << " --array=1-" << std::to_string(settings_slurm().array_size);
		cmd << " " << settings_slurm().sbatch_options;
		cmd << " " + submitfile;
		BENCHMAX_LOG_DEBUG("benchmax.slurm", "Command: " << cmd.str());
		std::string output;
		call_program(cmd.str(), output, true);
		int jobid = slurm::parse_job_id(output);
		store_job_id(jobid);
		if (wait_for_termination) {
			BENCHMAX_LOG_INFO("benchmax.slurm", "Job terminated.");
		} else {
			BENCHMAX_LOG_INFO("benchmax.slurm", "Job " << jobid << " was scheduled.");
		}
	}

	bool collect_results() override {
		BENCHMAX_LOG_INFO("benchmax.slurm", "Check if job finished.");
		int jobid = load_job_id();
		if (jobid == -1) {
			BENCHMAX_LOG_ERROR("benchmax.slurm", "Jobid could not be determined!");
			return false;
		}
		if (!slurm::is_job_finished(jobid)) {
			BENCHMAX_LOG_WARN("benchmax.slurm", "Job " << jobid << " is not finished yet.");
			return false;
		}
		remove_job_id();

		BENCHMAX_LOG_INFO("benchmax.slurm", "Collecting results.");
		auto files = slurm::collect_result_files(settings_slurm().tmp_dir);
		for (const auto& f: files) {
			parse_result_file(f);
		}
		BENCHMAX_LOG_DEBUG("benchmax.slurm", "Parsed results.");
		for (auto& r: mResults) {
			addResult(std::get<0>(r), std::get<1>(r), std::get<2>(r));
		}
		if (settings_slurm().archive_log_file != "") {
			slurm::archive_log_files({
				settings_slurm().archive_log_file + "-" + std::to_string(settings_core().start_time) + ".tgz",
				settings_slurm().tmp_dir
			});
		}
		slurm::remove_log_files(files, !settings_slurm().keep_logs);

		return true;
	}
public:
	bool suspendable() const {
		return true;
	}
	/// Run all tools on all benchmarks using Slurm.
	void run(const Jobs& jobs, bool wait_for_termination) {
		for (const auto& [tool, file]: jobs.randomized()) {
			mResults.emplace_back(JobData { tool, file, BenchmarkResult() });
		}
		BENCHMAX_LOG_DEBUG("benchmax.slurm", "Gathered " << mResults.size() << " jobs");
		
		std::vector<std::future<void>> tasks;
		std::size_t count = mResults.size() / (settings_slurm().array_size * settings_slurm().slice_size) + 1;
		for (std::size_t i = 0; i < count; ++i) {
			tasks.emplace_back(std::async(std::launch::async,
				[i,wait_for_termination,this](){
					return run_job_async(i, wait_for_termination);
				}
			));
		}
		for (auto& f: tasks) {
			f.wait();
		}
		if (wait_for_termination) {
			BENCHMAX_LOG_DEBUG("benchmax.slurm", "All jobs terminated.");
		} else {
			BENCHMAX_LOG_DEBUG("benchmax.slurm", "All jobs scheduled.");
		}
	}
};

}