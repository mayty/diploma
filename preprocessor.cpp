#include "pch.h"
#include "preprocessor.h"
#include <cstdlib>
#include "helpers.h"

uint64_t unit_delay_preprocesson::map(int64_t error)
{
	int64_t theta = get_min(preceeding_sample, max_value - preceeding_sample);
	if (error >= 0 && error <= theta)
	{
		return 2 * error;
	}
	if (error >= -theta && error < 0)
	{
		return (2 * std::abs(error)) - 1;
	}
	if (error > 0)
	{
		return theta + error;
	}
	return theta - error;
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
	max_value = (1ll << sample_size) - 1;
}

uint64_t unit_delay_preprocesson::get_preprocessed(uint32_t sample)
{
	int64_t signed_sample = sample;
	int64_t error = signed_sample - preceeding_sample;
	uint32_t mapped_error = map(error);
	preceeding_sample = sample;
	return mapped_error;
}

