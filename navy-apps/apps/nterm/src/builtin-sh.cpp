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

  char **argv = NULL; // Argument array, dynamic
  int argc = 0;       // Argument count

  // Get the arguments (remaining tokens of the string)
  char *arg = strtok(cmd_ptr, " ");
  while (arg != NULL) {
    // Reallocate memory to store the new argument
    argv = (char **)realloc(argv, sizeof(char *) * (argc + 1));
    if (argv == NULL) {
      fprintf(stderr, "nterm sh_handle_cmd: Memory allocation error\n");
    }

    // Store the argument
    argv[argc++] = arg;

    // Get the next token
    arg = strtok(NULL, " ");
  }

  // Terminate the argument array with NULL
  argv = (char **)realloc(argv, sizeof(char *) * (argc + 1));
  argv[argc] = NULL;

  printf("Arguments:\n");
  for (int i = 0; argv[i] != NULL; i++) {
    printf("  argv[%d] = %s\n", i, argv[i]);
  }

  if (argc > 0) {
    if (strcmp(argv[0], "echo") == 0) {
      for (int i = 1; i < argc; i++) {
        sh_printf("%s ", argv[i]);
      }
      sh_printf("\n");
    } else if (strcmp(argv[0], "quit") == 0) {
      exit(0);
    } else {
      int ret = execvp(argv[0], argv);
      if (ret == -1) {
        sh_printf("Cannot find excutable file: %s\n", argv[0]);
      }
    }
  }
  free(argv);
}

void builtin_sh_run() {
  sh_banner();
  sh_prompt();

  setenv("PATH", "/bin:/usr/bin", 0);
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
