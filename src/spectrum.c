#include "spectrum.h"
#include "arm_math.h"
#include <math.h>
#include <string.h>

#define FFT_LEN 1024
#define FFT_BINS (FFT_LEN / 2 + 1)
#define CAPTURE_BUFFERS 2
#define DB_FLOOR (-80.0f)
#define DB_LUT_MIN_EXP (-27)
#define DB_LUT_EXP_COUNT (1 - DB_LUT_MIN_EXP)
#define DB_LUT_MANTISSA_COUNT 256

typedef enum { BUFFER_FREE, BUFFER_FILLING, BUFFER_READY, BUFFER_PROCESSING } buffer_state_t;
static float capture[CAPTURE_BUFFERS][2][FFT_LEN];
static volatile buffer_state_t state[CAPTURE_BUFFERS] = {BUFFER_FILLING, BUFFER_FREE};
static volatile int8_t ready_index = -1;
static uint8_t fill_index = 0;
static uint16_t capture_pos = 0;
static float window[FFT_LEN];
static float fft_input[FFT_LEN];
static float fft_output[FFT_LEN];
// Combined A-weighting and FFT amplitude-normalization power gain per bin.
// Calculated when the sample rate changes; the FFT path only multiplies by it.
#define SUPPORTED_RATE_COUNT 4
static const uint32_t supported_rates[SUPPORTED_RATE_COUNT] = {
    44100u, 48000u, 88200u, 96000u
};
static float a_weight_power_gain[SUPPORTED_RATE_COUNT][FFT_BINS];
static uint8_t power_to_level_lut[DB_LUT_EXP_COUNT][DB_LUT_MANTISSA_COUNT];
static float hann_sum;
static arm_rfft_fast_instance_f32 fft;
static bool initialized;
static bool discard_next_frame;
static uint8_t active_rate_index = 0;
static float sample_rate_hz = 44100.0f;

static const float band_edges[SPECTRUM_BAND_COUNT + 1] = {
    100.0f, 146.8f, 316.2f, 681.3f, 1647.5f, 4472.1f,
    12139.2f, 16000.0f
};

static float a_weight_gain(float f)
{
    const float f2 = f * f;
    const float numerator = 12194.0f * 12194.0f * f2 * f2;
    const float denominator = (f2 + 20.6f * 20.6f) *
        sqrtf((f2 + 107.7f * 107.7f) * (f2 + 737.9f * 737.9f)) *
        (f2 + 12194.0f * 12194.0f);
    if (denominator <= 0.0f) return 0.0f;
    return powf(10.0f, (2.0f + 20.0f * log10f(numerator / denominator)) / 20.0f);
}

static uint8_t power_to_level(float power)
{
    if (!(power > 0.0f)) return 0;
    union { float f; uint32_t u; } bits = { .f = power };
    const int exponent = (int)((bits.u >> 23) & 0xffu) - 127;
    if (exponent < DB_LUT_MIN_EXP) return 0;
    if (exponent > 0) return 100;
    const uint8_t mantissa = (uint8_t)((bits.u >> 15) & 0xffu);
    return power_to_level_lut[exponent - DB_LUT_MIN_EXP][mantissa];
}

void spectrum_init(void)
{
    if (arm_rfft_fast_init_f32(&fft, FFT_LEN) != ARM_MATH_SUCCESS) return;
    hann_sum = 0.0f;
    for (uint32_t i = 0; i < FFT_LEN; ++i) {
        window[i] = 0.5f - 0.5f * cosf(6.28318530718f * (float)i / (float)(FFT_LEN - 1));
        hann_sum += window[i];
    }
    const float normalization = 2.0f / hann_sum;
    for (uint8_t rate = 0; rate < SUPPORTED_RATE_COUNT; ++rate) {
        const float rate_hz = (float)supported_rates[rate];
        for (uint16_t bin = 0; bin < FFT_BINS; ++bin) {
            const float a_gain = a_weight_gain((float)bin * (rate_hz / (float)FFT_LEN));
            a_weight_power_gain[rate][bin] = normalization * normalization * a_gain * a_gain;
        }
    }
    // Precompute the approximate power-to-dBFS display conversion at startup.
    // Runtime FFT processing then uses a bit-field lookup instead of sqrt/log.
    for (int exponent = DB_LUT_MIN_EXP; exponent <= 0; ++exponent) {
        float exponent_scale = 1.0f;
        for (int e = 0; e > exponent; --e) exponent_scale *= 0.5f;
        for (uint16_t mantissa = 0; mantissa < DB_LUT_MANTISSA_COUNT; ++mantissa) {
            const float significand = 1.0f + (float)mantissa / 256.0f;
            const float power = significand * exponent_scale;
            const float dbfs = 10.0f * log10f(power);
            float scaled = (dbfs - DB_FLOOR) * (100.0f / -DB_FLOOR);
            if (scaled < 0.0f) scaled = 0.0f;
            if (scaled > 100.0f) scaled = 100.0f;
            power_to_level_lut[exponent - DB_LUT_MIN_EXP][mantissa] = (uint8_t)(scaled + 0.5f);
        }
    }
    initialized = true;
}

void spectrum_set_sample_rate(uint32_t sample_rate)
{
    for (uint8_t rate = 0; rate < SUPPORTED_RATE_COUNT; ++rate) {
        if (sample_rate == supported_rates[rate]) {
            active_rate_index = rate;
            sample_rate_hz = (float)supported_rates[rate];
            return;
        }
    }
}

void spectrum_feed(float left_q31, float right_q31)
{
    if (!initialized) return;
    if (state[fill_index] != BUFFER_FILLING) {
        const uint8_t other = fill_index ^ 1u;
        if (state[other] != BUFFER_FREE) return;
        fill_index = other;
        state[fill_index] = BUFFER_FILLING;
        capture_pos = 0;
    }
    capture[fill_index][0][capture_pos] = left_q31 * (1.0f / 2147483648.0f);
    capture[fill_index][1][capture_pos] = right_q31 * (1.0f / 2147483648.0f);
    if (++capture_pos < FFT_LEN) return;

    if (ready_index < 0) {
        state[fill_index] = BUFFER_READY;
        ready_index = (int8_t)fill_index;
        const uint8_t other = fill_index ^ 1u;
        if (state[other] == BUFFER_FREE) {
            fill_index = other;
            state[fill_index] = BUFFER_FILLING;
        }
    } else {
        // Discard this analysis frame when the previous one has not been consumed.
    }
    capture_pos = 0;
}

void spectrum_feed_block(const float *left_q31, const float *right_q31, uint32_t length)
{
    if (!initialized) return;
    for (uint32_t i = 0; i < length; ++i)
        spectrum_feed(left_q31[i], right_q31[i]);
}

static void calculate_channel(const float *samples, uint8_t bands[SPECTRUM_BAND_COUNT])
{
    for (uint32_t i = 0; i < FFT_LEN; ++i) fft_input[i] = samples[i] * window[i];
    arm_rfft_fast_f32(&fft, fft_input, fft_output, 0);
    float energy[SPECTRUM_BAND_COUNT] = {0};
    uint16_t counts[SPECTRUM_BAND_COUNT] = {0};
    const float bin_hz = sample_rate_hz / (float)FFT_LEN;
    const uint16_t nearest_100hz_bin = (uint16_t)(100.0f / bin_hz + 0.5f);
    for (uint16_t bin = 1; bin < FFT_BINS; ++bin) {
        const float frequency = (float)bin * bin_hz;
        uint8_t band = 0;
        if (frequency < band_edges[0]) {
            // Include only the FFT bin nearest 100Hz so the first band remains
            // visible even when the bin spacing skips over the 100Hz edge.
            if (bin != nearest_100hz_bin) continue;
        } else {
            while (band + 1 < SPECTRUM_BAND_COUNT && frequency >= band_edges[band + 1]) ++band;
            if (frequency >= band_edges[band + 1]) continue;
        }
        float real, imag;
        if (bin == FFT_LEN / 2) { real = fft_output[1]; imag = 0.0f; }
        else { real = fft_output[2 * bin]; imag = fft_output[2 * bin + 1]; }
        // |FFT|^2 times the precomputed A-weighting/normalization gain.
        energy[band] += (real * real + imag * imag) *
            a_weight_power_gain[active_rate_index][bin];
        counts[band]++;
    }
    for (uint8_t band = 0; band < SPECTRUM_BAND_COUNT; ++band)
        bands[band] = counts[band] ? power_to_level(energy[band]) : 0;
}

bool spectrum_process(uint8_t left[SPECTRUM_BAND_COUNT], uint8_t right[SPECTRUM_BAND_COUNT])
{
    if (!initialized || ready_index < 0) return false;
    const uint8_t index = (uint8_t)ready_index;
    ready_index = -1;
    // Keep the analysis frame cadence close to the old 2048-point FFT while
    // halving the worst-case transform work per pass.
    if (discard_next_frame) {
        discard_next_frame = false;
        state[index] = BUFFER_FREE;
        return false;
    }
    discard_next_frame = true;
    state[index] = BUFFER_PROCESSING;
    calculate_channel(capture[index][0], left);
    calculate_channel(capture[index][1], right);
    state[index] = BUFFER_FREE;
    return true;
}
