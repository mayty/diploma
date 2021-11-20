#include "pch.h"
#include "preprocessor.h"
#include <cstdlib>

uint64_t unit_delay_preprocesson::map(int64_t error, int64_t predicted)
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

uint64_t unit_delay_preprocesson::get_reference()
{
	return preceeding_sample;
}

unit_delay_preprocesson::unit_delay_preprocesson(unsigned int sample_size)
	:sample_size{ sample_size }
{
	if (sample_size == 0 || sample_size > 32)
	{
		throw std::exception{};
	}
	int64_t max = 0;
	for (int i = 0; i < sample_size; ++i)
	{
		max <<= 1;
		max |= 1;
	}
	max_value = max;
}

uint64_t unit_delay_preprocesson::get_preprocessed(unt64_t sample)
{
	preceeding_sample = sample;
	return sample;
}

