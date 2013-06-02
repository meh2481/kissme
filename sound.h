#ifndef SOUND_PLAYBACK_H
#define SOUND_PLAYBACK_H

//#include <SDL/SDL.h>
//#include <SDL/SDL_mixer.h>
#include <string>
#include <gtk/gtk.h>
#include <id3/tag.h>
#include <tyrsound.h>
//using namespace std;


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

//Callbacks (Don't call directly)
gboolean    check_music_playing(gpointer data); //Callback for every loop to see if we should play_song next song
//void        music_done();   //Callback for when music finishes









#endif
