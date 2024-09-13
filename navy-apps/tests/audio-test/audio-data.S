.section .data
.global audio_payload, audio_payload_end
.p2align 3
audio_payload:
.incbin "little-star.pcm"
audio_payload_end:
