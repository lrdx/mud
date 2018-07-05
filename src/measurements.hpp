#ifndef __MEASUREMENTS_HPP__
#define __MEASUREMENTS_HPP__

#include <set>
#include <list>
#include <unordered_map>

class BasicMeasurements
{
public:
	using value_t = double;
	using measurement_t = std::pair<int, value_t>;

	static constexpr measurement_t NO_VALUE = measurement_t(-1, -1);

	BasicMeasurements();
	virtual ~BasicMeasurements() {}

	const auto window_size() const { return WINDOW_SIZE; }

	const auto window_sum() const { return m_sum; }
	const auto current_window_size() const { return m_measurements.size(); }

	const auto global_sum() const { return m_global_sum; }
	const auto global_count() const { return m_global_count; }

	const auto& min() const { return m_min; }
	const auto& max() const { return m_max; }

	void add(const measurement_t& measurement);

protected:
	virtual void handler_remove(const int) { /* do nothing by default */ }

private:
	using measurements_t = std::list<measurement_t>;

	class Min
	{
	public:
		bool operator()(const measurement_t& a, const measurement_t& b) const { return a.second < b.second; }
	};

	class Max
	{
	public:
		bool operator()(const measurement_t& a, const measurement_t& b) const { return a.second > b.second; }
	};

	using min_t = std::multiset<measurement_t, Min>;
	using max_t = std::multiset<measurement_t, Max>;

	static constexpr std::size_t WINDOW_SIZE = 100;

	void add_measurement(const measurement_t& measurement);
	void squeeze();

	measurements_t m_measurements;

	min_t m_min;
	max_t m_max;

	value_t m_sum;

	value_t m_global_sum;
	std::size_t m_global_count;
};

template <typename Label>
class LabelledMeasurements : public BasicMeasurements
{
public:
	using single_value_t = std::pair<Label, measurement_t>;

	LabelledMeasurements() :
		m_global_min(Label(), NO_VALUE),
		m_global_max(Label(), NO_VALUE)
	{
	}

	void add(const Label& label, const int pulse, const value_t value) { add(label, measurement_t(pulse, value)); }
	void add(const Label& label, const measurement_t measurement);

	const auto& global_min() const { return m_global_min; }
	const auto& global_max() const { return m_global_max; }

protected:
	using BasicMeasurements::add;

private:
	using pulse_to_label_t = std::unordered_map<int, Label>;

	virtual void handler_remove(const int pulse) override;

	pulse_to_label_t m_labels;

	single_value_t m_global_min;
	single_value_t m_global_max;
};

template <typename Label>
void LabelledMeasurements<Label>::add(const Label& label, const measurement_t measurement)
{
	BasicMeasurements::add(measurement);
	m_labels.emplace(measurement.first, label);

	if (m_global_min.second.second > measurement.second
		|| m_global_min.second == NO_VALUE)
	{
		m_global_min = std::move(single_value_t(label, measurement));
	}

	if (m_global_max.second.second < measurement.second
		|| m_global_max.second == NO_VALUE)
	{
		m_global_max = std::move(single_value_t(label, measurement));
	}
}

template <typename Label>
void LabelledMeasurements<Label>::handler_remove(const int pulse)
{
	m_labels.erase(pulse);
}

#endif // __MEASUREMENTS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

