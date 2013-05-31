#include "sound.h"
#include "signalhandler.h"
#include <iostream>
using namespace std;

//Global variables for use by our functions here
Mix_Music *music = NULL;
bool bMusicDone = false;
extern int iRepeatMode;

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
    Mix_HookMusicFinished(music_done);
	atexit(SDL_Quit);
	atexit(Mix_CloseAudio);
}

void load_song(string sFilename)
{
    if(music != NULL)
    {
        Mix_FreeMusic(music);
    }
    music = Mix_LoadMUS(sFilename.c_str());
    Mix_PlayMusic(music, 0);
    Mix_PauseMusic();

    //TODO: Fill in data for song   //TODO While loading...
    ID3_Tag mp3Tag(sFilename.c_str());

    //Get album
    ID3_Frame* myFrame = mp3Tag.Find(ID3FID_ALBUM);
    if (myFrame != NULL)
    {
        ID3_Field* myField = myFrame->GetField(ID3FN_TEXT);
        if(myField != NULL)
        {
            cout << "Album: " << myField->GetRawText() << endl;
        }
    }

    //Get title
    myFrame = mp3Tag.Find(ID3FID_TITLE);
    if (myFrame != NULL)
    {
        ID3_Field* myField = myFrame->GetField(ID3FN_TEXT);
        if(myField != NULL)
        {
            cout << "Title: " << myField->GetRawText() << endl;
        }
    }

    //Get song length (Doesn't work)
    myFrame = mp3Tag.Find(ID3FID_SONGLEN);
    if (myFrame != NULL)
    {
        ID3_Field* myField = myFrame->GetField(ID3FN_TEXT);
        if(myField != NULL)
        {
            cout << "Length: " << myField->GetRawText() << endl;
        }
    }

    //Get
    myFrame = mp3Tag.Find(ID3FID_LEADARTIST);
    if (myFrame != NULL)
    {
        ID3_Field* myField = myFrame->GetField(ID3FN_TEXT);
        if(myField != NULL)
        {
            cout << "Artist: " << myField->GetRawText() << endl;
        }
    }
}

gboolean check_music_playing(gpointer data)
{
    //cout << "Hai dood" << endl;
    if(bMusicDone)
    {
        bMusicDone = false;
        //cout << "Repeat and stuff" << endl;
        switch(iRepeatMode)
        {
            case REPEAT_ALL:
                //TODO
                break;
            case REPEAT_NONE:
                //TODO stop
                break;
            case REPEAT_ONE:
                //Mix_RewindMusic();
                Mix_PlayMusic(music, 0);
                break;
        }
    }
    if(Mix_PlayingMusic() && !Mix_PausedMusic())
        show_pause();
    else
        show_play();

    return true;
}

void music_done()
{
    bMusicDone = true;
}

void play_song()
{
    Mix_ResumeMusic();
}

void pause_song()
{
    Mix_PauseMusic();
}

void rewind_song()
{
    Mix_RewindMusic();
}

void setVolume(float fVol)
{
    Mix_VolumeMusic(fVol*128);
}
