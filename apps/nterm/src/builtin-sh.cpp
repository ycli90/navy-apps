#include <nterm.h>
#include <stdarg.h>
#include <unistd.h>
#include <SDL.h>
#include <vorbis.h>

#define MUSIC_PATH "/share/music/boot.ogg"
#define SAMPLES 4096

stb_vorbis *v = NULL;
stb_vorbis_info info = {};
void *audio_buf = NULL;
int16_t *stream_save = NULL;
int music_play_end = 0;
int audio_closed = 0;

void FillAudio(void *userdata, uint8_t *stream, int len) {
  int nbyte = 0;
  int samples_per_channel = stb_vorbis_get_samples_short_interleaved(v,
      info.channels, (int16_t *)stream, len / sizeof(int16_t));
  if (samples_per_channel != 0 || len < sizeof(int16_t)) {
    int samples = samples_per_channel * info.channels;
    nbyte = samples * sizeof(int16_t);
  }
  else {
    music_play_end = 1;
  }
  if (nbyte < len) memset(stream + nbyte, 0, len - nbyte);
  memcpy(stream_save, stream, len);
}

void play_music() {
  FILE *fp = fopen(MUSIC_PATH, "r");
  assert(fp);
  fseek(fp, 0, SEEK_END);
  size_t size = ftell(fp);
  audio_buf = malloc(size);
  assert(size);
  fseek(fp, 0, SEEK_SET);
  int ret = fread(audio_buf, size, 1, fp);
  assert(ret == 1);
  fclose(fp);

  int error;
  v = stb_vorbis_open_memory((const unsigned char *)audio_buf, size, &error, NULL);
  assert(v);
  info = stb_vorbis_get_info(v);

  SDL_AudioSpec spec;
  spec.freq = info.sample_rate;
  spec.channels = info.channels;
  spec.samples = SAMPLES;
  spec.format = AUDIO_S16SYS;
  spec.userdata = NULL;
  spec.callback = FillAudio;
  SDL_OpenAudio(&spec, NULL);

  stream_save = (int16_t*)malloc(SAMPLES * info.channels * sizeof(*stream_save));
  assert(stream_save);
  printf("Playing %s(freq = %d, channels = %d)...\n", MUSIC_PATH, info.sample_rate, info.channels);
  SDL_PauseAudio(0);
}

static void sh_printf(const char *format, ...);
char handle_key(SDL_Event *ev);

static int cmd_echo(char *args) {
  char *arg;
  bool first = true;
  while ((arg = strtok(NULL, " ")) != NULL) {
    if (first) first = false;
    else sh_printf(" ");
    sh_printf("%s", arg);
  }
  sh_printf("\n");
  return 0;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "echo", "display a line of text", cmd_echo },
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      sh_printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        sh_printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    sh_printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

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

static void sh_handle_cmd(const char *line) {
  if (line == nullptr) {
    printf("cmd is NULL\n");
    return;
  }
  char str[256];
  strncpy(str, line, 256);
  int len = strlen(str);
  if (str[len-1] == '\n') {
    str[len-1] = '\0';
    len--;
  }
  char *str_end = str + len;
  char *cmd = strtok(str, " ");
  if (cmd == NULL) return;
  char *args = cmd + strlen(cmd) + 1;
  if (args >= str_end) {
    args = NULL;
  }
  int i;
  for (i = 0; i < NR_CMD; i ++) {
    if (strcmp(cmd, cmd_table[i].name) == 0) {
      if (cmd_table[i].handler(args) < 0) { return; }
      break;
    }
  }
  if (i == NR_CMD) {
    const int max_argc = 8;
    char * argv [max_argc] = {};
    int argc = 0;
    char *token = cmd;
    while (token != NULL) {
      if (argc == max_argc - 1) {
        printf("too many args\n");
        break;
      }
      argv[argc++] = token;
      token = strtok(NULL, " ");
    }
    execvp(cmd, argv);
    sh_printf("Unknown command '%s'\n", cmd);
  }
}

void builtin_sh_run() {
  char *env_boot = getenv("boot");
  if (env_boot && strcmp(env_boot, "true") == 0) {
    unsetenv("boot");
    play_music();
  }
  setenv("PATH", "/bin:/usr/bin", 0);
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
    if (music_play_end && !audio_closed) {
      SDL_CloseAudio();
      stb_vorbis_close(v);
      free(stream_save);
      free(audio_buf);
      audio_closed = 1;
    }
  }
}
