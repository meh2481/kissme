#include "sound.h"
#include <iostream>
using namespace std;

//Global variables for use by our functions here
Mix_Music *music = NULL;

void init_sdl()
{

    if (SDL_Init(SDL_INIT_AUDIO) == -1)
    {
        cout << "SDL_Init() failed! reason: " << SDL_GetError() << endl;
        exit(1);
    }

    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096)==-1)
    {
        cout << "Mix_OpenAudio: " << Mix_GetError() << endl;
        exit(1);
    }

	atexit(SDL_Quit);
	atexit(Mix_CloseAudio);
}

void play_song(string sFilename)
{
    if(music != NULL)
    {
        Mix_FreeMusic(music);
    }
    music = Mix_LoadMUS(sFilename.c_str());
    Mix_PlayMusic(music, 0);
    Mix_PauseMusic();
}
