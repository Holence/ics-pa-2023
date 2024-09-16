#include <proc.h>
#include <elf.h>
#include <fs.h>

// predefined compiler macro
// 可以打印看看 riscv64-linux-gnu-gcc -dM -E - < /dev/null | grep LP
#ifdef __LP64__
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Phdr Elf64_Phdr
#else
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Phdr Elf32_Phdr
#endif

#if defined(__ISA_AM_NATIVE__)
#define EXPECT_TYPE EM_X86_64
#elif defined(__riscv)
#define EXPECT_TYPE EM_RISCV
#else
#error Loader Unsupported ISA
#endif

int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);
size_t ramdisk_read(void *buf, size_t offset, size_t len);

// 将ramdisk中的elf文件装入内存，返回entry point address
uintptr_t loader(PCB *pcb, const char *filename) {
  int fd = fs_open(filename, 0, 0);

  if (fd == -1) {
    return (uintptr_t)NULL;
  }

  Elf_Ehdr ehdr;
  fs_lseek(fd, 0, SEEK_SET);
  fs_read(fd, &ehdr, sizeof(ehdr));

  // check magic number
  if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
    panic("nanos loader: navy-apps is not a elf format");
  }

  if (ehdr.e_machine != EXPECT_TYPE) {
    panic("nanos loader: unsupported isa %d, expected %d (look up id in elf.h)", ehdr.e_machine, EXPECT_TYPE);
  }

  Elf_Phdr phdr;
  for (int i = 0; i < ehdr.e_phnum; i++) {
    fs_lseek(fd, ehdr.e_phoff + i * ehdr.e_phentsize, SEEK_SET);
    fs_read(fd, &phdr, ehdr.e_phentsize);
    if (phdr.p_type == PT_LOAD) {
      fs_lseek(fd, phdr.p_offset, SEEK_SET);
      fs_read(fd, (void *)phdr.p_vaddr, phdr.p_memsz);
      memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0, phdr.p_memsz - phdr.p_filesz);
    }
  }

  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  Log("Start Loading %s", filename);
  uintptr_t entry = loader(pcb, filename);
  if (entry) {
    Log("Loading Finfished...");
    Log("Jump to entry = %p", entry);
    ((void (*)())entry)();
  } else {
    Warning("Loading Failed!!!");
  }
}
