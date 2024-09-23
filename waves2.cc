/*
 * File: waves2.cpp
 *
 */

#include "unit_osc.h"

#define __fast_inline fast_inline

typedef enum {
    k_user_osc_param_shape = 0,
    k_user_osc_param_shiftshape,
    k_user_osc_param_id1,
    k_user_osc_param_id2,
    k_user_osc_param_id3,
    k_user_osc_param_id4,
    k_user_osc_param_id5,
    k_user_osc_param_id6,
    k_num_user_osc_param_id
};

static unit_runtime_desc_t s_desc;
static int32_t params[k_num_user_osc_param_id];

typedef struct State {
    float w0;
    float phase;
    const float *wave0;
    float shape;
    float shapez;
    float shiftshape;
    float lfo;
    float lfoz;
    uint8_t flags;
    /* parameter values */
    uint8_t w_index;
    float mod_int;
    uint32_t ev_t1;
    uint32_t ev_t2;
    float lfo_att;
    float amp_int;
    /* envelope internal variables */
    uint32_t ev_t;
    uint32_t ev_t1t2;
    float ev_value;
    float ev_slope1;
    float ev_slope2;
    float mod_init;
    /* actual intensities */
    float lfo_int;
    float ev_int;
} State;

enum {
    k_flags_none = 0,
    k_flag_reset = 1<<0,
    k_flag_wave0 = 1<<1,
    k_flag_envelope = 1<<2,
};

#define FLT_NOT_INITIALIZED M_E

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

__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc)
{
    if (!desc)
      return k_unit_err_undef;

    // Note: make sure the unit is being loaded to the correct platform/module target
    if (desc->target != unit_header.target)
      return k_unit_err_target;

    // Note: check API compatibility with the one this unit was built against
    if (!UNIT_API_IS_COMPAT(desc->api))
      return k_unit_err_api_version;

    // Check compatibility of samplerate with unit, for NTS-1 MKII should be 48000
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    // Check compatibility of frame geometry
    // Note: NTS-1 mkII oscillators can make use of the audio input depending on the routing options in global settings, see product documentation for details.
    if (desc->input_channels != 2 || desc->output_channels != 1)  // should be stereo input / mono output
      return k_unit_err_geometry;

    // Cache the runtime descriptor for later use
    s_desc = *desc;

    s_state.w0    = 0.f;
    s_state.phase = 0.f;
    s_state.wave0 = wavesA[0];
    s_state.shape = 0.f;
    s_state.shapez = FLT_NOT_INITIALIZED;
    s_state.lfoz = FLT_NOT_INITIALIZED;
    s_state.lfo_att = FLT_NOT_INITIALIZED;
    s_state.mod_int = 0;
    s_state.amp_int = 0;
    s_state.ev_t1 = 0;
    s_state.ev_t2 = 0;
    s_state.shiftshape = 0.f;
    s_state.flags = k_flags_none;

    return k_unit_err_none;
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
    s_state.ev_slope1 =  (s_state.ev_t1 != 0) ? 1. / s_state.ev_t1 : 0;
    s_state.ev_slope2 =  (s_state.ev_t2 != 0) ? -1. / s_state.ev_t2 : 0;
    s_state.ev_t1t2 = s_state.ev_t1 + s_state.ev_t2;
    s_state.ev_int = ev_int;
    s_state.mod_init = (ev_int < 0) ? -ev_int : 0;
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

__unit_callback void unit_render(const float * in, float * out, uint32_t frames)
{
    const unit_runtime_osc_context_t *ctxt = static_cast<const unit_runtime_osc_context_t *>(s_desc.hooks.runtime_context);
    const uint8_t flags = s_state.flags;
    s_state.flags = k_flags_none;
    if (flags & k_flag_wave0) {
        update_wave(s_state.w_index);
    }

    s_state.lfo = q31_to_f32(ctxt->shape_lfo);

    const float w0 = s_state.w0 = osc_w0f_for_note((ctxt->pitch)>>8, ctxt->pitch & 0xFF);
    float phase = (flags & k_flag_reset) ? 0.f : s_state.phase;
    const float *wave0 = s_state.wave0;
    float * __restrict y = out;
    const float * y_e = y + frames;

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
    float ev_value = (flags & k_flag_reset) ? 0 : s_state.ev_value;
    const float amp_int = s_state.amp_int;
    const uint32_t ev_t1 = s_state.ev_t1;
    const uint32_t ev_t1t2 = s_state.ev_t1t2;
    const float ev_slope1 = s_state.ev_slope1;
    const float ev_slope2 = s_state.ev_slope2;
    const float mod_init = s_state.mod_init;

    for (; y != y_e; ) {
        shapez = linintf(0.002f, shapez, shape);
        float mod_value = mod_init + ev_int * ev_value;
        float inv_width = powf(2.0, (shapez + lfo_max * lfoz + mod_value) * 3);
        float sig = my_osc_wave_scanf(wave0, phase, inv_width);
        sig *= 1 - amp_int + amp_int * ev_value;

        *(y++) = sig;

        phase += w0;
        phase -= (uint32_t)phase;
        lfoz += lfo_inc;

        if (ev_t < ev_t1) {
            ev_value += ev_slope1;
        } else if (ev_t == ev_t1) {
            ev_value = 1.;
        } else if (ev_t < ev_t1t2) {
            ev_value += ev_slope2;
        } else {
            ev_value = 0.;
        }
        ev_t++;
    }
    s_state.phase = phase;
    s_state.shapez = shapez;
    s_state.lfoz = lfoz;
    s_state.ev_t = ev_t;
    s_state.ev_value = ev_value;
}

__unit_callback void unit_note_on(uint8_t note, uint8_t velo)
{
    s_state.flags |= k_flag_reset;
}

__unit_callback void unit_note_off(uint8_t note)
{
    (void)params;
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value)
{
    params[id] = value;
    const float valf = param_val_to_f32(value);
    switch (id) {
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
        /* Envelope to amplitude intensity */
        s_state.amp_int = clip01f(value * 0.01f);
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

__unit_callback void unit_teardown() {
}

__unit_callback void unit_reset() {
}

__unit_callback void unit_resume() {
}

__unit_callback void unit_suspend() {
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
    return params[id];
}

__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
    return nullptr;
}

__unit_callback void unit_set_tempo(uint32_t tempo) {
}

__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter) {
}

__unit_callback void unit_all_note_off() {
}

__unit_callback void unit_pitch_bend(uint16_t bend) {
}

__unit_callback void unit_channel_pressure(uint8_t press) {
}

__unit_callback void unit_aftertouch(uint8_t note, uint8_t press) {
}
