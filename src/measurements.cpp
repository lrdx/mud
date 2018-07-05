#include "measurements.hpp"

BasicMeasurements::BasicMeasurements() :
	m_sum(0.0),
	m_global_sum(0.0),
	m_global_count(0)
{
}

void BasicMeasurements::add(const measurement_t& measurement)
{
	add_measurement(measurement);
	squeeze();
}

void BasicMeasurements::add_measurement(const measurement_t& measurement)
{
	m_measurements.push_front(measurement);
	m_sum += measurement.second;
	m_global_sum += measurement.second;
	++m_global_count;

	m_min.insert(measurement);
	m_max.insert(measurement);
}

void BasicMeasurements::squeeze()
{
	while (m_measurements.size() > window_size())
	{
		const auto& last_value = m_measurements.back();

		handler_remove(last_value.first);

		const auto min_i = m_min.find(last_value);
		m_min.erase(min_i);

		const auto max_i = m_max.find(last_value);
		m_max.erase(max_i);

		m_sum -= last_value.second;

		m_measurements.pop_back();
	}
}

constexpr std::size_t BasicMeasurements::WINDOW_SIZE;
constexpr BasicMeasurements::measurement_t BasicMeasurements::NO_VALUE;

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
