#pragma once
#include <cstdint>
#include <cstdlib>
#include <exception>

class preprocessor
{
public:
    virtual uint64_t get_preprocessed(uint64_t sample) = 0;
    virtual uint64_t get_reference() = 0;
    virtual ~preprocessor() = default;
};

class unit_delay_preprocesson : public preprocessor
{
private:
	int64_t max_value;
    int64_t min_value = 0;
	unsigned int sample_size;
    int64_t preceeding_sample = 0;
    uint64_t map(int64_t error);
public:
    unit_delay_preprocesson(unsigned int sample_size);
    uint64_t get_preprocessed(uint64_t sample) override;
    uint64_t get_reference() override;
	~unit_delay_preprocesson() override = default;
};
