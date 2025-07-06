#include "fft.h"
#include <math.h>
#include <string.h>
#include <assert.h>

float f;
float a;
uint8_t db[512];

// 静态函数声明
static inline Complex complex_add(Complex a, Complex b);
static inline Complex complex_sub(Complex a, Complex b);
static inline Complex complex_mul(Complex a, Complex b);
static void bit_reverse(Complex* data, uint16_t n);
static void fft_compute(Complex* data, uint16_t n);
static void fft_compute_magnitude(Complex* data, float* magnitude, uint16_t n);
static float amplitude_to_db(float amplitude, float db_floor);
static void map_db_to_uint8(const float* db_values, uint8_t* out_db, uint16_t length, float db_min, float db_max);

// ADC参数
#define ADC_REF_VOLTAGE 3.3f
#define ADC_MAX_VALUE 255.0f
#define DB_FLOOR -80.0f
#define DB_CEILING 0.0f

struct fft {
    int sample_rate;
    float *data;
    int size;
};


// 设置采样率
void fft_set_sample_rate(fft_t *fft, int rate) {
    if (fft) {
        fft->sample_rate = rate;
    }
}


float db_values[512];

FFT_Error fft_process(
    const uint8_t* adc_data,
    uint16_t length,
    float sample_rate,
    float* fundamental_freq_hz,
    float* fundamental_amp_v,
    uint8_t* spectrum_db)
{
    // 参数检查
    if (!adc_data || !fundamental_freq_hz || !fundamental_amp_v || !spectrum_db) {
        return FFT_ERROR_NULL_POINTER;
    }
    
    if (length == 0 || (length & (length - 1)) != 0) {
        return FFT_ERROR_INVALID_LENGTH;
    }
    
    if (sample_rate <= 0.0f) {
        return FFT_ERROR_INVALID_SAMPLE_RATE;
    }

    // 使用静态数组避免栈溢出（根据实际平台调整）
    static Complex fft_input[1024]; // 最大支持1024点
    static float magnitude[512];    // length/2
    
    if (length > 1024) {
        return FFT_ERROR_INVALID_LENGTH;
    }

    // 1. 输入预处理（去直流分量）
    float mean = 0.0f;
    for (uint16_t i = 0; i < length; i++) {
        mean += adc_data[i];
    }
    mean /= length;
    
    for (uint16_t i = 0; i < length; i++) {
        fft_input[i].real = (float)adc_data[i] - mean;
        fft_input[i].imag = 0.0f;
    }

    // 2. 执行FFT
    fft_compute(fft_input, length);

    // 3. 计算幅度谱
    fft_compute_magnitude(fft_input, magnitude, length);

	// 4. 找基频（跳过直流分量）
	uint16_t max_index = 1;
	float max_val = magnitude[1];
	for (uint16_t i = 2; i < length / 2; i++) {
		if (magnitude[i] > max_val) {
			max_val = magnitude[i];
			max_index = i;
		}
	}

	// 5. 输出峰峰值（Vpp）
	*fundamental_freq_hz = (float)max_index * sample_rate / (float)length;
	// 5. 优化幅值计算（时域峰峰值为主，频域校准为辅）
	float vpp_time_domain = 0.0f;
	float vmin = ADC_REF_VOLTAGE, vmax = 0.0f;

	// 时域峰峰值计算（跳过前5个采样点避免瞬态干扰）
	for (uint16_t i = 5; i < length; i++) {
		float voltage = adc_data[i] * (ADC_REF_VOLTAGE / ADC_MAX_VALUE);
		vmin = fminf(vmin, voltage);
		vmax = fmaxf(vmax, voltage);
	}
	vpp_time_domain = vmax - vmin;

	// 频域幅值校准（防止时域受噪声干扰）
	float vpp_freq_domain = (2.0f * max_val / (length/2)) * (ADC_REF_VOLTAGE/ADC_MAX_VALUE);

	// 智能选择最终结果（时域优先，频域兜底）
	if (vpp_time_domain > 0.1f * ADC_REF_VOLTAGE) { // 时域有效阈值
		*fundamental_amp_v = vpp_time_domain;
	} else {
		*fundamental_amp_v = vpp_freq_domain;
	}

    // 6. 计算dB谱
    db_values[length / 2];
    for (uint16_t i = 0; i < length / 2; i++) {
        float voltage = magnitude[i] * (ADC_REF_VOLTAGE / ADC_MAX_VALUE) / length;
        db_values[i] = amplitude_to_db(voltage, DB_FLOOR);
    }

    // 7. 映射到0-255
    map_db_to_uint8(db_values, spectrum_db, length / 2, DB_FLOOR, DB_CEILING);

    return FFT_OK;
}

// --- 静态函数实现 ---
static inline Complex complex_add(Complex a, Complex b) {
    return (Complex){a.real + b.real, a.imag + b.imag};
}

static inline Complex complex_sub(Complex a, Complex b) {
    return (Complex){a.real - b.real, a.imag - b.imag};
}

static inline Complex complex_mul(Complex a, Complex b) {
    return (Complex){
        a.real * b.real - a.imag * b.imag,
        a.real * b.imag + a.imag * b.real
    };
}

static void bit_reverse(Complex* data, uint16_t n) {
    uint16_t j = 0;
    for (uint16_t i = 0; i < n; ++i) {
        if (i < j) {
            Complex tmp = data[i];
            data[i] = data[j];
            data[j] = tmp;
        }
        uint16_t bit = n >> 1;
        while (j >= bit && bit > 0) {
            j -= bit;
            bit >>= 1;
        }
        j += bit;
    }
}

static void fft_compute(Complex* data, uint16_t n) {
    bit_reverse(data, n);

    for (uint16_t len = 2; len <= n; len <<= 1) {
        float angle = -2.0f * M_PI / len;
        Complex wlen = {cosf(angle), sinf(angle)};
        for (uint16_t i = 0; i < n; i += len) {
            Complex w = {1.0f, 0.0f};
            for (uint16_t j = 0; j < len / 2; ++j) {
                Complex u = data[i + j];
                Complex t = complex_mul(w, data[i + j + len / 2]);
                data[i + j] = complex_add(u, t);
                data[i + j + len / 2] = complex_sub(u, t);
                w = complex_mul(w, wlen);
            }
        }
    }
}

static void fft_compute_magnitude(Complex* data, float* magnitude, uint16_t n) {
    for (uint16_t i = 0; i < n / 2; i++) {
        float re = data[i].real;
        float im = data[i].imag;
        magnitude[i] = sqrtf(re * re + im * im);
    }
}

static float amplitude_to_db(float amplitude, float db_floor) {
    if (amplitude < 1e-6f) amplitude = 1e-6f;
    return 20.0f * log10f(amplitude);
}

static void map_db_to_uint8(const float* db_values, uint8_t* out_db, uint16_t length, float db_min, float db_max) {
    float db_range = db_max - db_min;
    for (uint16_t i = 0; i < length; i++) {
        float val = (db_values[i] - db_min) / db_range;
        if (val < 0.0f) val = 0.0f;
        if (val > 1.0f) val = 1.0f;
        out_db[i] = (uint8_t)(val * 255.0f);
    }
}