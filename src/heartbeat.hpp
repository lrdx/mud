#ifndef __HEARTBEAT_HPP__
#define __HEARTBEAT_HPP__

#include "measurements.hpp"
#include "structs.h"

#include <functional>
#include <queue>
#include <list>
#include <memory>

class AbstractPulseAction
{
public:
	using shared_ptr = std::shared_ptr<AbstractPulseAction>;

	virtual ~AbstractPulseAction() {}

	virtual void perform(int pulse_number, int missed_pulses) = 0;
};

using pulse_t = int;

class Heartbeat
{
public:
	static constexpr long long ROLL_OVER_AFTER = 600 * 60 * PASSES_PER_SEC;

	using pulse_action_t = AbstractPulseAction::shared_ptr;

	class PulseStep
	{
	public:
		using StepMeasurement = LabelledMeasurements<std::size_t>;	// Label is the index in steps array

		PulseStep(const std::string& name, const int modulo, const int offset, const pulse_action_t& action);

		const auto& name() const { return m_name; }
		const auto modulo() const { return m_modulo; }
		const auto offset() const { return m_offset; }
		const auto& action() const { return m_action; }
		const auto off() const { return m_off; }
		const auto on() const { return !off(); }

		void turn_on() { m_off = false; }
		void turn_off() { m_off = true; }

		void add_measurement(const std::size_t index, const pulse_t pulse, const StepMeasurement::value_t value);

		const auto& stats() const { return m_measurements; }

	private:
		std::string m_name;
		int m_modulo;
		int m_offset;
		pulse_action_t m_action;

		bool m_off;
		StepMeasurement m_measurements;
	};

	using steps_t = std::vector<PulseStep>;
	using pulse_label_t = std::unordered_map<std::size_t, double>;
	using PulseMeasurement = LabelledMeasurements<pulse_label_t>;

	Heartbeat();

	void operator()(const int missed_pulses);

	auto pulse_number() const { return m_pulse_number; }
	auto global_pulse_number() const { return m_global_pulse_number; }
	long long period() const;

	const auto& stats() const { return m_measurements; }
	const auto& steps() const { return m_steps; }
	const auto& executed_steps() const { return m_executed_steps; }

	const std::string& step_name(const std::size_t index) const { return m_steps[index].name(); }

private:
	using steps_set_t = std::unordered_set<std::size_t>;

	void advance_pulse_numbers();
	void pulse(const int missed_pulses, pulse_label_t& label);

	steps_t m_steps;
	pulse_t m_pulse_number;
	unsigned long m_global_pulse_number;	// number of pulses since game start

	steps_set_t m_executed_steps;	// set of step indexes ever executed
	PulseMeasurement m_measurements;
};

#endif // __HEARTBEAT_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
