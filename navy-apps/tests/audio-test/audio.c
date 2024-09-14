#include <stdio.h>
#include <NDL.h>

int main() {
  int sbuf_size = NDL_QueryAudio();

  NDL_OpenAudio(8000, 1, 1024);
  extern uint8_t audio_payload, audio_payload_end;
  uint32_t audio_len = &audio_payload_end - &audio_payload;
  int nplay = 0;
  void *start = &audio_payload;
  while (nplay < audio_len) {
    int len = (audio_len - nplay > 1024 ? 1024 : audio_len - nplay);
    while (NDL_QueryAudio() < len) {
    }
    NDL_PlayAudio(start, len);
    start += len;
    nplay += len;
    printf("Already play %d/%d bytes of data\n", nplay, audio_len);
  }

  // wait until the audio finishes
  int free_space;
  do {
    free_space = NDL_QueryAudio();
  } while (free_space != sbuf_size);

  return 0;
}
