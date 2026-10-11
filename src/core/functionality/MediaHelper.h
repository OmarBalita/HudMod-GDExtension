#pragma once
#include "godot_cpp/classes/resource.hpp"
#include <godot_cpp/classes/gradient.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>

namespace godot {

class MediaHelper : public Resource {
    GDCLASS(MediaHelper, Resource)

public:
    enum WaveformStyle {
        STYLE_THUMBNAIL = 0,
        STYLE_TIMELINE = 1,
    };
    
    static constexpr int AUDIO_CHANNELS = 2;
    static constexpr int AUDIO_SAMPLE_RATE = 48000;

private:
    static Ref<Gradient> waveform_gradient;
    static void draw_line(const Ref<Image> &image, int style, int height, int line_width, int x, float amplitude);

protected:
    static void _bind_methods();

public:
    static Ref<Gradient> get_waveform_gradient();
    static void set_waveform_gradient(const Ref<Gradient> &new_val);
    
    static Dictionary build_peak_map(const PackedByteArray &data, int peaks_per_second, int max_frames_per_peak);

    static Ref<Image> render_waveform(
        const PackedFloat32Array &peaks,
        int64_t pixel_from, int64_t peaks_span, int64_t pixels_span,
        Image::Format image_format, int width, int height,
        int space_width, int line_width, float global_peak,
        int style, Color bg_color
    );
};

} // namespace godot

VARIANT_ENUM_CAST(godot::MediaHelper::WaveformStyle);
