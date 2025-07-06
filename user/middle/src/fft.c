#include "fft.h"
#include <math.h>
#include <string.h>
#include <assert.h>

float f;
float a;
uint8_t db[512];

// ��̬��������
static inline Complex complex_add(Complex a, Complex b);
static inline Complex complex_sub(Complex a, Complex b);
static inline Complex complex_mul(Complex a, Complex b);
static void bit_reverse(Complex* data, uint16_t n);
static void fft_compute(Complex* data, uint16_t n);
static void fft_compute_magnitude(Complex* data, float* magnitude, uint16_t n);
static float amplitude_to_db(float amplitude, float db_floor);
static void map_db_to_uint8(const float* db_values, uint8_t* out_db, uint16_t length, float db_min, float db_max);

// ADC����
#define ADC_REF_VOLTAGE 3.3f
#define ADC_MAX_VALUE 255.0f
#define DB_FLOOR -80.0f
#define DB_CEILING 0.0f

struct fft {
    int sample_rate;
    float *data;
    int size;
};


// ���ò�����
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
    // �������
    if (!adc_data || !fundamental_freq_hz || !fundamental_amp_v || !spectrum_db) {
        return FFT_ERROR_NULL_POINTER;
    }
    
    if (length == 0 || (length & (length - 1)) != 0) {
        return FFT_ERROR_INVALID_LENGTH;
    }
    
    if (sample_rate <= 0.0f) {
        return FFT_ERROR_INVALID_SAMPLE_RATE;
    }

    // ʹ�þ�̬�������ջ���������ʵ��ƽ̨������
    static Complex fft_input[1024]; // ���֧��1024��
    static float magnitude[512];    // length/2
    
    if (length > 1024) {
        return FFT_ERROR_INVALID_LENGTH;
    }

    // 1. ����Ԥ����ȥֱ��������
    float mean = 0.0f;
    for (uint16_t i = 0; i < length; i++) {
        mean += adc_data[i];
    }
    mean /= length;
    
    for (uint16_t i = 0; i < length; i++) {
        fft_input[i].real = (float)adc_data[i] - mean;
        fft_input[i].imag = 0.0f;
    }

    // 2. ִ��FFT
    fft_compute(fft_input, length);

    // 3. ���������
    fft_compute_magnitude(fft_input, magnitude, length);

	// 4. �һ�Ƶ������ֱ��������
	uint16_t max_index = 1;
	float max_val = magnitude[1];
	for (uint16_t i = 2; i < length / 2; i++) {
		if (magnitude[i] > max_val) {
			max_val = magnitude[i];
			max_index = i;
		}
	}

	// 5. ������ֵ��Vpp��
	*fundamental_freq_hz = (float)max_index * sample_rate / (float)length;
	// 5. �Ż���ֵ���㣨ʱ����ֵΪ����Ƶ��У׼Ϊ����
	float vpp_time_domain = 0.0f;
	float vmin = ADC_REF_VOLTAGE, vmax = 0.0f;

	// ʱ����ֵ���㣨����ǰ5�����������˲̬���ţ�
	for (uint16_t i = 5; i < length; i++) {
		float voltage = adc_data[i] * (ADC_REF_VOLTAGE / ADC_MAX_VALUE);
		vmin = fminf(vmin, voltage);
		vmax = fmaxf(vmax, voltage);
	}
	vpp_time_domain = vmax - vmin;

	// Ƶ���ֵУ׼����ֹʱ�����������ţ�
	float vpp_freq_domain = (2.0f * max_val / (length/2)) * (ADC_REF_VOLTAGE/ADC_MAX_VALUE);

	// ����ѡ�����ս����ʱ�����ȣ�Ƶ�򶵵ף�
	if (vpp_time_domain > 0.1f * ADC_REF_VOLTAGE) { // ʱ����Ч��ֵ
		*fundamental_amp_v = vpp_time_domain;
	} else {
		*fundamental_amp_v = vpp_freq_domain;
	}

    // 6. ����dB��
    db_values[length / 2];
    for (uint16_t i = 0; i < length / 2; i++) {
        float voltage = magnitude[i] * (ADC_REF_VOLTAGE / ADC_MAX_VALUE) / length;
        db_values[i] = amplitude_to_db(voltage, DB_FLOOR);
    }

    // 7. ӳ�䵽0-255
    map_db_to_uint8(db_values, spectrum_db, length / 2, DB_FLOOR, DB_CEILING);

    return FFT_OK;
}

// --- ��̬����ʵ�� ---
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