#ifndef SOUND_PLAYBACK_H
#define SOUND_PLAYBACK_H

#include <string>
#include <gtk/gtk.h>
#include <tag.h>
#include <fileref.h>
#include <tyrsound.h>

//For changing audio file tags
#define CHANGE_TITLE    0
#define CHANGE_ARTIST   1
#define CHANGE_ALBUM    2
#define CHANGE_TRACK    3
typedef uint8_t tagType;

//Global functions
void        init_sound();                     //Initializes tyrsound
void        cleanup_sound();                    //Closes tyrsound
void        load_song(std::string sFilename);    //Loads a song from a filename
void        play_song();                    //Starts playing the last song loaded into memory
void        pause_song();                   //Pauses the currently playing music
void        rewind_song();                  //Rewinds the currently-playing song to the beginning
void        setVolume(float fVol);          //Set the current volume (between 0 and 1)
void        add_to_playlist(std::string sFilename);  //Add this song to the current playlist
void        save_playlist();                //Save current playlist to file
void        load_playlist();                //Load the playlist we saved
void        set_music_loc(float fPos);      //Set the music to play from pos (0.0 to 1.0)
bool        change_tag(std::string sFilename, tagType tagToChange, std::string sNewTag);    //Change the tag in a music file
float       get_song_length();

//Callbacks (Don't call directly)
gboolean    check_music_playing(gpointer data); //Callback for every loop to see if we should play_song next song


//Supported file types
#define OGG_SUPPORT
#define OPUS_SUPPORT
//#define FLAC_SUPPORT
//#define WAV_SUPPORT
//#define MP3_SUPPORT







#endif
