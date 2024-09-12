#include <nterm.h>
#include <stdarg.h>
#include <unistd.h>
#include <SDL.h>

char handle_key(SDL_Event *ev);

static void sh_printf(const char *format, ...) {
  static char buf[256] = {};
  va_list ap;
  va_start(ap, format);
  int len = vsnprintf(buf, 256, format, ap);
  va_end(ap);
  term->write(buf, len);
}

static void sh_banner() {
  sh_printf("Built-in Shell in NTerm (NJU Terminal)\n\n");
}

static void sh_prompt() {
  sh_printf("sh> ");
}

#define isprint(c) ((unsigned)((c) - 0x20) <= (0x7e - 0x20))

char *ltrim(char *s) {
  while (!isprint(*s))
    s++;
  return s;
}

char *rtrim(char *s) {
  char *back = s + strlen(s);
  while (!isprint(*--back))
    ;
  *(back + 1) = '\0';
  return s;
}

char *trim(char *s) {
  return rtrim(ltrim(s));
}

static void sh_handle_cmd(const char *cmd) {
  char *cmd_ptr = const_cast<char *>(cmd);
  cmd_ptr = trim(cmd_ptr);
  char *op = strtok(cmd_ptr, " ");
  if (op == NULL) {
    return;
  } else {
    if (strcmp(op, "echo") == 0) {
      char *p = cmd_ptr + strlen(op) + 1;
      while (*p == ' ') {
        p++;
      }
      sh_printf("%s", p);
    } else if (strcmp(op, "quit") == 0) {
      exit(0);
    } else {
      int ret = execve(op, &op, NULL);
      if (ret == -1) {
        sh_printf("Cannot find excutable file: %s\n", op);
      }
    }
  }
}

void builtin_sh_run() {
  sh_banner();
  sh_prompt();

  while (1) {
    SDL_Event ev;
    if (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_KEYUP || ev.type == SDL_KEYDOWN) {
        const char *res = term->keypress(handle_key(&ev));
        if (res) {
          sh_handle_cmd(res);
          sh_prompt();
        }
      }
    }
    refresh_terminal();
  }
}
