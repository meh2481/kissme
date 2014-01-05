#include "sound.h"
#include "signalhandler.h"
#include "cover.h"
#include <iostream>
#include <list>
#include <fstream>
#include <sstream>

//Global variables for use by our functions here
tyrsound_Handle handle = TYRSOUND_NULLHANDLE;
extern int      iRepeatMode;
extern bool bPaused;
std::list<std::string>    g_lCurPlaylist; //Current list of songs we're playing

void init_sound()
{
    if(tyrsound_init(NULL, NULL) != TYRSOUND_ERR_OK)
    {
        std::cout << "Failed to init tyrsound." << std::endl;
        exit(1);
    }
}

void cleanup_sound()
{
    tyrsound_shutdown();
}

bool song_is_valid()
{
	return (handle != TYRSOUND_NULLHANDLE);
}

bool is_playing()
{
	return tyrsound_isPlaying(handle);
}

void load_song(std::string sFilename)
{
    //Make sure this file format is supported
#ifndef OGG_SUPPORT
    if(sFilename.find(".ogg") != std::string::npos)
    {
        std::cout << "Ogg playback not supported at this time" << std::endl;
        return;
    }
#endif
#ifndef MP3_SUPPORT
    if(sFilename.find(".mp3") != std::string::npos)
    {
        std::cout << "MP3 playback not supported at this time" << std::endl;
        return;
    }
#endif
#ifndef OPUS_SUPPORT
    if(sFilename.find(".opus") != std::string::npos)
    {
        std::cout << "Opus playback not supported at this time" << std::endl;
        return;
    }
#endif
#ifndef FLAC_SUPPORT
    if(sFilename.find(".flac") != std::string::npos)
    {
        std::cout << "Flac playback not supported at this time" << std::endl;
        return;
    }
#endif
#ifndef WAV_SUPPORT
    if(sFilename.find(".wav") != std::string::npos)
    {
        std::cout << "Wav playback not supported at this time" << std::endl;
        return;
    }
#endif
    if(handle != TYRSOUND_NULLHANDLE)
    {
        tyrsound_stop(handle);
        tyrsound_unload(handle);
    }

    tyrsound_Stream strm;
    if(tyrsound_createFileNameStream(&strm, sFilename.c_str(), "rb") != TYRSOUND_ERR_OK)
    {
        std::cout << "File not found: " << sFilename << std::endl;
        return;
    }

    handle = tyrsound_load(strm);

    if(handle == TYRSOUND_NULLHANDLE)
    {
        std::cout << "Invalid handle for song " << sFilename << std::endl;
        return;
    }


    tyrsound_Error err = tyrsound_play(handle);

    if(err != TYRSOUND_ERR_OK)
    {
        std::cout << "Failed to start playback on file " << sFilename << " Err: " << err << std::endl;
        return;
    }
}

gboolean check_music_playing(gpointer data)
{
    if(!tyrsound_isPlaying(handle) && !bPaused)
    {
        switch(iRepeatMode)
        {
            case REPEAT_ALL:
                next_song(true);
                break;
            case REPEAT_NONE:
                next_song(false);
                break;
            case REPEAT_ONE:
                //tyrsound_stop(handle);
                //tyrsound_play(handle);
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
    tyrsound_stop(handle);
    tyrsound_seek(handle, 0);
    tyrsound_play(handle);
}

void stop_song()
{
	if(handle != TYRSOUND_NULLHANDLE)
  {
      tyrsound_stop(handle);
      tyrsound_unload(handle);
  }
  handle = TYRSOUND_NULLHANDLE;
}

void setVolume(float fVol)
{
    tyrsound_setVolume(handle, fVol);
}

void loop_song(bool bLoop)
{
	if(bLoop)
		tyrsound_setLoop(handle, 0.0f, -1);
	else
		tyrsound_setLoop(handle, 0.0f, 0);
}

void add_to_playlist(std::string sFilename)
{
    //Get data for song
    //ID3_Tag mp3Tag(sFilename.c_str());
    TagLib::FileRef f(sFilename.c_str());
    if(f.isNull())
    {
        add_song(sFilename, "", "", "", 0, 0.0);
        g_lCurPlaylist.push_back(sFilename);
        return;
    }
    std::string sAlbum = f.tag()->album().to8Bit(true);
    std::string sTitle = f.tag()->title().to8Bit(true);
    std::string sArtist = f.tag()->artist().to8Bit(true);
    uint iTrack = f.tag()->track();

    //Write this all to the proper location in the table
    add_song(sFilename, sTitle, sArtist, sAlbum, iTrack, f.audioProperties()->length());
    g_lCurPlaylist.push_back(sFilename);
}

void save_playlist()
{
    //For now, just shove all the data out to the file, without caring about format
    std::ofstream playlistFile("kissme.last");
    if(playlistFile.fail()) return;
    std::list<std::string> playlist = get_cur_playlist();
    for(std::list<std::string>::iterator i = playlist.begin(); i != playlist.end(); i++)
        playlistFile << *i << std::endl;
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

bool change_tag(std::string sFilename, tagType tagToChange, std::string sNewTag)
{
    #ifndef BOTCHED_TAGGING
    if(sFilename.find(".m4a") != std::string::npos || sFilename.find(".wma") != std::string::npos)
    {
        std::cout << "WMA and M4A tagging is broken currently, sorry. Complain to the TagLib devs." << std::endl;
        return false;
    }
    #endif
    TagLib::FileRef f(sFilename.c_str());

    if(f.isNull())  //Can't really do anything
    {
        std::cout << "Warning: can't change tag on file " << sFilename << std::endl;
        return false;
    }

    std::istringstream iss(sNewTag);
    int32_t track = 0;
    switch(tagToChange)
    {
        case CHANGE_ARTIST:
            f.tag()->setArtist(sNewTag);
            break;
        case CHANGE_ALBUM:
            f.tag()->setAlbum(sNewTag);
            break;
        case CHANGE_TITLE:
            f.tag()->setTitle(sNewTag);
            break;
        case CHANGE_TRACK:
            if(!sNewTag.size()) //Erase tag by clearing it
            {
                f.tag()->setTrack(0);
                break;
            }
            if(!(iss >> track) || track <= 0)   //Only accept valid numbers; no negative numbers allowed    //TODO: What if we type something like "1noodle"? How should I handle this?
                return false;
            f.tag()->setTrack(track);
            break;
        default:
            std::cout << "Err: Unknown tag type " << tagToChange << ". Ignoring..." << std::endl;
            break;
    }

    if(!f.save())
    {
        std::cout << "Err: Unable to save tag changes to file " << sFilename << "." << std::endl;
        return false;
    }

    return true;
}

float get_song_length()
{
    if(handle == TYRSOUND_NULLHANDLE)
        return 0.0f;
    return tyrsound_getLength(handle);
}








