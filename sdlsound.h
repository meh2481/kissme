#ifndef SDL_SOUND_H
#define SDL_SOUND_H

#include <SDL/SDL.h>
#include <SDL/SDL_sound.h>
#include <string>
using namespace std;

// global decoding state.
typedef struct
{
    Uint8 *decoded_ptr;
    Uint32 decoded_bytes;
    int predecode;
    int looping;
    int wants_volume_change;
    float volume;
    Uint32 total_seeks;
    Uint32 *seek_list;
    Uint32 seek_index;
    Sint32 bytes_before_next_seek;
} playsound_global_state;


void init_sdl();
void play_song(string sFilename);



















#endif
