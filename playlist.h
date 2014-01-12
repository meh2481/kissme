#ifndef PLAYLIST_H
#define PLAYLIST_H
#include <string>
#include <list>
#include <set>
#include <gtk/gtk.h>
#include "song.h"

std::list<song> playlist_load(std::string sFilename);			//Calls the appropriate function of the following:
std::list<song> playlist_load_M3U(std::string sFilename);
std::list<song> playlist_load_ASX(std::string sFilename);
std::list<song> playlist_load_PLS(std::string sFilename);	//dood, pls
std::list<song> playlist_load_WPL(std::string sFilename);
std::list<song> playlist_load_XSPF(std::string sFilename);
std::list<song> playlist_load_iTunes(std::string sFilename);
std::list<song> playlist_load_kissme(std::string sFilename);

void playlist_save_kissme(std::string sFilename, std::list<song> sFiles);	//Save given playlist in kissme format
void playlist_save_M3U(std::string sFilename, std::string sName);		//Save given playlist in M3U format

std::set<std::string> get_playlisttypes_supported();								//Get the types of playlists we support
void        save_config();                													//Save current playlist to file
void        load_config();                													//Load the playlist we saved
std::list<song>	convert_to_global(std::list<song> sFilenames, std::string sPath);	//Convert the list of filenames to be global, rather than relative, paths.
std::string convert_to_path(std::string sURI);											//Convert a file:/// URI to an actual system path
void strip_leading_whitespace(std::string& s);
std::list<song> fill_out(std::list<song> songs, std::string sPath);	//Fill out song metadata from filename (Warning: Can take a while if a lot of songs)

void playlist_play(std::string sName);												//Show this playlist in the right pane
void playlist_add(std::string sName, std::list<song> sSongs);	//Add this playlist to our playlist manager

void save();																//Save both config and current playlist
void load();																//Load both config and playlists
void load_playlists();											//Load all playlists in user dir
gboolean save_cur_playlist(gpointer data);	//Save the currently-viewing playlist
void save_cur_playlist(std::string sName);	//Save the currently-viewing list under this name
void resort_playlist_pane();								//Resort the playlist pane to display playlists alphabetically

std::string playlist_currently_playing();	//TODO
GtkTreePath* path_currently_playing();		//TODO
std::string song_currently_playing();			//TODO
bool is_playlist(std::string sName);			//If this is a current playlist
void delete_playlist(std::string sName);	//Remove playlist forever
void rename_playlist(std::string sOldName, std::string sNewName);	//Rename a playlist to another
void playlist_add_commandline(int argc, char** argv);	//Parse the commandline and add songs on it







#endif
