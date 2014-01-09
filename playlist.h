#ifndef PLAYLIST_H
#define PLAYLIST_H
#include <string>
#include <list>
#include <set>

std::list<std::string> playlist_load(std::string sFilename);			//Calls the appropriate function of the following:
std::list<std::string> playlist_load_M3U(std::string sFilename);
std::list<std::string> playlist_load_ASX(std::string sFilename);
std::list<std::string> playlist_load_PLS(std::string sFilename);	//dood, pls
std::list<std::string> playlist_load_WPL(std::string sFilename);
std::list<std::string> playlist_load_XSPF(std::string sFilename);
std::list<std::string> playlist_load_iTunes(std::string sFilename);
std::list<std::string> playlist_load_kissme(std::string sFilename);

void playlist_save_kissme(std::string sFilename, std::list<std::string> sFiles);	//Save given playlist in kissme format
void playlist_save_M3U(std::string sFilename, std::list<std::string> sFiles);			//Save given playlist in M3U format

std::set<std::string> get_playlisttypes_supported();	//Get the types of playlists we support
void        save_config();                //Save current playlist to file
void        load_config();                //Load the playlist we saved
std::list<std::string>	convert_to_global(std::list<std::string> sFilenames, std::string sPath);	//Convert the list of filenames to be global, rather than relative, paths.
std::string convert_to_path(std::string sURI);	//Convert a file:/// URI to an actual system path

void playlist_play(std::string sName);	//Show this playlist in the right pane
void playlist_add(std::string sName, std::list<std::string> sSongs);	//Add this playlist to our playlist manager

void save();	//Save both config and current playlist
void save_cur_playlist();







#endif
