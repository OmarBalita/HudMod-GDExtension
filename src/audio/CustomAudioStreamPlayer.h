#pragma once
#include "godot_cpp/variant/array.hpp"
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/audio_stream_generator.hpp>
#include <godot_cpp/classes/audio_stream_generator_playback.hpp>

namespace godot {

class CustomAudioStreamPlayer: public AudioStreamPlayer {
    GDCLASS(CustomAudioStreamPlayer, AudioStreamPlayer);

private:
    Ref<AudioStreamGeneratorPlayback> generator_playback;
    
    Array active_effects;
 
    void _fill_buffer();

protected:
    static void _bind_methods();

public:
    
    static const int CHANNELS = 2;
    static const int SAMPLE_RATE = 48000;
    static const int BYTES_PER_SAMPLE = 4;
    static const int BYTES_PER_FRAME = 4 * 2;

    PackedByteArray _data;
    int _curr_frame_idx = 0;

    PackedByteArray get_data() const {return _data;}
    void set_data(const PackedByteArray &new_data) {_data = new_data;}
    
    CustomAudioStreamPlayer();
    ~CustomAudioStreamPlayer();

    void play(float at, Array effects = Array());
    void seek(float position);
    void stop();

    void _process(double delta) override;
};
}
