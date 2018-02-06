#include "time_utils.hpp"

#include "logger.hpp"
#include "utils.h"

#include <sstream>
#include <iomanip>

namespace utils
{
	CSteppedProfiler::~CSteppedProfiler()
	{
		if (0 < m_steps.size())
		{
			m_steps.back()->stop();
		}
		report();
	}

	void CSteppedProfiler::next_step(const std::string& step_name)
	{
		if (0 < m_steps.size())
		{
			m_steps.back()->stop();
		}
		m_steps.push_back(step_t(new CExecutionStepProfiler(step_name)));
	}

	void CSteppedProfiler::report() const
	{
		FILE* flog;
		std::stringstream ss;
		ss << "Stepped profiler report for scope '" << m_scope_name << "'";
		if (0 == m_steps.size())
		{
			ss << ": it took ";
		}
		else
		{
			ss << ": " << std::endl;

			size_t number = 0;
			auto steps = m_steps;
			steps.sort(step_t__less);
			for (const auto& step : steps)
			{
				++number;
				ss << "  " << number << ". Step '" << step->name() << "' took "
					<< std::fixed << std::setprecision(6) << step->duration().count() << " second(s) ("
					<< std::fixed << std::setprecision(6) << (100*step->duration().count()/ m_timer.delta().count()) << "%)"
					<< ";" << std::endl;
			}

			ss << "Whole scope took ";
		}
		ss << std::fixed << std::setprecision(6) << m_timer.delta().count() << " second(s)";

		log("INFO: %s\n", ss.str().c_str());

		flog = fopen(LOAD_LOG_FOLDER LOAD_LOG_FILE, "w");
		if (!flog)
		{
			log("ERROR: Can't open file %s", LOAD_LOG_FOLDER LOAD_LOG_FILE);
			return;
		}

		fprintf(flog, "%s", ss.str().c_str());
		fclose(flog);
	}

	void CSteppedProfiler::CExecutionStepProfiler::stop()
	{
		if (m_stopped)
		{
			return;
		}
		m_duration = m_timer.delta();
		m_stopped = true;
	}

	void ProfilingStatistics::add_measurement(const std::string& label, utils::CExecutionTimer::duration_t duration)
	{
		auto& stats_window = m_stats[label];
		stats_window.stats.push_back(duration);

		if (stats_window.min == stats_window.min.zero()
			|| stats_window.min > duration)
		{
			stats_window.min = duration;
		}

		if (stats_window.max == stats_window.max.zero()
			|| stats_window.max < duration)
		{
			stats_window.max = duration;
		}

		while (100 < stats_window.stats.size())
		{
			stats_window.stats.pop_front();
		}
	}

	void ProfilingStatistics::report(std::ostream& os) const
	{
		StreamFlagsHolder holder(os);

		std::multimap<double, ReportItem, std::greater<double>> report;
		for (const auto& w : m_stats)
		{
			double sum = 0.0;
			double min = -1.0;
			double max = -1.0;
			for (const auto& m : w.second.stats)
			{
				const double duration = m.count();
				sum += duration;
				if (min < 0.0 || min > duration)
				{
					min = duration;
				}

				if (max < duration)
				{
					max = duration;
				}
			}
			const auto average = sum / w.second.stats.size();
			report.emplace(w.second.max.count(), ReportItem(w.first, min, max, average, w.second.stats.size(), w.second.min.count(), w.second.max.count()));
		}

		os << std::fixed << std::setprecision(6);
		os << "-- Report:" << std::endl;
		int i = 0;
		for (const auto& r : report)
		{
			++i;
			const auto& item = r.second;
			os << i << ". " << r.first << ": " << item << std::endl;
		}
	}

	ProfilingStatistics::ReportItem::ReportItem(const std::string& l,
		const double mn,
		const double mx,
		const double avg,
		const std::size_t ws,
		const double omn,
		const double omx):
		label(l),
		min(mn),
		max(mx),
		average(avg),
		window_size(ws),
		overall_min(omn),
		overall_max(omx)
	{
	}

	std::ostream& ProfilingStatistics::ReportItem::dump(std::ostream& os) const
	{
		return os << "(" << label << ": [" << min << "; " << max << "]/" << average << "{" << window_size << "}; [" << overall_min << "; " << overall_max << "])";
	}

	ProfilingStatistics stats;

	CExecutionTimer::shared_ptr Profiler::create(const std::string& label, const bool fake)
	{
		if (fake)
		{
			return std::make_shared<FakeTimer>(label, stats);
		}
		else
		{
			return std::make_shared<Timer>(label, stats);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
