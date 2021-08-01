#pragma once
#include "Byte.h"

#ifdef MATHLIBRARY_EXPORTS
#define ENCODER_API __declspec(dllexport)
#else
#define ENCODER_API __declspec(dllimport)
#endif

extern "C" ENCODER_API size_t create_decoder();
extern "C" ENCODER_API void destroy_decoder(size_t handle);
extern "C" ENCODER_API void feed_data_to_decoder(size_t handle, BYTE * data, size_t data_size);
extern "C" ENCODER_API void decode_data(size_t handle);
extern "C" ENCODER_API size_t get_decoded_length(size_t handle);
extern "C" ENCODER_API void get_decoded_data(size_t handle, BYTE * data_buf, size_t data_size);
