#include "playlist.h"
#include "signalhandler.h"
#include "sound.h"
#include "tinyxml2.h"
#include <fstream>

std::set<std::string> get_playlisttypes_supported()
{
	std::set<std::string> sList;
	sList.insert("m3u");
	sList.insert("asx");
	sList.insert("pls");
	sList.insert("wpl");
	sList.insert("xspf");
	sList.insert("vlc");
	sList.insert("xml");
	sList.insert("kiss");
	return sList;
}

std::list<std::string> playlist_load(std::string sFilename)
{
	if(sFilename.find(".m3u", sFilename.size() - 4) != std::string::npos)
		return playlist_load_M3U(sFilename);
	if(sFilename.find(".asx", sFilename.size() - 4) != std::string::npos)
		return playlist_load_ASX(sFilename);
	if(sFilename.find(".pls", sFilename.size() - 4) != std::string::npos)
		return playlist_load_PLS(sFilename);
	if(sFilename.find(".wpl", sFilename.size() - 4) != std::string::npos)
		return playlist_load_WPL(sFilename);
	if(sFilename.find(".xspf", sFilename.size() - 5) != std::string::npos)
		return playlist_load_XSPF(sFilename);
	if(sFilename.find(".vlc", sFilename.size() - 4) != std::string::npos)
		return playlist_load_VLC(sFilename);
	if(sFilename.find(".xml", sFilename.size() - 4) != std::string::npos)
		return playlist_load_iTunes(sFilename);
	if(sFilename.find(".kiss", sFilename.size() - 4) != std::string::npos)
		return playlist_load_kissme(sFilename);
	else
		std::cout << "Playlist file format " << sFilename.substr(sFilename.size() - 3) << " unsupported." << std::endl;
		
	std::list<std::string> ret;
	return ret;		//Something went wrong; return empty list
}

std::list<std::string> playlist_load_M3U(std::string sFilename)
{
	std::cout << "m3u" << std::endl;
	std::list<std::string> ret;
	return ret;
}

std::list<std::string> playlist_load_ASX(std::string sFilename)
{
	std::cout << "asx" << std::endl;
	std::list<std::string> ret;
	return ret;
}

std::list<std::string> playlist_load_PLS(std::string sFilename)
{
	std::cout << "pls" << std::endl;
	std::list<std::string> ret;
	return ret;
}

std::list<std::string> playlist_load_WPL(std::string sFilename)
{
	std::cout << "wpl" << std::endl;
	std::list<std::string> ret;
	return ret;
}

std::list<std::string> playlist_load_XSPF(std::string sFilename)
{
	std::cout << "xspf" << std::endl;
	std::list<std::string> ret;
	return ret;
}

std::list<std::string> playlist_load_VLC(std::string sFilename)
{
	std::cout << "vlc" << std::endl;
	std::list<std::string> ret;
	return ret;
}

std::list<std::string> playlist_load_iTunes(std::string sFilename)
{
	std::cout << "iTunes" << std::endl;
	std::list<std::string> ret;
	return ret;
}

std::list<std::string> playlist_load_kissme(std::string sFilename)
{
	std::cout << "kiss" << std::endl;
	std::list<std::string> ret;
	return ret;
}

void save_playlist()
{
    //For now, just shove all the data out to the file, without caring about format
    std::ofstream playlistFile("last.kiss");
    if(playlistFile.fail()) return;
    std::list<std::string> playlist = get_cur_playlist();
    for(std::list<std::string>::iterator i = playlist.begin(); i != playlist.end(); i++)
        playlistFile << *i << std::endl;
    playlistFile.close();
}

void load_playlist()
{
    std::ifstream playlistFile("last.kiss");
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














