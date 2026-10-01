extends Node

@export var sound_file_path: String = "/home/el3tek/Projects/C-C++/hudmod-extension/demo/untitled.mp3"

var renderer: AudioRenderer

func _ready() -> void:
	renderer = AudioRenderer.new()
	renderer.start()
	
	play_demo()

func play_demo() -> void:
	if sound_file_path.is_empty():
		return
	
	var tracks: Array[PackedByteArray] = AudioDecoder.create_data_from_path(sound_file_path)
	if tracks.is_empty():
		push_error("فشل فك تشفير الملف الصوتي من المسار: %s" % sound_file_path)
		return
	
	# أخذ الصوت الخام المباشر بدون تعديل
	var raw_buffer: PackedByteArray = tracks[0]
	
	# إرسال البايتات الخام مباشرة للمؤثرات بقدر 0 (Volume 1.0, Pan 0.0 ومصفوفة مؤثرات فارغة)
	var empty_effects: Array[AudioEffectCustom] = []
	AudioMixer.packedbytearray_apply_effects(raw_buffer, empty_effects, 1.0, 0.0)
	
	# تشغيل الصوت في الـ Renderer
	renderer.push_samples(raw_buffer)
	print("تم إرسال عينات الصوت الخام بنجاح، الحجم: ", raw_buffer.size(), " bytes")
