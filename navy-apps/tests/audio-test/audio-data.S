.section .data
.global audio_payload, audio_payload_end
.p2align 3
audio_payload:
// copy little-star.pcm from /am-kernels/tests/am-tests/src/tests/audio/little-star.pcm
.incbin "little-star.pcm"
audio_payload_end:
