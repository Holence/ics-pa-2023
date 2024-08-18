#include <common.h>
#include <elf.h>

#define read_str_from_STRTAB(STRTAB_offset, name) read_str_from_file(file, STRTAB_offset + name) // name is index

static void read_from_file(FILE *file, size_t offset, size_t size, void *dest) {
  fseek(file, offset, SEEK_SET);
  if (fread(dest, size, 1, file) != 1) {
    panic("elf read_from_file error!");
  }
}

char str_buf[64];
static void read_str_from_file(FILE *file, size_t offset) {
  fseek(file, offset, SEEK_SET);
  char c;
  char *buf_ptr = str_buf;
  while (1) {
    c = fgetc(file);
    *buf_ptr = c;
    buf_ptr++;
    if (c == '\0')
      break;
  }
}

static struct {
  char name[64];
  vaddr_t start;
  vaddr_t end;
} Functions[256];

static int functions_nums = 0;
void record_function(char *name, vaddr_t start, vaddr_t size) {
  if (functions_nums == 256) {
  }
  strcpy(Functions[functions_nums].name, name);
  Functions[functions_nums].start = start;
  Functions[functions_nums].end = start + size;
  functions_nums++;
}

static bool ftrace_enable = false;
void init_ftrace(char *filename) {
  if (filename == NULL) {
    return;
  }
  FILE *file = fopen(filename, "rb");
  if (!file) {
    panic("cannot find elf file: %s", filename);
  }

  Elf32_Ehdr ehdr;
  read_from_file(file, 0, sizeof(ehdr), &ehdr);

  if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
    panic("not a elf format: %s", filename);
  }

#define read_shdr(index) read_from_file(file, ehdr.e_shoff + ehdr.e_shentsize * index, ehdr.e_shentsize, &temp_section_header);
  Elf32_Shdr temp_section_header;

  Elf32_Off symtab_offset = 0;
  Elf32_Shdr symtab_header;
  int symtab_len = 0;
  Elf32_Off strtab_offset = 0;

  // find symtab's and strtab's offset
  read_shdr(ehdr.e_shstrndx);
  Elf32_Off section_name_str_table = temp_section_header.sh_offset;
  for (int i = 0; i < ehdr.e_shnum; i++) {
    read_shdr(i);
    if (temp_section_header.sh_type == SHT_SYMTAB) {
      symtab_offset = temp_section_header.sh_offset;
      symtab_header = temp_section_header;
      symtab_len = symtab_header.sh_size / symtab_header.sh_entsize;
    } else if (temp_section_header.sh_type == SHT_STRTAB) {
      // x86上TYPE为STRTAB的有三个，.dynstr .strtab .shstrtab，那只能用名字来选出.strtab了
      read_str_from_STRTAB(section_name_str_table, temp_section_header.sh_name);
      if (strcmp(".strtab", str_buf) == 0) {
        strtab_offset = temp_section_header.sh_offset;
      }
    }
  }

  // read all symtab's entry and find FUNC and store them in Functions
  Elf32_Sym symtab_entry;
  for (int i = 0; i < symtab_len; i++) {
    read_from_file(file, symtab_offset + i * sizeof(symtab_entry), sizeof(symtab_entry), &symtab_entry);
    if (ELF32_ST_TYPE(symtab_entry.st_info) == STT_FUNC) {
      read_str_from_STRTAB(strtab_offset, symtab_entry.st_name);
      record_function(str_buf, symtab_entry.st_value, symtab_entry.st_size);
    }
  }

  for (int i = 0; i < functions_nums; i++) {
    printf("%s: %x %x\n", Functions[i].name, Functions[i].start, Functions[i].end);
  }

  fclose(file);
  ftrace_enable = true;
}

static int depth = 0;
void ftrace_log(vaddr_t address, bool jump_in) {
  if (ftrace_enable) {
    if (jump_in) {
      depth++;
    }
    printf(FMT_WORD ":", address);
    for (int i = 0; i < functions_nums; i++) {
      // 函数真实范围，包含start，不包含end
      if (address >= Functions[i].start && address < Functions[i].end) {
        printf("%*c", depth * 2, ' ');
        if (jump_in) {
          printf("Call ");
        } else {
          printf("Ret  ");
        }
        printf("%s\n", Functions[i].name);
        break;
      }
    }
    if (!jump_in) {
      depth--;
    }
  }
}
