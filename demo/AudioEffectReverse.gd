class_name AudioEffectReverse
extends AudioEffectCustom

func _process_stream(data: PackedByteArray) -> PackedByteArray:
	if data.is_empty():
		return data
	
	const CHANNELS: int = 2
	
	var samples: PackedFloat32Array = data.to_float32_array()
	var total_samples: int = samples.size()
	var total_frames: int = total_samples / CHANNELS
	
	if total_frames <= 1:
		return data
	
	var reversed_samples := PackedFloat32Array()
	reversed_samples.resize(total_samples)
	
	for i in range(total_frames):
		var src_frame_idx: int = (total_frames - 1 - i) * CHANNELS
		var dst_frame_idx: int = i * CHANNELS
		
		reversed_samples[dst_frame_idx]		= samples[src_frame_idx]	 # Left
		reversed_samples[dst_frame_idx + 1] = samples[src_frame_idx + 1] # Right
	
	return reversed_samples.to_byte_array()
