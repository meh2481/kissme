#include "sound.h"
#include "signalhandler.h"
#include "cover.h"
#include "playlist.h"
#include <iostream>
#include <list>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <VFSTools.h>

//Global variables for use by our functions here
tyrsound_Handle handle = TYRSOUND_NULLHANDLE;
extern int      iRepeatMode;
extern bool bPaused;
static float g_fVolume = 1.0f;
//std::list<std::string>    g_lCurPlaylist; //Current list of songs we're playing

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
    if(sFilename.find(".ogg", sFilename.size() - 4) != std::string::npos)
    {
        std::cout << "Ogg playback not supported at this time" << std::endl;
        return;
    }
#endif
#ifndef MP3_SUPPORT
    if(sFilename.find(".mp3", sFilename.size() - 4) != std::string::npos)
    {
        std::cout << "MP3 playback not supported at this time" << std::endl;
        return;
    }
#endif
#ifndef OPUS_SUPPORT
    if(sFilename.find(".opus", sFilename.size() - 5) != std::string::npos)
    {
        std::cout << "Opus playback not supported at this time" << std::endl;
        return;
    }
#endif
#ifndef FLAC_SUPPORT
    if(sFilename.find(".flac", sFilename.size() - 5) != std::string::npos)
    {
        std::cout << "Flac playback not supported at this time" << std::endl;
        return;
    }
#endif
#ifndef WAV_SUPPORT
    if(sFilename.find(".wav", sFilename.size() - 4) != std::string::npos)
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

		tyrsound_setVolume(handle, g_fVolume);
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
		g_fVolume = fVol;
    tyrsound_setVolume(handle, fVol);
}

float getVolume()
{
	return g_fVolume;
}

void loop_song(bool bLoop)
{
	if(bLoop)
		tyrsound_setLoop(handle, 0.0f, -1);
	else
		tyrsound_setLoop(handle, 0.0f, 0);
}

song song_get_tags(std::string sSongFilename)
{
	song ret;
	ret.filename = sSongFilename;
	int len;
	song_get_tags(sSongFilename, ret.album, ret.title, ret.artist, ret.track, ret.length);
  return ret;
}

void song_get_tags(std::string sSongFilename, std::string& sAlbum, std::string& sTitle, std::string& sArtist, uint& iTrack, int& iLength)
{
	TagLib::FileRef f(sSongFilename.c_str());
  if(f.isNull())
  {
      sAlbum = sArtist = "";
      sTitle = sSongFilename;
      iTrack = iLength = 0;
      return;
  }
  sAlbum = f.tag()->album().to8Bit(true);
  sTitle = f.tag()->title().to8Bit(true);
  if(!sTitle.size())	//At least populate metadata if song filename isn't here
  	sTitle = ttvfs::StripFileExtension(ttvfs::PathToFileName(sSongFilename.c_str()));
  sArtist = f.tag()->artist().to8Bit(true);
  iTrack = f.tag()->track();
  iLength = f.audioProperties()->length();
}

void add_to_playlist(song s)
{
	add_song(s.filename, s.title, s.artist, s.album, s.track, s.length);
}

void add_to_playlist(std::string sFilename)
{
    //Get data for song
    std::string sAlbum, sTitle, sArtist;
    uint iTrack;
    int iLength;
    
    song_get_tags(sFilename, sAlbum, sTitle, sArtist, iTrack, iLength);

    //Write this all to the proper location in the table
    add_song(sFilename, sTitle, sArtist, sAlbum, iTrack, iLength);
    //g_lCurPlaylist.push_back(sFilename);
}

float get_song_length(std::string sFilename)
{
	float ret = 0.0f;
	TagLib::FileRef f(sFilename.c_str());
  if(!f.isNull())
  	return f.audioProperties()->length();
}

void set_music_loc(float fPos)
{
    if(handle != TYRSOUND_NULLHANDLE)
        tyrsound_seek(handle, fPos*tyrsound_getLength(handle));
}

bool change_tag(std::string sFilename, tagType tagToChange, gchar *sNewTag)
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
    TagLib::String utfs(sNewTag, TagLib::String::UTF8);	//Explicitly treat as UTF-8
    int32_t track = 0;
    switch(tagToChange)
    {
        case CHANGE_ARTIST:
            f.tag()->setArtist(utfs);
            break;
        case CHANGE_ALBUM:
            f.tag()->setAlbum(utfs);
            break;
        case CHANGE_TITLE:
            f.tag()->setTitle(utfs);
            break;
        case CHANGE_TRACK:
            if(!iss.str().size()) //Erase tag by clearing it
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








