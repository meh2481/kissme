#include "playlist.h"
#include "signalhandler.h"
#include "sound.h"
#include "tinyxml2.h"
#include <fstream>
#include <VFSTools.h>

std::string stripcarriageret(std::string s)
{
	size_t pos = s.find('\r');
	while(pos != std::string::npos)
 	{
 		s.erase(pos, 1);	//Replace carriage returns
 		pos = s.find('\r');
 	}
 	return s;
}

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
	std::list<std::string> ret;
	
	std::ifstream infile(sFilename.c_str());
	if(infile.fail())
	{
		std::cout << "Error: file " << sFilename << " does not exist." << std::endl;
		return ret;
	}
	while(!infile.fail() && !infile.eof())
  {
      std::string s;
      getline(infile, s);
      s = stripcarriageret(s);
      if(s.size() && s[0] != '#')	//Skip over m3u comments; we'll load all track data ourselves
          ret.push_back(s);
  }
  infile.close();
	
	return convert_to_global(ret, ttvfs::StripLastPath(sFilename));
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
	std::list<std::string> ret;
	std::ifstream playlistFile(sFilename.c_str());
  while(!playlistFile.fail() && !playlistFile.eof())
  {
      std::string s;
      getline(playlistFile, s);
      if(s.size())
      {
          ret.push_back(s);
      }
  }
  playlistFile.close();
	return ret;
}

void playlist_save_kissme(std::string sFilename, std::list<std::string> sFiles)
{
	std::ofstream playlistFile(sFilename.c_str());
  if(playlistFile.fail()) return;
  for(std::list<std::string>::iterator i = sFiles.begin(); i != sFiles.end(); i++)
      playlistFile << *i << std::endl;
  playlistFile.close();
}

void save_playlist()
{
    std::list<std::string> playlist = get_cur_playlist();
    playlist_save_kissme("last.kiss", playlist);
}

void load_playlist()
{
		std::list<std::string> sFiles = playlist_load_kissme("last.kiss");
    for(std::list<std::string>::iterator i = sFiles.begin(); i != sFiles.end(); i++)
    	add_to_playlist(*i);
}

std::list<std::string> convert_to_global(std::list<std::string> sFilenames, std::string sPath)
{
	std::list<std::string> ret;
	for(std::list<std::string>::iterator i = sFilenames.begin(); i != sFilenames.end(); i++)
	{
		ret.push_back(sPath + '/' + *i);
	}
	//TODO: Convert URIs
	return ret;
}












