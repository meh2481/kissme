#include "sound.h"
#include "signalhandler.h"
#include <iostream>
#include <list>
using namespace std;

//Global variables for use by our functions here
Mix_Music       *music = NULL;
bool            bMusicDone = false;
extern int      iRepeatMode;
list<string>    g_lCurPlaylist; //Current list of songs we're playing

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

char *ID3_GetString(const ID3_Frame *frame, ID3_FieldID fldName)
{
    char *text = NULL;
    if (NULL != frame)
    {
        ID3_Field* fld = frame->GetField(fldName);
        ID3_TextEnc enc = fld->GetEncoding();
        fld->SetEncoding(ID3TE_ASCII);
        size_t nText = fld->Size();
        text = new char[nText + 1];
        fld->Get(text, nText + 1);
        fld->SetEncoding(enc);
    }
    return text;
}

void add_to_playlist(string sFilename)
{
    //Get data for song
    ID3_Tag mp3Tag(sFilename.c_str());
    string sAlbum = "\0";
    string sTitle = "\0";
    string sLength = "\0";
    string sArtist = "\0";

    //Get album
    //cout << "Filename load: " << sFilename << endl;
    ID3_Frame* myFrame = mp3Tag.Find(ID3FID_ALBUM);
    char* cs = NULL;
    cs = ID3_GetString(myFrame, ID3FN_TEXT);
    if(cs != NULL)
        sAlbum = cs;
    myFrame = mp3Tag.Find(ID3FID_TITLE);
    cs = ID3_GetString(myFrame, ID3FN_TEXT);
    if(cs != NULL)
        sTitle = cs;
    myFrame = mp3Tag.Find(ID3FID_SONGLEN);
    cs = ID3_GetString(myFrame, ID3FN_TEXT);
    if(cs != NULL)
        sLength = cs;
    myFrame = mp3Tag.Find(ID3FID_LEADARTIST);
    cs = ID3_GetString(myFrame, ID3FN_TEXT);
    if(cs != NULL)
        sArtist = cs;

    //Write this all to the proper location in the table
    add_song(sFilename, sTitle, sArtist, sAlbum, sLength);
    g_lCurPlaylist.push_back(sFilename);
}

void save_playlist()
{
    //For now, just shove all the data out to the file, without caring about format
    ofstream playlistFile("kissme.last");
    if(playlistFile.fail()) return;
    for(list<string>::iterator i = g_lCurPlaylist.begin(); i != g_lCurPlaylist.end(); i++)
    {
        playlistFile << *i << endl;
    }
    playlistFile.close();
}

void load_playlist()
{
    ifstream playlistFile("kissme.last");
    while(!playlistFile.fail() && !playlistFile.eof())
    {
        string s;
        getline(playlistFile, s);
        if(s.size())
        {
            //g_lCurPlaylist.push_back(s);
            add_to_playlist(s);
        }
    }
    playlistFile.close();
}
