/*
 * File: waves2.cpp
 *
 */

#include "userosc.h"

typedef struct State {
    float w0;
    float phase;
    const float *wave0;
    uint8_t w_index;
    uint8_t flags;
} State;

enum {
    k_flags_none = 0,
    k_flag_reset = 1<<0,
    k_flag_wave0 = 1<<1,
};

static State s_state;

inline void UpdateWaves(const uint8_t flags) {
    if (flags & k_flag_wave0) {
        static const uint8_t k_a_thr = k_waves_a_cnt;
        static const uint8_t k_b_thr = k_a_thr + k_waves_b_cnt;
        static const uint8_t k_c_thr = k_b_thr + k_waves_c_cnt;
        static const uint8_t k_d_thr = k_c_thr + k_waves_d_cnt;
        static const uint8_t k_e_thr = k_d_thr + k_waves_e_cnt;
        static const uint8_t k_f_thr = k_e_thr + k_waves_f_cnt;
        const float * const * table;
        uint8_t idx = s_state.w_index;
      
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
}

void OSC_INIT(uint32_t platform, uint32_t api)
{
    s_state.w0    = 0.f;
    s_state.phase = 0.f;
    s_state.wave0 = wavesB[7];
    s_state.flags = k_flags_none;
}

void OSC_CYCLE(const user_osc_param_t * const params,
               int32_t *yn,
               const uint32_t frames)
{
    const uint8_t flags = s_state.flags;
    s_state.flags = k_flags_none;
    UpdateWaves(flags);

    const float w0 = s_state.w0 = osc_w0f_for_note((params->pitch)>>8, params->pitch & 0xFF);
    float phase = (flags & k_flag_reset) ? 0.f : s_state.phase;
    const float *wave0 = s_state.wave0;
    q31_t * __restrict y = (q31_t *)yn;
    const q31_t * y_e = y + frames;


    for (; y != y_e; ) {
        float sig = osc_wave_scanf(wave0, phase);

        *(y++) = f32_to_q31(sig);

        phase += w0;
        phase -= (uint32_t)phase;
    }
    s_state.phase = phase;
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
    switch (index) {
    case k_user_osc_param_id1:
        {
            static const uint8_t cnt = k_waves_a_cnt + k_waves_b_cnt \
                +k_waves_c_cnt +k_waves_d_cnt +k_waves_e_cnt +k_waves_f_cnt;
            s_state.w_index = value % cnt;
            s_state.flags |= k_flag_wave0;
        }
        break;
    
    case k_user_osc_param_id2:
        break;
    
    case k_user_osc_param_id3:
        break;
    
    case k_user_osc_param_id4:
        break;
    
    case k_user_osc_param_id5:
        break;
    
    case k_user_osc_param_id6:
        break;
    
    case k_user_osc_param_shape:
        break;
    
    case k_user_osc_param_shiftshape:
        break;
    
    default:
        break;
  }
}

