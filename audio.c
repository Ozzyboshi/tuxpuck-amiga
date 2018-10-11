/* audio.c - Copyright (C) 2001-2002 Jacob Kroon, see COPYING for details */

#include <stdlib.h>
#include <string.h>
#include <SDL_audio.h>
#include <SDL_endian.h>
#include <vorbis/vorbisfile.h>
#include "audio.h"
#include <SDL.h>
#include <SDL_mixer.h>

/* defines */
#define BUFFER_SIZE	((Uint32)128)
#define BUFFER_LIMIT	((Uint32)1000000000)

/* structs */
struct _Sound {
  Uint32 length;
  Uint8 *buffer;
  Uint8 single;
};

typedef struct _SoundNode {
  Uint32 position;
  Sound *sound;
  struct _SoundNode *next;
} SoundNode;

/* statics */
static Uint8 _mute = 0, _initiated = 0;
static SDL_AudioSpec _spec;
static SoundNode *_sound_node = NULL;

/* functions */
static size_t _RWops_ogg_read(void *ptr, size_t size, size_t nmemb,
			      void *source)
{
  return ((size_t) SDL_RWread((SDL_RWops *) source, ptr, size, nmemb));
}

static int _RWops_ogg_seek(void *source, ogg_int64_t offset, int whence)
{
  return (SDL_RWseek((SDL_RWops *) source, offset, whence));
}

static int _RWops_ogg_close(void *source)
{
  return 0;
}

static long _RWops_ogg_tell(void *source)
{
  return ((long) SDL_RWtell((SDL_RWops *) source));
}

static const ov_callbacks _ogg_callbacks = {
  _RWops_ogg_read,
  _RWops_ogg_seek,
  _RWops_ogg_close,
  _RWops_ogg_tell
};

static void _audio_mix_audio(void *unused, Uint8 * stream, int length)
{
  SoundNode *node = NULL, *prev = NULL, *temp = NULL;
  int toWrite = 0;
  
  // test
  //return;

/*#ifdef _DEBUG
	printf("Playing audio - mute: %d\n",_mute);
	exit(1);
#endif*/

  if (_mute)
    return;
  prev = NULL;
  node = _sound_node;
  while (node != NULL) {
    if (node->position + length > node->sound->length)
      toWrite = node->sound->length - node->position;
    else
      toWrite = length;
    SDL_MixAudio(stream, &node->sound->buffer[node->position], toWrite,
		 SDL_MIX_MAXVOLUME);
    node->position += toWrite;
    if (toWrite < length) {
      if (prev)
	prev->next = node->next;
      else
	_sound_node = node->next;
      temp = node->next;
      free(node);
      node = temp;
    } else {
      prev = node;
      node = node->next;
    }
  }
}

void audio_init(void)
{

/*  int audio_rate = MIX_DEFAULT_FREQUENCY;
  Uint16 audio_format = MIX_DEFAULT_FORMAT;
  int audio_channels = 1;
  if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, 4096) < 0) 
    {
      fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
      return -1;
    } 
    return ;*/

  SDL_AudioSpec fmt;
  SDL_AudioSpec wanted;
  
  /* Set the audio format */
    wanted.freq = 11025;
    wanted.format = AUDIO_S16SYS;
    wanted.channels = 1;    /* 1 = mono, 2 = stereo */
    wanted.samples = 1024;  /* Good low-latency value for callback */
    wanted.callback = _audio_mix_audio;
    wanted.userdata = NULL;

  fmt.freq = 11025;
  //fmt.freq = MIX_DEFAULT_FREQUENCY;
  fmt.format = AUDIO_S16SYS;
  // fmt.format = MIX_DEFAULT_FORMAT;
  fmt.channels = 1;
  fmt.samples = BUFFER_SIZE;
//  test fmt.callback = _audio_mix_audio;
  fmt.callback= NULL;
  fmt.userdata = NULL;
  if (SDL_OpenAudio(&wanted, &_spec) < 0)
  {
#ifdef _DEBUG
	fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
	exit(1);
#endif
    _initiated = 0;
  }
  else {
    SDL_PauseAudio(0);
#ifdef _DEBUG
    printf("SDL Openaudio succeded\n");
#endif
    _initiated = 1;
  }
  return;
}

void audio_deinit(void)
{
  SoundNode *node = _sound_node, *next = NULL;

  while (node) {
    next = node->next;
    free(node);
    node = next;
  }
  if (_initiated)
    SDL_CloseAudio();
}

void audio_set_mute(Uint8 mute)
{
  _mute = mute;
}

void audio_play_sound(Sound * sound)
{
  SoundNode *node = NULL;

  if (!sound || !_initiated || _mute)
    return;
  if (sound->single) {
    node = _sound_node;
    while (node != NULL) {
      if (node->sound == sound)
	return;
      node = node->next;
    }
  }
  node = (SoundNode *) malloc(sizeof(SoundNode));
  node->sound = sound;
  node->position = 0;
  node->next = NULL;
  if (_sound_node != NULL)
    node->next = _sound_node;
  _sound_node = node;
}

Sound *audio_create_sound(Uint8 * data, Uint32 * memcounter)
{
  OggVorbis_File *vf = NULL;
  vorbis_info *vi = NULL;
  SDL_RWops *src = NULL;
  Sound *sound = NULL;
  int dummy = 0;
  Uint8 ov_open_success = 0;
  Uint32 size = 0, buffer_size = 0, bytes_read = 0, counter = 0;

  revmemcpy_snd(&size, data, sizeof(Uint32));
#ifdef _DEBUG
	printf("Size : %d %d %d %d %d\n",size,data[0],data[1],data[2],data[3]);
#endif
  //memcpy(&size, data, sizeof(Uint32));
#ifdef _DEBUG
	printf("Memcounter: %d\n",memcounter);
#endif
if (memcounter)
    *memcounter += size + sizeof(Uint32);
  if (!_initiated)
    goto done;
#ifdef _DEBUG
	printf("Data : %d\n",data[0],data[1]);
#endif
  data += sizeof(Uint32);
  src = SDL_RWFromMem(data, size);
  vf = (OggVorbis_File *) malloc(sizeof(OggVorbis_File));
  if (ov_open_callbacks(src, vf, NULL, 0, _ogg_callbacks) != 0)
    goto done;
#ifdef _DEBUG
	printf("Ov open callbacks done\n");
#endif
  ov_open_success = 1;
  vi = ov_info(vf, -1);
#ifdef _DEBUG
	printf("Vi channels: %d %d %d %d\n",vi->channels,vi->rate,_spec.channels,_spec.freq);
#endif
  if (vi->channels != _spec.channels || vi->rate != _spec.freq)
    goto done;
#ifdef _DEBUG
	printf("Vi channels done\n");
#endif
  buffer_size = ov_pcm_total(vf, -1) * _spec.channels * 2;
  if (buffer_size > BUFFER_LIMIT)
    goto done;
#ifdef _DEBUG
	printf("Buffer size allocated %d\n",buffer_size);
#endif
  sound = (Sound *) malloc(sizeof(Sound));
  sound->length = buffer_size;
  sound->single = 1;
  /* ATTENTION!!!!! UGLY HACK
   * I've added +4096 bytes to be mallocated, but I have no
   * idea _why_ this prevents TuxPuck from crashing :)
   */
  sound->buffer = (Uint8 *) malloc(buffer_size + 4096);
  while ((bytes_read =
	  ov_read(vf, (char *) &sound->buffer[counter], 4096, 1, 2, 1,
		  &dummy)) != 0) {
    counter += bytes_read;
    if (counter > buffer_size)
      goto done;
  }
  #ifdef _DEBUG
	printf("OV read done\n");
#endif
done:
  if (ov_open_success)
    ov_clear(vf);
  if (src)
    SDL_FreeRW(src);
  if (vf)
    free(vf);
  return sound;
}

void audio_set_single(Sound * sound, Uint8 single)
{
  sound->single = single;
}

void audio_free_sound(Sound * sound)
{
  SoundNode *node = NULL, *prev = NULL, *temp = NULL;

  if (sound) {
    prev = NULL;
    node = _sound_node;
    while (node) {
      if (node->sound == sound) {
	if (prev)
	  prev->next = node->next;
	else
	  _sound_node = node->next;
	temp = node->next;
	free(node);
	node = temp;
      } else {
	prev = node;
	node = node->next;
      }
    }
    SDL_FreeWAV(sound->buffer);
    free(sound);
  }
}
void *
revmemcpy_snd (void *dest, const void *src, size_t len)
{
  char *d = dest + len - 1;
  const char *s = src;
  while (len--)
    *d-- = *s++;
  return dest;
}
