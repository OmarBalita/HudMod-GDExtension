#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

namespace godot {

class AudioMixer : public Resource {
    GDCLASS(AudioMixer, Resource);

protected:
    static void _bind_methods();

public:
    AudioMixer();
    ~AudioMixer();

    static PackedByteArray mix_buffers(Array buffers, float master_gain = 1.0f);

    static PackedVector2Array bytes_to_vector2(PackedByteArray data);
    static PackedByteArray vector2_to_bytes(PackedVector2Array data);

    static PackedVector2Array run_effect_chain(PackedVector2Array buffer, Array components, int sample_rate);

    static void reset_effect_chain(Array components);
    
    static PackedVector2Array apply_gain(PackedVector2Array buffer, float gain_linear);
    static PackedVector2Array apply_pan(PackedVector2Array buffer, float pan);
    static PackedVector2Array soft_clip_buffer(PackedVector2Array buffer);
    static inline float sample_soft_clip(float sample);
    
    static PackedVector2Array mix_two(PackedVector2Array a, PackedVector2Array b);
    static PackedVector2Array mix_dry_wet(PackedVector2Array dry, PackedVector2Array wet, float wet_amount);
    
    static Dictionary process_delay_line(PackedVector2Array buffer, Dictionary state, int delay_samples, float feedback, float mix);
    
    static PackedFloat32Array compute_biquad_lowpass(float cutoff_hz, float q, int sample_rate);
    static PackedFloat32Array compute_biquad_highpass(float cutoff_hz, float q, int sample_rate);
    static PackedFloat32Array compute_biquad_bandpass(float center_hz, float q, int sample_rate);
    static PackedFloat32Array compute_biquad_peaking_eq(float center_hz, float q, float gain_db, int sample_rate);
    
    static Dictionary process_biquad(PackedVector2Array buffer, PackedFloat32Array coeffs, PackedFloat32Array state);
    
    static Dictionary process_comb_filter(PackedVector2Array buffer, Dictionary state, int delay_samples, float feedback, float damping);
    static Dictionary process_allpass_filter(PackedVector2Array buffer, Dictionary state, int delay_samples, float gain);
    
    static PackedVector2Array process_saturation(PackedVector2Array buffer, float drive, float mix);
    
    static Dictionary process_compressor(PackedVector2Array buffer, Dictionary state, float threshold_db, float ratio, float attack_ms, float release_ms, int sample_rate);
};

}
