#pragma once
#include <cstdint>
#include <cstdlib>
#include <exception>

class preprocessor
{
public:
    virtual uint64_t operator()(int64_t sample) = 0;
    virtual ~preprocessor() = default;
};

template<unsigned int sample_size>
class unit_delay_preprocesson : public preprocessor
{
private:
	constexpr static int64_t max_value = []() constexpr -> int64_t {
		int64_t max = 0;
		for (int i = 0; i < sample_size; ++i)
		{
			max <<= 1;
			max |= 1;
		}
		return max;
	}();
    constexpr static int64_t min_value = 0;
    int64_t preceeding_sample = 0;
    uint64_t map(int64_t error, int64_t predicted);
public:
    unit_delay_preprocesson();
    uint64_t operator()(int64_t sample) override;
	~unit_delay_preprocesson() override = default;
};


template<unsigned int sample_size>
uint64_t unit_delay_preprocesson<sample_size>::map(int64_t error, int64_t predicted)
{
	int64_t theta = min(predicted - min_value, max_value - predicted);
	if (0 <= error && error <= theta)
	{
		return 2 * error;
	}
	else if (-theta <= error && error < 0)
	{
		return (2 * std::abs(error)) - 1;
	}
	else
	{
		return theta + std::abs(error);
	}
}

template<unsigned int sample_size>
unit_delay_preprocesson<sample_size>::unit_delay_preprocesson()
{
	static_assert(sample_size > 0 && sample_size <= 32);
}

template<unsigned int sample_size>
uint64_t unit_delay_preprocesson<sample_size>::operator()(int64_t sample)
{
	int64_t prediction_error = sample - this->preceeding_sample;
	uint64_t mapped_sample = map(prediction_error, this->preceeding_sample);
	this->preceeding_sample = sample;
	if (mapped_sample != int32_t(mapped_sample))
	{
		throw std::exception{};
	}
	return mapped_sample;
}
