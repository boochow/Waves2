/*
 * File: waves2.cpp
 *
 */

#include "userosc.h"

typedef struct State {
    float w0;
    float phase;
    const float *wave0;
    float shape;
    float shapez;
    float shiftshape;
    float lfo;
    float lfoz;
    float lfo_att;
    float lfo_int;
    float mod_int;
    float ev_int;
    uint32_t ev_t;
    uint32_t ev_t1;
    uint32_t ev_t2;
    uint32_t ev_t1t2;
    float ev_value;
    float ev_slope1;
    float ev_slope2;
    uint8_t w_index;
    uint8_t flags;
} State;

enum {
    k_flags_none = 0,
    k_flag_reset = 1<<0,
    k_flag_wave0 = 1<<1,
    k_flag_envelope = 1<<2,
};

#define FLT_NOT_INITIALIZED M_E
#define INT_NOT_INITIALIZED 0xa5a5

static State s_state;

inline void update_wave(uint8_t idx) {
    static const uint8_t k_a_thr = k_waves_a_cnt;
    static const uint8_t k_b_thr = k_a_thr + k_waves_b_cnt;
    static const uint8_t k_c_thr = k_b_thr + k_waves_c_cnt;
    static const uint8_t k_d_thr = k_c_thr + k_waves_d_cnt;
    static const uint8_t k_e_thr = k_d_thr + k_waves_e_cnt;
    static const uint8_t k_f_thr = k_e_thr + k_waves_f_cnt;
    const float * const * table;
      
    if (idx < k_a_thr) {
        table = wavesA;
    } else if (idx < k_b_thr) {
        table = wavesB;
        idx -= k_a_thr;
    } else if (idx < k_c_thr) {
        table = wavesC;
        idx -= k_b_thr;
    } else if (idx < k_d_thr) {
        table = wavesD;
        idx -= k_c_thr;
    } else if (idx < k_e_thr) {
        table = wavesE;
        idx -= k_d_thr;
    } else { // if (idx < k_f_thr) {
        table = wavesF;
        idx -= k_e_thr;
    }
    s_state.wave0 = table[idx];
}

void OSC_INIT(uint32_t platform, uint32_t api)
{
    s_state.w0    = 0.f;
    s_state.phase = 0.f;
    s_state.wave0 = wavesA[0];
    s_state.shape = 0.f;
    s_state.shapez = FLT_NOT_INITIALIZED;
    s_state.lfoz = FLT_NOT_INITIALIZED;
    s_state.lfo_att = FLT_NOT_INITIALIZED;
    s_state.ev_t1 = 0;
    s_state.ev_t2 = 0;
    s_state.shiftshape = 0.f;
    s_state.flags = k_flags_none;
}

inline void recalc_envelope() {
    float lfo_int = 1.0 - s_state.lfo_att;
    float ev_int = s_state.mod_int;
    float sum = lfo_int + fabsf(ev_int);

    if (sum > 1.0) {
        lfo_int = lfo_int / sum;
        ev_int = ev_int / sum;
    }

    lfo_int *= (1 - s_state.shape);
    s_state.lfo_int = lfo_int;

    ev_int *= (1 - s_state.shape);
    s_state.ev_slope1 =  (s_state.ev_t1 != 0) ? ev_int / s_state.ev_t1 : 0;
    s_state.ev_slope2 =  (s_state.ev_t2 != 0) ? -ev_int / s_state.ev_t2 : 0;
    s_state.ev_t1t2 = s_state.ev_t1 + s_state.ev_t2;
    s_state.ev_int = ev_int;
}

__fast_inline float my_osc_wave_scanf(const float *wave, const float phase, const float multi)
{
    float sig;
    float p0 = phase * multi;
    if (p0 >= 1.0) {
        float coeff = fasterpow2f(s_state.shiftshape * 7);
        sig = osc_wave_scanf(wave, p0 - 1.0);
        sig *= fastpowf((multi - p0) / (multi - 1), coeff);
    } else {
        sig = osc_wave_scanf(wave, p0);
    }
    return sig;
}

void OSC_CYCLE(const user_osc_param_t * const params,
               int32_t *yn,
               const uint32_t frames)
{
    const uint8_t flags = s_state.flags;
    s_state.flags = k_flags_none;
    if (flags & k_flag_wave0) {
        update_wave(s_state.w_index);
    }

    s_state.lfo = q31_to_f32(params->shape_lfo);

    const float w0 = s_state.w0 = osc_w0f_for_note((params->pitch)>>8, params->pitch & 0xFF);
    float phase = (flags & k_flag_reset) ? 0.f : s_state.phase;
    const float *wave0 = s_state.wave0;
    q31_t * __restrict y = (q31_t *) yn;
    const q31_t * y_e = y + frames;

    const float shape = s_state.shape;
    float shapez = s_state.shapez;

    float lfoz = s_state.lfoz;
    if (lfoz == FLT_NOT_INITIALIZED) {
        lfoz = s_state.lfo;
    }

    if (flags & k_flag_envelope) {
        recalc_envelope();
    }

    const float lfo_inc = (s_state.lfo - lfoz) / frames;
    const float lfo_max = s_state.lfo_int;

    uint32_t ev_t  = (flags & k_flag_reset) ? 0 : s_state.ev_t;
    const float ev_int = s_state.ev_int;
    float ev_value;
    if (flags & k_flag_reset) {
        ev_value = (ev_int > 0) ? 0 : -ev_int;
    } else {
        ev_value = s_state.ev_value;
    }
    const uint32_t ev_t1 = s_state.ev_t1;
    const uint32_t ev_t1t2 = s_state.ev_t1t2;
    const float ev_slope1 = s_state.ev_slope1;
    const float ev_slope2 = s_state.ev_slope2;

    for (; y != y_e; ) {
        shapez = linintf(0.002f, shapez, shape);
        float inv_width = powf(2.0, (shapez + lfo_max * lfoz + ev_value) * 3);
        float sig = my_osc_wave_scanf(wave0, phase, inv_width);

        *(y++) = f32_to_q31(sig);

        phase += w0;
        phase -= (uint32_t)phase;
        lfoz += lfo_inc;

        if (ev_t < ev_t1) {
            ev_value += ev_slope1;
        } else if (ev_t == ev_t1) {
            ev_value = (ev_int > 0) ? ev_int : 0;
        } else if (ev_t < ev_t1t2) {
            ev_value += ev_slope2;
        } else {
            ev_value = (ev_int > 0) ? 0 : -ev_int;
        }
        ev_t++;
    }
    s_state.phase = phase;
    s_state.shapez = shapez;
    s_state.lfoz = lfoz;
    s_state.ev_t = ev_t;
    s_state.ev_value = ev_value;
}

void OSC_NOTEON(const user_osc_param_t * const params)
{
    s_state.flags |= k_flag_reset;
}

void OSC_NOTEOFF(const user_osc_param_t * const params)
{
    (void)params;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
    const float valf = param_val_to_f32(value);
    switch (index) {
    case k_user_osc_param_id1:
        /* wave */
        {
            static const uint8_t cnt = k_waves_a_cnt + k_waves_b_cnt \
                +k_waves_c_cnt +k_waves_d_cnt +k_waves_e_cnt +k_waves_f_cnt;
            s_state.w_index = value % cnt;
            s_state.flags |= k_flag_wave0;
        }
        break;
    
    case k_user_osc_param_id2:
        /* ShapeMod Intensity */
        s_state.mod_int = clip1m1f((value - 100) * 0.01f);
        s_state.flags |= k_flag_envelope;
        break;
    
    case k_user_osc_param_id3:
        /* Mod Attack */
        s_state.ev_t1 = (powf(9, 0.01 * value) - 1) * k_samplerate;
        s_state.flags |= k_flag_envelope;
        break;
    
    case k_user_osc_param_id4:
        /* Mod Decay */
        s_state.ev_t2 = (powf(9, 0.01 * value) - 1) * k_samplerate;
        s_state.flags |= k_flag_envelope;
        break;
    
    case k_user_osc_param_id5:
        /* ShapeLFO Attenuation */
        s_state.lfo_att = clip01f(value * 0.01f);
        s_state.flags |= k_flag_envelope;
        break;
    
    case k_user_osc_param_id6:
        break;
    
    case k_user_osc_param_shape:
        s_state.shape = valf;
        if (s_state.shapez == FLT_NOT_INITIALIZED) {
            s_state.shapez = valf;
        }
        s_state.flags |= k_flag_envelope;
        break;
    
    case k_user_osc_param_shiftshape:
        s_state.shiftshape = valf;
        break;
    
    default:
        break;
  }
}
