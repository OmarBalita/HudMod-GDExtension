#include "core/functionality/MediaHelper.h"
#include <godot_cpp/core/class_db.hpp>
#include <algorithm>
#include <cmath>

using namespace godot;

Ref<Gradient> MediaHelper::waveform_gradient;

void MediaHelper::_bind_methods() {
    ClassDB::bind_static_method("MediaHelper", D_METHOD("get_waveform_gradient"), &MediaHelper::get_waveform_gradient);
    ClassDB::bind_static_method("MediaHelper", D_METHOD("set_waveform_gradient", "new_val"), &MediaHelper::set_waveform_gradient);

    ClassDB::bind_static_method("MediaHelper", D_METHOD("build_peak_map", "data", "peaks_per_second", "max_frames_per_peak"), &MediaHelper::build_peak_map, DEFVAL(0));
    ClassDB::bind_static_method("MediaHelper", D_METHOD("render_waveform", "peaks", "pixel_from", "peaks_span", "pixels_span", "image_format", "width", "height", "space_width", "line_width", "global_peak", "style", "bg_color"), &MediaHelper::render_waveform);

    BIND_ENUM_CONSTANT(STYLE_THUMBNAIL);
    BIND_ENUM_CONSTANT(STYLE_TIMELINE);
}

Ref<Gradient> MediaHelper::get_waveform_gradient() {
    return waveform_gradient;
}

void MediaHelper::set_waveform_gradient(const Ref<Gradient> &new_val) {
    waveform_gradient = new_val;
}

Dictionary MediaHelper::build_peak_map(const PackedByteArray &data, int peaks_per_second, int max_frames_per_peak) {
    peaks_per_second = std::max(1, peaks_per_second);
    
    PackedFloat32Array peaks;
    float global_peak = 0.0f;
    
    const int64_t frame_count = data.size() / (int64_t)(sizeof(float) * AUDIO_CHANNELS);
    
    if (frame_count > 0) {
        const float *samples = reinterpret_cast<const float *>(data.ptr());

        // ceil(frame_count * peaks_per_second / sample_rate)
        const int64_t peak_count = (frame_count * peaks_per_second + AUDIO_SAMPLE_RATE - 1) / AUDIO_SAMPLE_RATE;
        peaks.resize(peak_count);
        float *out = peaks.ptrw();

        for (int64_t p = 0; p < peak_count; p++) {
            // Integer slice edges: neighbouring slices share a boundary, so nothing drifts on long files.
            const int64_t f0 = p * AUDIO_SAMPLE_RATE / peaks_per_second;
            const int64_t f1 = std::min<int64_t>((p + 1) * AUDIO_SAMPLE_RATE / peaks_per_second, frame_count);

            int64_t stride = 1;
            if (max_frames_per_peak > 0 && f1 - f0 > max_frames_per_peak) {
                stride = (f1 - f0 + max_frames_per_peak - 1) / max_frames_per_peak;
            }

            float peak = 0.0f;
            for (int64_t f = f0; f < f1; f += stride) {
                const float value = (std::fabs(samples[f * AUDIO_CHANNELS]) + std::fabs(samples[f * AUDIO_CHANNELS + 1])) * 0.5f;
                // isfinite: one corrupt sample (inf) must not flatten the whole file after normalising.
                if (value > peak && std::isfinite(value)) {
                    peak = value;
                }
            }
            
            out[p] = peak;
            global_peak = std::max(global_peak, peak);
        }
    }
    
    Dictionary result;
    result["peaks"] = peaks;
    result["peak"] = global_peak;
    result["peaks_per_second"] = peaks_per_second;
    return result;
}

Ref<Image> MediaHelper::render_waveform(const PackedFloat32Array &peaks, int64_t pixel_from, int64_t peaks_span, int64_t pixels_span, Image::Format image_format, int width, int height, int space_width, int line_width, float global_peak, int style, Color bg_color) {
    if (width < 1 || height < 1 || pixels_span < 1 || peaks_span < 0 || line_width < 1 || space_width < 0) {
        return nullptr;
    }

    Ref<Image> image = Image::create_empty(width, height, false, image_format);
    if (image.is_null()) {
        return nullptr;
    }
    image->fill(bg_color);

    const int step_width = space_width + line_width;
    const int lines_count = (width + step_width - 1) / step_width;

    const float *src = peaks.ptr();
    const int64_t peak_count = peaks.size();
    const float norm = global_peak > 0.0f ? 1.0f / global_peak : 1.0f;

    for (int line = 0; line < lines_count; line++) {
        const int x = line * step_width;
        const int64_t abs_pixel = pixel_from + x;
        
        // Peaks covered by this line: [a, b). Exact integer math: no gaps or overlaps between lines or chunks.
        const int64_t a = abs_pixel * peaks_span / pixels_span;
        int64_t b = (abs_pixel + step_width) * peaks_span / pixels_span;
        if (b <= a) {
            b = a + 1;
        }
        
        float amplitude = 0.0f;
        for (int64_t i = std::max<int64_t>(a, 0); i < b && i < peak_count; i++) {
            amplitude = std::max(amplitude, src[i]);
        }

        draw_line(image, style, height, line_width, x, std::min(amplitude * norm, 1.0f));
    }

    image->generate_mipmaps();
    return image;
}

void MediaHelper::draw_line(const Ref<Image> &image, int style, int height, int line_width, int x, float amplitude) {
    const int line_height = std::max(1, (int)(amplitude * height));
    const Vector2i pos(x, (int)(height / 2.0f - line_height / 2.0f));
    const Vector2i size(line_width, line_height);

    if (style == STYLE_THUMBNAIL) {
        // Colour follows the position along the image (left to right).
        Color color(1, 1, 1, 1);
        if (waveform_gradient.is_valid()) {
            color = waveform_gradient->sample(x / (float)image->get_width());
        }
        image->fill_rect(Rect2i(pos, size), color);
    } else {
        image->fill_rect(Rect2i(pos, size), Color(0, 0, 0, 0.6f));
    }
}
