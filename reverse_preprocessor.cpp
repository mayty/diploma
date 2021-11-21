#include "pch.h"
#include "reverse_preprocessor.h"
#include "helpers.h"

reverse_preprocessor::reverse_preprocessor(unsigned int sample_resolution)
{
	set_sample_resolution(sample_resolution);
}

void reverse_preprocessor::set_sample_resolution(unsigned int sample_resolution)
{
	max_value = (1ull << sample_resolution) - 1;
}

void reverse_preprocessor::set_reference(uint32_t reference)
{
	this->reference = reference;
}

uint32_t reverse_preprocessor::get_value(uint32_t preprocessed)
{
	reference = _get_value(preprocessed);
	return reference;
}

uint32_t reverse_preprocessor::_get_value(uint32_t preprocessed)
{
	uint64_t theta = get_min(reference, max_value - reference);
	if (preprocessed > theta * 2)
	{
		preprocessed -= theta;
		if (reference > max_value / 2)
		{
			return reference - preprocessed;
		}
		return reference + preprocessed;
	}
	if (preprocessed % 2 == 0)
	{
		return reference + preprocessed / 2;
	}
	preprocessed += 1;
	return reference - preprocessed / 2;
}
