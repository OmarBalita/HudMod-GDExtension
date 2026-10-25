#include "audio/CustomAudioStreamPlayer.h"
#include "audio/AudioMixer.h"

#include <godot_cpp/classes/object.hpp>

namespace godot {

    void CustomAudioStreamPlayer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("get_data"), &CustomAudioStreamPlayer::get_data);
        ClassDB::bind_method(D_METHOD("set_data", "new_data"), &CustomAudioStreamPlayer::set_data);

        ClassDB::bind_method(D_METHOD("play", "from_position"), &CustomAudioStreamPlayer::play, DEFVAL(0.0f), DEFVAL(Array()));
        ClassDB::bind_method(D_METHOD("seek", "to_position"), &CustomAudioStreamPlayer::seek);
        ClassDB::bind_method(D_METHOD("stop"), &CustomAudioStreamPlayer::stop);
    }

    CustomAudioStreamPlayer::CustomAudioStreamPlayer() {
        Ref<AudioStreamGenerator> gen_stream;
        gen_stream.instantiate();
        gen_stream->set_mix_rate(SAMPLE_RATE);
        gen_stream->set_buffer_length(0.1f);

        set_stream(gen_stream);
        set_process(false);
    }

    CustomAudioStreamPlayer::~CustomAudioStreamPlayer() {}

    void CustomAudioStreamPlayer::play(float at, Array effects) {
        _curr_frame_idx = (int)(at * SAMPLE_RATE);
        AudioStreamPlayer::play(at);
        
        active_effects = effects;
        AudioMixer::reset_effect_chain(active_effects);
        
        generator_playback = get_stream_playback();
        set_process(true);
    }

    void CustomAudioStreamPlayer::seek(float position) {
        _curr_frame_idx = (int)(position * SAMPLE_RATE);
        AudioStreamPlayer::seek(position);

        AudioMixer::reset_effect_chain(active_effects);
        
        generator_playback = get_stream_playback();
    }

    void CustomAudioStreamPlayer::stop() {
        AudioStreamPlayer::stop();
        set_process(false);
        generator_playback.unref();
    }

    void CustomAudioStreamPlayer::_process(double delta) {
        if (!is_playing()) {
            set_process(false);
            return;
        }

        if (generator_playback.is_null()) {
            generator_playback = get_stream_playback();
        }

        _fill_buffer();
    }

    void CustomAudioStreamPlayer::_fill_buffer() {
        if (generator_playback.is_null() || _data.is_empty()) return;

        int frames_available = generator_playback->get_frames_available();
        if (frames_available <= 0) return;

        const float* raw_ptr = reinterpret_cast<const float*>(_data.ptr());
        int total_frames = _data.size() / BYTES_PER_FRAME;

        int frames_to_push = std::min(frames_available, total_frames - _curr_frame_idx);
        if (frames_to_push <= 0) {
            set_process(false);
            return;
        }

        PackedVector2Array buffer;
        buffer.resize(frames_to_push);
        Vector2* write_ptr = buffer.ptrw();

        for (int i = 0; i < frames_to_push; i++) {
            float l = raw_ptr[_curr_frame_idx * 2];
            float r = raw_ptr[_curr_frame_idx * 2 + 1];

            write_ptr[i] = Vector2(l, r);
            _curr_frame_idx++;
        }

        if (!active_effects.is_empty()) {
            buffer = AudioMixer::run_effect_chain(buffer, active_effects, SAMPLE_RATE);
        }

        generator_playback->push_buffer(buffer);
    }

}
