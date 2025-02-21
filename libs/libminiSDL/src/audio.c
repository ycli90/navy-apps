#include <NDL.h>
#include <SDL.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int pause;
static SDL_AudioSpec audio_spec;
static uint8_t *audio_buf;
static SDL_TimerID timer_id = NULL;

uint32_t audio_timer_callback(uint32_t interval, void *param) {
  if (!pause) {
    int free_size = NDL_QueryAudio(NULL);
    int len = free_size < audio_spec.size ? free_size : audio_spec.size;
    audio_spec.callback(NULL, audio_buf, len);
    NDL_PlayAudio(audio_buf, len);
  }
  return interval;
}

int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained) {
  if (desired->format != AUDIO_S16SYS) {
    printf("Audio format not supported\n");
    return -1;
  }
  audio_spec.freq = desired->freq;
  audio_spec.channels = desired->channels;
  audio_spec.samples = desired->samples;
  audio_spec.size = audio_spec.samples * sizeof(int16_t) * audio_spec.channels;
  audio_spec.callback = desired->callback;
  audio_buf = malloc(audio_spec.size);
  if (audio_buf == NULL) return -1;
  pause = 1;
  uint32_t interval = 1000 * desired->samples / desired->freq;
  timer_id = SDL_AddTimer(interval, audio_timer_callback, NULL);
  if (timer_id == NULL) return -1;
  NDL_OpenAudio(desired->freq, desired->channels, desired->samples);
  if (obtained != NULL) {
    memcpy(obtained, &audio_spec, sizeof(SDL_AudioSpec));
  } else {
    memcpy(desired, &audio_spec, sizeof(SDL_AudioSpec));
  }
  return 0;
}

void SDL_CloseAudio() {
  SDL_RemoveTimer(timer_id);
  free(audio_buf);
  NDL_CloseAudio();
}

void SDL_PauseAudio(int pause_on) {
  pause = pause_on;
}

void SDL_MixAudio(uint8_t *dst, uint8_t *src, uint32_t len, int volume) {
  assert(volume >= 0);
  if (volume > SDL_MIX_MAXVOLUME) volume = SDL_MIX_MAXVOLUME;
  int16_t *dst16 = (int16_t*)dst;
  int16_t *src16 = (int16_t*)src;
  for (int i = 0; i < len / 2; i++) {
    int value = dst16[i] + src16[i] * volume / SDL_MIX_MAXVOLUME;
    if (value > INT16_MAX) value = INT16_MAX;
    if (value < INT16_MIN) value = INT16_MIN;
    dst16[i] = value;
  }
}

typedef struct {
  uint32_t Subchunk1ID;    // Contains the letters "fmt " (0x666d7420 big-endian form).
  uint32_t Subchunk1Size;  // 16 for PCM.  This is the size of the rest of the Subchunk which follows this number.
  uint16_t AudioFormat;    // PCM = 1 (i.e. Linear quantization) Values other than 1 indicate some form of compression.
  uint16_t NumChannels;    // Mono = 1, Stereo = 2, etc.
  uint32_t SampleRate;     // 8000, 44100, etc.
  uint32_t ByteRate;       // == SampleRate * NumChannels * BitsPerSample/8
  uint16_t BlockAlign;     // == NumChannels * BitsPerSample/8 The number of bytes for one sample including all channels.
  uint16_t BitsPerSample;  // 8 bits = 8, 16 bits = 16, etc.
} WAV_Format_desc;

SDL_AudioSpec *SDL_LoadWAV(const char *file, SDL_AudioSpec *spec, uint8_t **audio_buf, uint32_t *audio_len) {
  FILE *fp = fopen(file, "rb");
  uint32_t chunk_desc[3];
  fread(chunk_desc, 4, 3, fp);
  if (chunk_desc[0] != 0x46464952 || chunk_desc[2] != 0x45564157) {
    printf("Invalid WAV file %s\n", file);
    return NULL;
  }
  WAV_Format_desc fmt_desc;
  fread(&fmt_desc, sizeof(WAV_Format_desc), 1, fp);
  if (fmt_desc.Subchunk1ID != 0x20746d66 || fmt_desc.Subchunk1Size != 16
    || fmt_desc.AudioFormat != 1 || fmt_desc.BitsPerSample != 16) {
    printf("Invalid WAV file %s\n", file);
    return NULL;
  }
  if (fmt_desc.ByteRate != fmt_desc.SampleRate * fmt_desc.NumChannels * fmt_desc.BitsPerSample / 8
    || fmt_desc.BlockAlign != fmt_desc.NumChannels * fmt_desc.BitsPerSample / 8) {
    printf("Invalid WAV file %s\n", file);
    return NULL;
  }
  uint32_t data_desc[2];
  fread(data_desc, 4, 2, fp);
  if (data_desc[0] != 0x61746164) {
    printf("Invalid WAV file %s\n", file);
    return NULL;
  }
  uint32_t data_len = data_desc[1];
  uint8_t *data = malloc(data_len);
  assert(data != NULL);
  fread(data, data_len, 1, fp);
  spec->freq = fmt_desc.SampleRate;
  spec->channels = fmt_desc.NumChannels;
  spec->samples = 4096;
  *audio_buf = data;
  *audio_len = data_len;
  return spec;
}

void SDL_FreeWAV(uint8_t *audio_buf) {
  free(audio_buf);
}

void SDL_LockAudio() {
}

void SDL_UnlockAudio() {
}
