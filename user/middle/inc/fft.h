#ifndef __FFT_H__
#define __FFT_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// 复数结构体
typedef struct {
    float real;
    float imag;
} Complex;

// 错误码
typedef enum {
    FFT_OK = 0,
    FFT_ERROR_INVALID_LENGTH,
    FFT_ERROR_NULL_POINTER,
    FFT_ERROR_INVALID_SAMPLE_RATE
} FFT_Error;

typedef struct fft fft_t;


void fft_set_sample_rate(fft_t *fft, int rate);

extern float f;
extern float a;
extern uint8_t db[512];

// FFT处理函数
FFT_Error fft_process(
    const uint8_t* adc_data,
    uint16_t length,
    float sample_rate,
    float* fundamental_freq_hz,
    float* fundamental_amp_v,
    uint8_t* spectrum_db
);

#ifdef __cplusplus
}
#endif

#endif // __FFT_H__