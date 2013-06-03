#include "sound.h"
#include "signalhandler.h"
#include <iostream>
#include <list>
//using namespace std;

//Global variables for use by our functions here
//Mix_Music       *music = NULL;
//bool            bMusicDone = false;
tyrsound_Handle handle = TYRSOUND_NULLHANDLE;
extern int      iRepeatMode;
std::list<std::string>    g_lCurPlaylist; //Current list of songs we're playing

void init_sound()
{
    if(tyrsound_init(NULL, NULL) != TYRSOUND_ERR_OK)
    {
        std::cout << "Failed to init tyrsound." << std::endl;
        exit(1);
    }
    /*if (SDL_Init(SDL_INIT_AUDIO) == -1)
    {
        std::cout << "SDL_Init() failed! reason: " << SDL_GetError() << std::endl;
        exit(1);
    }

    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096)==-1)
    {
        std::cout << "Mix_OpenAudio: " << Mix_GetError() << std::endl;
        exit(1);
    }
//    Mix_HookMusicFinished(music_done);tyrsound_setVolume
	atexit(SDL_Quit);
	atexit(Mix_CloseAudio);*/
	atexit(cleanup_sound);
}

void cleanup_sound()
{
    tyrsound_shutdown();
}

void load_song(std::string sFilename)
{
    if(handle != TYRSOUND_NULLHANDLE)
    {
        tyrsound_unload(handle);
    }

    tyrsound_Stream strm;
    if(tyrsound_createFileNameStream(&strm, sFilename.c_str(), "rb") != TYRSOUND_ERR_OK)
    {
        std::cout << "File not found: " << sFilename << std::endl;
        exit(1);
    }

    handle = tyrsound_load(strm, NULL);

    if(tyrsound_play(handle) != TYRSOUND_ERR_OK)
    {
        std::cout << "Failed to start playback." << std::endl;
        exit(1);
    }



    //music = Mix_LoadMUS(sFilename.c_str());
    //Mix_PlayMusic(music, 0);
    //Mix_PauseMusic();


}

gboolean check_music_playing(gpointer data)
{
    //std::cout << "Hai dood" << std::endl;
    if(!tyrsound_isPlaying(handle))
    {
//        bMusicDone = false;
        //std::cout << "Repeat and stuff" << std::endl;
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
                //Mix_PlayMusic(music, 0);
//                tyrsound_stop(handle);
                tyrsound_seek(handle, 0.0);
                tyrsound_play(handle);
                break;
        }
    }
    if(tyrsound_isPlaying(handle))
        show_pause();
    else
        show_play();

    tyrsound_update();  //Update OpenAL

    update_play_slider(tyrsound_getPlayPosition(handle), tyrsound_getLength(handle));   //Update the slider to show where our current song is playing

    return true;
}

//void music_done()
//{
//    bMusicDone = true;
//

void play_song()
{
    //Mix_ResumeMusic();
    tyrsound_play(handle);
}

void pause_song()
{
    //Mix_PauseMusic();
    tyrsound_pause(handle);
}

void rewind_song()
{
    //Mix_RewindMusic();
    tyrsound_seek(handle, 0.0f);
    //tyrsound_stop(handle);
    //tyrsound_play(handle);
}

void setVolume(float fVol)
{
    //Mix_VolumeMusic(fVol*128);
    tyrsound_setVolume(handle, fVol*2.0);   //Loud is awesum
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

void add_to_playlist(std::string sFilename)
{
    //Get data for song
    ID3_Tag mp3Tag(sFilename.c_str());
    std::string sAlbum = "\0";
    std::string sTitle = "\0";
    std::string sLength = "\0";
    std::string sArtist = "\0";

    //Get album
    //std::cout << "Filename load: " << sFilename << std::endl;
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
    for(std::list<std::string>::iterator i = g_lCurPlaylist.begin(); i != g_lCurPlaylist.end(); i++)
    {
        playlistFile << *i << std::endl;
    }
    playlistFile.close();
}

void load_playlist()
{
    ifstream playlistFile("kissme.last");
    while(!playlistFile.fail() && !playlistFile.eof())
    {
        std::string s;
        getline(playlistFile, s);
        if(s.size())
        {
            //g_lCurPlaylist.push_back(s);
            add_to_playlist(s);
        }
    }
    playlistFile.close();
}
