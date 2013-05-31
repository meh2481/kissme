#ifndef SOUND_PLAYBACK_H
#define SOUND_PLAYBACK_H

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <string>
#include <gtk/gtk.h>
#include <id3/tag.h>
using namespace std;


//Global functions
void        init_sdl();                     //Initializes SDL
void        load_song(string sFilename);    //Loads a song from a filename
void        play_song();                    //Starts playing the last song loaded into memory
void        pause_song();                   //Pauses the currently playing music
void        rewind_song();                  //Rewinds the currently-playing song to the beginning
void        setVolume(float fVol);          //Set the current volume (between 0 and 1)

//Callbacks (Don't call directly)
gboolean    check_music_playing(gpointer data); //Callback for every loop to see if we should play_song next song
void        music_done();   //Callback for when music finishes









#endif
