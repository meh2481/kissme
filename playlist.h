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
std::list<std::string> playlist_load_VLC(std::string sFilename);
std::list<std::string> playlist_load_iTunes(std::string sFilename);
std::list<std::string> playlist_load_kissme(std::string sFilename);

std::set<std::string> get_playlisttypes_supported();	//Get the types of playlists we support
void        save_playlist();                //Save current playlist to file
void        load_playlist();                //Load the playlist we saved














#endif
