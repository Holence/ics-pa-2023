#include <fs.h>

extern uint8_t ramdisk_start;
void play_boot_music() {
  int fd = fs_open("/share/music/xp.pcm", 0, 0);
  size_t audio_len = fs_lseek(fd, 0, SEEK_END);
  io_write(AM_AUDIO_CTRL, 22050 * 2, 2, 4096);
  Area sbuf;
  sbuf.start = &ramdisk_start + get_disk_offset(fd);
  size_t nplay = 0;
  while (nplay < audio_len) {
    int len = (audio_len - nplay > 4096 ? 4096 : audio_len - nplay);
    sbuf.end = sbuf.start + len;
    io_write(AM_AUDIO_PLAY, sbuf);
    sbuf.start += len;
    nplay += len;
  }
  io_write(AM_AUDIO_CTRL, 0, 0, 0);
}