#include <proc.h>
#include <elf.h>

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

size_t ramdisk_read(void *buf, size_t offset, size_t len);

// 将ramdisk中的elf文件装入内存，返回entry point address
static uintptr_t loader(PCB *pcb, const char *filename) {
  Elf_Ehdr ehdr;
  ramdisk_read(&ehdr, 0, sizeof(ehdr));

  // check magic number
  if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
    panic("nanos loader: navy-apps is not a elf format");
  }

  if (ehdr.e_machine != EXPECT_TYPE) {
    panic("nanos loader: unsupported isa %d, expected %d (look up id in elf.h)", ehdr.e_machine, EXPECT_TYPE);
  }

  Elf_Phdr phdr;
  for (int i = 0; i < ehdr.e_phnum; i++) {
    ramdisk_read(&phdr, ehdr.e_phoff + i * ehdr.e_phentsize, ehdr.e_phentsize);
    if (phdr.p_type == PT_LOAD) {
      ramdisk_read((void *)phdr.p_vaddr, phdr.p_offset, phdr.p_memsz);
      memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0, phdr.p_memsz - phdr.p_filesz);
    }
  }

  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void (*)())entry)();
}
