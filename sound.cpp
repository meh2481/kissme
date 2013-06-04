#include "sound.h"
#include "signalhandler.h"
#include <iostream>
#include <list>
#include <fstream>

//Global variables for use by our functions here
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

    if(handle == TYRSOUND_NULLHANDLE)
    {
        std::cout << "Invalid handle for song " << sFilename << std::endl;
        exit(1);
    }


    tyrsound_Error err = tyrsound_play(handle);

    if(err != TYRSOUND_ERR_OK)
    {
        std::cout << "Failed to start playback on file " << sFilename << " Err: " << err << std::endl;
        exit(1);
    }
}

gboolean check_music_playing(gpointer data)
{
    if(!tyrsound_isPlaying(handle))
    {
        switch(iRepeatMode)
        {
            case REPEAT_ALL:
                //TODO
                break;
            case REPEAT_NONE:
                //TODO stop
                break;
            case REPEAT_ONE:
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

    if(handle != TYRSOUND_NULLHANDLE)
        update_play_slider(tyrsound_getPlayPosition(handle), tyrsound_getLength(handle));   //Update the slider to show where our current song is playing

    return true;
}

void play_song()
{
    tyrsound_play(handle);
}

void pause_song()
{
    tyrsound_pause(handle);
}

void rewind_song()
{
    tyrsound_seek(handle, 0.0f);
}

void setVolume(float fVol)
{
    tyrsound_setVolume(handle, fVol);
}

void add_to_playlist(std::string sFilename)
{
    //Get data for song
    //ID3_Tag mp3Tag(sFilename.c_str());
    TagLib::FileRef f(sFilename.c_str());
    if(f.isNull())
    {
        add_song(sFilename, "", "", "", "");
        g_lCurPlaylist.push_back(sFilename);
        return;
    }
    std::string sAlbum = f.tag()->album().to8Bit(true);
    std::string sTitle = f.tag()->title().to8Bit(true);
//    std::string sLength = f.tag()->length().to8Bit(true);
    std::string sArtist = f.tag()->artist().to8Bit(true);

    //Write this all to the proper location in the table
    add_song(sFilename, sTitle, sArtist, sAlbum, "");
    g_lCurPlaylist.push_back(sFilename);
}

void save_playlist()
{
    //For now, just shove all the data out to the file, without caring about format
    std::ofstream playlistFile("kissme.last");
    if(playlistFile.fail()) return;
    for(std::list<std::string>::iterator i = g_lCurPlaylist.begin(); i != g_lCurPlaylist.end(); i++)
    {
        playlistFile << *i << std::endl;
    }
    playlistFile.close();
}

void load_playlist()
{
    std::ifstream playlistFile("kissme.last");
    while(!playlistFile.fail() && !playlistFile.eof())
    {
        std::string s;
        getline(playlistFile, s);
        if(s.size())
        {
            add_to_playlist(s);
        }
    }
    playlistFile.close();
}

void set_music_loc(float fPos)
{
    if(handle != TYRSOUND_NULLHANDLE)
        tyrsound_seek(handle, fPos*tyrsound_getLength(handle));
}



