#include "audio/AudioMixer.h"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cmath>
#include <algorithm>

namespace godot {

static constexpr float AUDIO_MIXER_PI = 3.14159265358979323846f;

void AudioMixer::_bind_methods() {
    ClassDB::bind_static_method("AudioMixer", D_METHOD("mix_buffers", "buffers", "master_gain"), &AudioMixer::mix_buffers, DEFVAL(1.0f));

    ClassDB::bind_static_method("AudioMixer", D_METHOD("bytes_to_vector2", "data"), &AudioMixer::bytes_to_vector2);
    ClassDB::bind_static_method("AudioMixer", D_METHOD("vector2_to_bytes", "data"), &AudioMixer::vector2_to_bytes);

    ClassDB::bind_static_method("AudioMixer", D_METHOD("run_effect_chain", "buffer", "components", "sample_rate"), &AudioMixer::run_effect_chain);
    ClassDB::bind_static_method("AudioMixer", D_METHOD("reset_effect_chain", "components"), &AudioMixer::reset_effect_chain);

    ClassDB::bind_static_method("AudioMixer", D_METHOD("apply_gain", "buffer", "gain_linear"), &AudioMixer::apply_gain);
    ClassDB::bind_static_method("AudioMixer", D_METHOD("apply_pan", "buffer", "pan"), &AudioMixer::apply_pan);
    ClassDB::bind_static_method("AudioMixer", D_METHOD("soft_clip_buffer", "buffer"), &AudioMixer::soft_clip_buffer);

    ClassDB::bind_static_method("AudioMixer", D_METHOD("mix_two", "a", "b"), &AudioMixer::mix_two);
    ClassDB::bind_static_method("AudioMixer", D_METHOD("mix_dry_wet", "dry", "wet", "wet_amount"), &AudioMixer::mix_dry_wet);

    ClassDB::bind_static_method("AudioMixer", D_METHOD("process_delay_line", "buffer", "state", "delay_samples", "feedback", "mix"), &AudioMixer::process_delay_line);

    ClassDB::bind_static_method("AudioMixer", D_METHOD("compute_biquad_lowpass", "cutoff_hz", "q", "sample_rate"), &AudioMixer::compute_biquad_lowpass);
    ClassDB::bind_static_method("AudioMixer", D_METHOD("compute_biquad_highpass", "cutoff_hz", "q", "sample_rate"), &AudioMixer::compute_biquad_highpass);
    ClassDB::bind_static_method("AudioMixer", D_METHOD("compute_biquad_bandpass", "center_hz", "q", "sample_rate"), &AudioMixer::compute_biquad_bandpass);
    ClassDB::bind_static_method("AudioMixer", D_METHOD("compute_biquad_peaking_eq", "center_hz", "q", "gain_db", "sample_rate"), &AudioMixer::compute_biquad_peaking_eq);
    ClassDB::bind_static_method("AudioMixer", D_METHOD("process_biquad", "buffer", "coeffs", "state"), &AudioMixer::process_biquad);

    ClassDB::bind_static_method("AudioMixer", D_METHOD("process_comb_filter", "buffer", "state", "delay_samples", "feedback", "damping"), &AudioMixer::process_comb_filter);
    ClassDB::bind_static_method("AudioMixer", D_METHOD("process_allpass_filter", "buffer", "state", "delay_samples", "gain"), &AudioMixer::process_allpass_filter);

    ClassDB::bind_static_method("AudioMixer", D_METHOD("process_saturation", "buffer", "drive", "mix"), &AudioMixer::process_saturation);
    ClassDB::bind_static_method("AudioMixer", D_METHOD("process_compressor", "buffer", "state", "threshold_db", "ratio", "attack_ms", "release_ms", "sample_rate"), &AudioMixer::process_compressor);
}

AudioMixer::AudioMixer() {}
AudioMixer::~AudioMixer() {}

PackedByteArray AudioMixer::mix_buffers(Array buffers, float master_gain) {

    if (buffers.is_empty()) return PackedByteArray();

    PackedByteArray result = buffers[0];
    float* result_ptr = reinterpret_cast<float*>(result.ptrw());

    int buffers_count = buffers.size();
    int max_size = result.size() / sizeof(float);

    for (int idx = 1; idx < buffers_count; idx++) {
        const PackedByteArray &other_buffer = buffers[idx];
        const float* other_ptr = reinterpret_cast<const float*>(other_buffer.ptr());

        for (int i = 0; i < max_size; i++) {
            result_ptr[i] += other_ptr[i];
        }
    }

    for (int i = 0; i < max_size; i++) {
        float sample = result_ptr[i] * master_gain;
        result_ptr[i] = AudioMixer::sample_soft_clip(sample);
    }

    return result;
}

PackedVector2Array AudioMixer::bytes_to_vector2(PackedByteArray data) {
    int frame_count = data.size() / (int)sizeof(float) / 2;
    PackedVector2Array result;
    result.resize(frame_count);

    const float* src = reinterpret_cast<const float*>(data.ptr());
    Vector2* dst = result.ptrw();

    for (int i = 0; i < frame_count; i++) {
        dst[i] = Vector2(src[i * 2], src[i * 2 + 1]);
    }
    return result;
}

PackedByteArray AudioMixer::vector2_to_bytes(PackedVector2Array data) {
    int frame_count = data.size();
    PackedByteArray result;
    result.resize(frame_count * 2 * sizeof(float));

    const Vector2* src = data.ptr();
    float* dst = reinterpret_cast<float*>(result.ptrw());

    for (int i = 0; i < frame_count; i++) {
        dst[i * 2] = src[i].x;
        dst[i * 2 + 1] = src[i].y;
    }
    return result;
}

PackedVector2Array AudioMixer::run_effect_chain(PackedVector2Array buffer, Array components, int sample_rate) {
    Array snapshot = components.duplicate(false);
    for (int i = 0; i < snapshot.size(); i++) {
        Object* comp = snapshot[i];
        if (comp == nullptr) continue;
        if (!comp->has_method("_process_audio")) continue;

        Variant enabled_v = comp->get("enabled");
        bool enabled = (enabled_v.get_type() == Variant::BOOL) ? bool(enabled_v) : true;
        if (!enabled) continue;

        buffer = comp->call("_process_audio", buffer, sample_rate);
    }
    return buffer;
}

void AudioMixer::reset_effect_chain(Array components) {
    Array snapshot = components.duplicate(false);
    for (int i = 0; i < snapshot.size(); i++) {
        Object* comp = snapshot[i];
        if (comp && comp->has_method("_reset_audio")) {
            comp->call("_reset_audio");
        }
    }
}

PackedVector2Array AudioMixer::apply_gain(PackedVector2Array buffer, float gain_linear) {
    int count = buffer.size();
    Vector2* ptr = buffer.ptrw();
    for (int i = 0; i < count; i++) {
        ptr[i] *= gain_linear;
    }
    return buffer;
}

PackedVector2Array AudioMixer::apply_pan(PackedVector2Array buffer, float pan) {
    pan = std::clamp(pan, -1.0f, 1.0f);
    float left_gain = (pan <= 0.0f) ? 1.0f : (1.0f - pan);
    float right_gain = (pan >= 0.0f) ? 1.0f : (1.0f + pan);

    int count = buffer.size();
    Vector2* ptr = buffer.ptrw();
    for (int i = 0; i < count; i++) {
        ptr[i].x *= left_gain;
        ptr[i].y *= right_gain;
    }
    return buffer;
}

PackedVector2Array AudioMixer::soft_clip_buffer(PackedVector2Array buffer) {
    int count = buffer.size();
    Vector2* ptr = buffer.ptrw();
    for (int i = 0; i < count; i++) {
        ptr[i].x = sample_soft_clip(ptr[i].x);
        ptr[i].y = sample_soft_clip(ptr[i].y);
    }
    return buffer;
}

inline float AudioMixer::sample_soft_clip(float sample) {
    if (sample > 1.0f) return 1.0f;
    if (sample < -1.0f) return -1.0f;
    return (3.0f * sample - (sample * sample * sample)) / 2.0f;
}

PackedVector2Array AudioMixer::mix_two(PackedVector2Array a, PackedVector2Array b) {
    int count = std::min(a.size(), b.size());
    PackedVector2Array result;
    result.resize(count);

    const Vector2* ap = a.ptr();
    const Vector2* bp = b.ptr();
    Vector2* rp = result.ptrw();

    for (int i = 0; i < count; i++) {
        rp[i] = ap[i] + bp[i];
    }
    return result;
}

PackedVector2Array AudioMixer::mix_dry_wet(PackedVector2Array dry, PackedVector2Array wet, float wet_amount) {
    wet_amount = std::clamp(wet_amount, 0.0f, 1.0f);
    int count = std::min(dry.size(), wet.size());
    PackedVector2Array result;
    result.resize(count);

    const Vector2* dp = dry.ptr();
    const Vector2* wp = wet.ptr();
    Vector2* rp = result.ptrw();

    for (int i = 0; i < count; i++) {
        rp[i] = dp[i] * (1.0f - wet_amount) + wp[i] * wet_amount;
    }
    return result;
}

Dictionary AudioMixer::process_delay_line(PackedVector2Array buffer, Dictionary state, int delay_samples, float feedback, float mix) {
    delay_samples = std::max(delay_samples, 1);

    PackedVector2Array ring = state.get("ring", PackedVector2Array());
    int pos = state.get("pos", 0);

    if (ring.size() != delay_samples) {
        ring = PackedVector2Array();
        ring.resize(delay_samples);
        pos = 0;
    }

    int count = buffer.size();
    PackedVector2Array output;
    output.resize(count);

    const Vector2* in_ptr = buffer.ptr();
    Vector2* out_ptr = output.ptrw();
    Vector2* ring_ptr = ring.ptrw();

    for (int i = 0; i < count; i++) {
        Vector2 delayed = ring_ptr[pos];
        Vector2 dry = in_ptr[i];

        ring_ptr[pos] = dry + delayed * feedback;
        pos = (pos + 1) % delay_samples;

        out_ptr[i] = dry * (1.0f - mix) + delayed * mix;
    }

    Dictionary new_state;
    new_state["ring"] = ring;
    new_state["pos"] = pos;

    Dictionary result;
    result["output"] = output;
    result["state"] = new_state;
    return result;
}

PackedFloat32Array AudioMixer::compute_biquad_lowpass(float cutoff_hz, float q, int sample_rate) {
    float w0 = 2.0f * AUDIO_MIXER_PI * cutoff_hz / (float)sample_rate;
    float cos_w0 = cosf(w0);
    float alpha = sinf(w0) / (2.0f * std::max(q, 0.0001f));

    float b0 = (1.0f - cos_w0) / 2.0f;
    float b1 = 1.0f - cos_w0;
    float b2 = (1.0f - cos_w0) / 2.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos_w0;
    float a2 = 1.0f - alpha;

    PackedFloat32Array coeffs;
    coeffs.push_back(b0 / a0);
    coeffs.push_back(b1 / a0);
    coeffs.push_back(b2 / a0);
    coeffs.push_back(a1 / a0);
    coeffs.push_back(a2 / a0);
    return coeffs;
}

PackedFloat32Array AudioMixer::compute_biquad_highpass(float cutoff_hz, float q, int sample_rate) {
    float w0 = 2.0f * AUDIO_MIXER_PI * cutoff_hz / (float)sample_rate;
    float cos_w0 = cosf(w0);
    float alpha = sinf(w0) / (2.0f * std::max(q, 0.0001f));

    float b0 = (1.0f + cos_w0) / 2.0f;
    float b1 = -(1.0f + cos_w0);
    float b2 = (1.0f + cos_w0) / 2.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos_w0;
    float a2 = 1.0f - alpha;

    PackedFloat32Array coeffs;
    coeffs.push_back(b0 / a0);
    coeffs.push_back(b1 / a0);
    coeffs.push_back(b2 / a0);
    coeffs.push_back(a1 / a0);
    coeffs.push_back(a2 / a0);
    return coeffs;
}

PackedFloat32Array AudioMixer::compute_biquad_bandpass(float center_hz, float q, int sample_rate) {
    float w0 = 2.0f * AUDIO_MIXER_PI * center_hz / (float)sample_rate;
    float cos_w0 = cosf(w0);
    float alpha = sinf(w0) / (2.0f * std::max(q, 0.0001f));

    float b0 = alpha;
    float b1 = 0.0f;
    float b2 = -alpha;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos_w0;
    float a2 = 1.0f - alpha;

    PackedFloat32Array coeffs;
    coeffs.push_back(b0 / a0);
    coeffs.push_back(b1 / a0);
    coeffs.push_back(b2 / a0);
    coeffs.push_back(a1 / a0);
    coeffs.push_back(a2 / a0);
    return coeffs;
}

PackedFloat32Array AudioMixer::compute_biquad_peaking_eq(float center_hz, float q, float gain_db, int sample_rate) {
    float w0 = 2.0f * AUDIO_MIXER_PI * center_hz / (float)sample_rate;
    float cos_w0 = cosf(w0);
    float A = sqrtf(powf(10.0f, gain_db / 20.0f));
    float alpha = sinf(w0) / (2.0f * std::max(q, 0.0001f));

    float b0 = 1.0f + alpha * A;
    float b1 = -2.0f * cos_w0;
    float b2 = 1.0f - alpha * A;
    float a0 = 1.0f + alpha / A;
    float a1 = -2.0f * cos_w0;
    float a2 = 1.0f - alpha / A;

    PackedFloat32Array coeffs;
    coeffs.push_back(b0 / a0);
    coeffs.push_back(b1 / a0);
    coeffs.push_back(b2 / a0);
    coeffs.push_back(a1 / a0);
    coeffs.push_back(a2 / a0);
    return coeffs;
}

Dictionary AudioMixer::process_biquad(PackedVector2Array buffer, PackedFloat32Array coeffs, PackedFloat32Array state) {
    Dictionary result;

    if (coeffs.size() != 5) {
        result["output"] = buffer;
        result["state"] = state;
        return result;
    }

    float b0 = coeffs[0], b1 = coeffs[1], b2 = coeffs[2], a1 = coeffs[3], a2 = coeffs[4];

    float z1l = 0.0f, z2l = 0.0f, z1r = 0.0f, z2r = 0.0f;
    if (state.size() == 4) {
        z1l = state[0]; z2l = state[1]; z1r = state[2]; z2r = state[3];
    }

    int count = buffer.size();
    PackedVector2Array output;
    output.resize(count);

    const Vector2* in_ptr = buffer.ptr();
    Vector2* out_ptr = output.ptrw();

    for (int i = 0; i < count; i++) {
        float xl = in_ptr[i].x;
        float yl = b0 * xl + z1l;
        z1l = b1 * xl - a1 * yl + z2l;
        z2l = b2 * xl - a2 * yl;

        float xr = in_ptr[i].y;
        float yr = b0 * xr + z1r;
        z1r = b1 * xr - a1 * yr + z2r;
        z2r = b2 * xr - a2 * yr;

        out_ptr[i] = Vector2(yl, yr);
    }

    PackedFloat32Array new_state;
    new_state.push_back(z1l);
    new_state.push_back(z2l);
    new_state.push_back(z1r);
    new_state.push_back(z2r);

    result["output"] = output;
    result["state"] = new_state;
    return result;
}

Dictionary AudioMixer::process_comb_filter(PackedVector2Array buffer, Dictionary state, int delay_samples, float feedback, float damping) {
    delay_samples = std::max(delay_samples, 1);

    PackedVector2Array ring = state.get("ring", PackedVector2Array());
    int pos = state.get("pos", 0);
    float damp_l = state.get("damp_l", 0.0f);
    float damp_r = state.get("damp_r", 0.0f);

    if (ring.size() != delay_samples) {
        ring = PackedVector2Array();
        ring.resize(delay_samples);
        pos = 0;
    }

    int count = buffer.size();
    PackedVector2Array output;
    output.resize(count);

    const Vector2* in_ptr = buffer.ptr();
    Vector2* out_ptr = output.ptrw();
    Vector2* ring_ptr = ring.ptrw();

    for (int i = 0; i < count; i++) {
        Vector2 delayed = ring_ptr[pos];
        
        damp_l = delayed.x * (1.0f - damping) + damp_l * damping;
        damp_r = delayed.y * (1.0f - damping) + damp_r * damping;

        ring_ptr[pos] = in_ptr[i] + Vector2(damp_l, damp_r) * feedback;
        pos = (pos + 1) % delay_samples;

        out_ptr[i] = delayed;
    }

    Dictionary new_state;
    new_state["ring"] = ring;
    new_state["pos"] = pos;
    new_state["damp_l"] = damp_l;
    new_state["damp_r"] = damp_r;

    Dictionary result;
    result["output"] = output;
    result["state"] = new_state;
    return result;
}

Dictionary AudioMixer::process_allpass_filter(PackedVector2Array buffer, Dictionary state, int delay_samples, float gain) {
    delay_samples = std::max(delay_samples, 1);

    PackedVector2Array ring = state.get("ring", PackedVector2Array());
    int pos = state.get("pos", 0);

    if (ring.size() != delay_samples) {
        ring = PackedVector2Array();
        ring.resize(delay_samples);
        pos = 0;
    }

    int count = buffer.size();
    PackedVector2Array output;
    output.resize(count);

    const Vector2* in_ptr = buffer.ptr();
    Vector2* out_ptr = output.ptrw();
    Vector2* ring_ptr = ring.ptrw();

    for (int i = 0; i < count; i++) {
        Vector2 delayed = ring_ptr[pos];
        Vector2 input = in_ptr[i];

        out_ptr[i] = delayed - input * gain;
        ring_ptr[pos] = input + delayed * gain;

        pos = (pos + 1) % delay_samples;
    }

    Dictionary new_state;
    new_state["ring"] = ring;
    new_state["pos"] = pos;

    Dictionary result;
    result["output"] = output;
    result["state"] = new_state;
    return result;
}

PackedVector2Array AudioMixer::process_saturation(PackedVector2Array buffer, float drive, float mix) {
    drive = std::max(drive, 0.0001f);
    float pre_gain = 1.0f + drive * 9.0f;
    float norm = tanhf(pre_gain);

    int count = buffer.size();
    PackedVector2Array output;
    output.resize(count);

    const Vector2* in_ptr = buffer.ptr();
    Vector2* out_ptr = output.ptrw();

    for (int i = 0; i < count; i++) {
        float shaped_l = tanhf(in_ptr[i].x * pre_gain) / norm;
        float shaped_r = tanhf(in_ptr[i].y * pre_gain) / norm;
        out_ptr[i].x = in_ptr[i].x * (1.0f - mix) + shaped_l * mix;
        out_ptr[i].y = in_ptr[i].y * (1.0f - mix) + shaped_r * mix;
    }
    return output;
}

Dictionary AudioMixer::process_compressor(PackedVector2Array buffer, Dictionary state, float threshold_db, float ratio,
                                           float attack_ms, float release_ms, int sample_rate) {
    float envelope = state.get("envelope", 0.0f);

    float attack_coeff = expf(-1.0f / (std::max(attack_ms, 0.01f) * 0.001f * sample_rate));
    float release_coeff = expf(-1.0f / (std::max(release_ms, 0.01f) * 0.001f * sample_rate));
    ratio = std::max(ratio, 1.0f);

    int count = buffer.size();
    PackedVector2Array output;
    output.resize(count);

    const Vector2* in_ptr = buffer.ptr();
    Vector2* out_ptr = output.ptrw();

    for (int i = 0; i < count; i++) {
        float input_level = std::max(fabsf(in_ptr[i].x), fabsf(in_ptr[i].y));

        if (input_level > envelope)
            envelope = attack_coeff * envelope + (1.0f - attack_coeff) * input_level;
        else
            envelope = release_coeff * envelope + (1.0f - release_coeff) * input_level;

        float envelope_db = 20.0f * log10f(std::max(envelope, 1e-6f));
        float gain_reduction_db = 0.0f;
        if (envelope_db > threshold_db) {
            gain_reduction_db = (threshold_db - envelope_db) * (1.0f - 1.0f / ratio);
        }
        float gain_linear = powf(10.0f, gain_reduction_db / 20.0f);

        out_ptr[i] = in_ptr[i] * gain_linear;
    }

    Dictionary new_state;
    new_state["envelope"] = envelope;

    Dictionary result;
    result["output"] = output;
    result["state"] = new_state;
    return result;
}

}
