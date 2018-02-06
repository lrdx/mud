#if !defined __TIME_UTILS_HPP__
#define __TIME_UTILS_HPP__

#include <chrono>
#include <functional>
#include <memory>
#include <list>
#include <unordered_map>
#include <map>

#define LOAD_LOG_FOLDER "../log/"
#define LOAD_LOG_FILE "boot_load.log"

namespace utils
{
	class CExecutionTimer
	{
	public:
		using call_t = std::function<void()>;
		using clock_t = std::chrono::high_resolution_clock;
		using duration_t = std::chrono::duration<double>;
		using shared_ptr = std::shared_ptr<CExecutionTimer>;

		CExecutionTimer() { start(); }
		void restart() { start(); }
		duration_t delta() const;
		const auto& start_time() const { return m_start_time; }

	private:
		void start() { m_start_time = clock_t::now(); }
		clock_t::time_point m_start_time;
	};

	inline CExecutionTimer::duration_t CExecutionTimer::delta() const
	{
		const auto stop_time = clock_t::now();
		return std::chrono::duration_cast<duration_t>(stop_time - m_start_time);
	}

	class CSteppedProfiler
	{
	public:
		class CExecutionStepProfiler
		{
		public:
			CExecutionStepProfiler(const std::string& name) : m_name(name), m_stopped(false) {}
			const auto& name() const { return m_name; }
			void stop();
			const auto& duration() const { return m_duration; }
			bool operator<(const CExecutionStepProfiler& right) const { return duration() > right.duration(); }

		private:
			std::string m_name;
			CExecutionTimer::duration_t m_duration;
			CExecutionTimer m_timer;
			bool m_stopped;
		};

		using step_t = std::shared_ptr<CExecutionStepProfiler>;

		CSteppedProfiler(const std::string& scope_name): m_scope_name(scope_name) {}
		~CSteppedProfiler();
		void next_step(const std::string& step_name);

		static bool step_t__less(const step_t& left, const step_t& right) { return left->operator <(*right); }

	private:
		void report() const;

		const std::string m_scope_name;
		std::list<step_t> m_steps;
		CExecutionTimer m_timer;
	};

	class ProfilingStatistics
	{
	public:
		struct StatItem
		{
			using stats_window_t = std::list<utils::CExecutionTimer::duration_t>;
			stats_window_t stats;

			utils::CExecutionTimer::duration_t min;
			utils::CExecutionTimer::duration_t max;
		};

		void add_measurement(const std::string& label, utils::CExecutionTimer::duration_t duration);

		struct ReportItem
		{
			ReportItem(const std::string& l, const double mn, const double mx, const double avg, const std::size_t ws, const double omn, const double omx);

			std::ostream& dump(std::ostream& os) const;

			std::string label;
			double min;
			double max;
			double average;
			std::size_t window_size;
			double overall_min;
			double overall_max;
		};

		void report(std::ostream& os) const;

	private:
		using stats_t = std::unordered_map<std::string, StatItem>;

		stats_t m_stats;
	};

	inline std::ostream& operator<<(std::ostream& os, const ProfilingStatistics::ReportItem& item)
	{
		return item.dump(os);
	}

	class Timer : public CExecutionTimer
	{
	public:
		Timer(const std::string& label, ProfilingStatistics& stats) : m_label(label), m_stats(stats) {}
		~Timer() { m_stats.add_measurement(m_label, delta()); }

	private:
		std::string m_label;
		ProfilingStatistics& m_stats;
	};

	class FakeTimer : public CExecutionTimer
	{
	public:
		FakeTimer(const std::string& label, ProfilingStatistics& stats) {}
	};

	class Profiler
	{
	public:
		static CExecutionTimer::shared_ptr create(const std::string& label, const bool fake);
	};

	extern ProfilingStatistics stats;
}

#endif // __TIME_UTILS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
