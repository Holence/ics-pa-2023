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
      // PA3
      // fs_read(fd, (void *)phdr.p_vaddr, phdr.p_memsz);
      // memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0, phdr.p_memsz - phdr.p_filesz);

      // PA4.3后 用户进程也支持分页
      // 这里要在堆中动态申请page，并用map写好虚拟->物理的页表项
      // 这里不能用 fs_read(fd, (void *)phdr.p_vaddr, phdr.p_memsz)，因为现在还在内核中，satp并没有切换到用户进程的页表

      int written = 0;
      int filesz = phdr.p_filesz;
      int memsz = phdr.p_memsz;
      int offset = phdr.p_vaddr & 0xfff; // Segment起始的虚拟地址不一定是对齐的……
      if (offset != 0) {
        int len = (filesz < PGSIZE - offset ? filesz : PGSIZE - offset);
        void *page = new_page(1);
        map(&(pcb->as), (void *)phdr.p_vaddr, page, MMAP_READ | MMAP_WRITE);
        fs_read(fd, page + offset, len);

        int left_part = offset + len;
        if (left_part < PGSIZE) {
          memset(page + left_part, 0, PGSIZE - left_part);
        }
        written += PGSIZE - offset;
      }

      while (written < filesz) {
        int len = (filesz - written < PGSIZE ? filesz - written : PGSIZE);
        void *page = new_page(1);
        map(&(pcb->as), (void *)phdr.p_vaddr + written, page, MMAP_READ | MMAP_WRITE);
        fs_read(fd, page, len);
        if (len < PGSIZE) {
          memset(page + len, 0, PGSIZE - len);
        }
        written += PGSIZE;
      }

      while (written < memsz) {
        void *page = new_page(1);
        memset(page, 0, PGSIZE);
        map(&(pcb->as), (void *)phdr.p_vaddr + written, page, MMAP_READ | MMAP_WRITE);
        written += PGSIZE;
      }

      // 用户进程堆区的起始虚拟地址在bss结尾处，也就是这里装入memsz后的地址
      // 如果装入memsz后地址可以对齐，则max_brk就在此处，之后第一次mm_brk会从max_brk开始申请新页
      // 如果装入memsz后地址不能对齐，则max_brk可以等在下一个页处（因为前面已经清零了）
      pcb->max_brk = ROUNDUP(phdr.p_vaddr + phdr.p_memsz, PGSIZE);
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
