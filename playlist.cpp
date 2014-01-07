#include "playlist.h"
#include "signalhandler.h"
#include "sound.h"
#include "tinyxml2.h"
#include <fstream>
#include <VFSTools.h>
#include <sstream>
using namespace tinyxml2;

std::string stripcarriageret(std::string s) //For some reason, some files contain carriage returns, and Linux doesn't like them.
{
	size_t pos = s.find('\r');
	while(pos != std::string::npos)
 	{
 		s.erase(pos, 1);	//Replace carriage returns
 		pos = s.find('\r');
 	}
 	return s;
}

//Remove all comments after delim is encountered on this line
void removecomment(std::string& s, char delim)
{
	size_t pos = s.find(delim);
	if(pos != std::string::npos)
		s.erase(pos);
	s = stripcarriageret(s);
}

void strip_leading_whitespace(std::string& s)
{
	while(s[0] < '!') s.erase(0,1);
}

std::set<std::string> get_playlisttypes_supported()
{
	std::set<std::string> sList;
	sList.insert("m3u");
	sList.insert("m3u8");
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
	std::list<std::string> ret;
	if(sFilename.find(".m3u", sFilename.size() - 4) != std::string::npos ||
		 sFilename.find(".m3u8", sFilename.size() - 5) != std::string::npos)
		ret = convert_to_global(playlist_load_M3U(sFilename), ttvfs::StripLastPath(sFilename));
	else if(sFilename.find(".asx", sFilename.size() - 4) != std::string::npos)
		ret = convert_to_global(playlist_load_ASX(sFilename), ttvfs::StripLastPath(sFilename));
	else if(sFilename.find(".pls", sFilename.size() - 4) != std::string::npos)
		ret = convert_to_global(playlist_load_PLS(sFilename), ttvfs::StripLastPath(sFilename));
	else if(sFilename.find(".wpl", sFilename.size() - 4) != std::string::npos)
		ret = convert_to_global(playlist_load_WPL(sFilename), ttvfs::StripLastPath(sFilename));
	else if(sFilename.find(".xspf", sFilename.size() - 5) != std::string::npos)
		ret = convert_to_global(playlist_load_XSPF(sFilename), ttvfs::StripLastPath(sFilename));
	else if(sFilename.find(".vlc", sFilename.size() - 4) != std::string::npos)
		ret = convert_to_global(playlist_load_VLC(sFilename), ttvfs::StripLastPath(sFilename));
	else if(sFilename.find(".xml", sFilename.size() - 4) != std::string::npos)
		ret = convert_to_global(playlist_load_iTunes(sFilename), ttvfs::StripLastPath(sFilename));
	else if(sFilename.find(".kiss", sFilename.size() - 5) != std::string::npos)
		ret = convert_to_global(playlist_load_kissme(sFilename), ttvfs::StripLastPath(sFilename));
	else
		std::cout << "Playlist file format " << sFilename.substr(sFilename.size() - 3) << " unsupported." << std::endl;
		
	return ret;
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
      removecomment(s, '#');		//Skip over m3u comments; we'll load all track data ourselves
      if(s.size())
          ret.push_back(s);
  }
  infile.close();
	
	return ret;
}

std::list<std::string> playlist_load_ASX(std::string sFilename)
{
	std::list<std::string> ret;
	
  XMLDocument* doc = new XMLDocument;
  int iErr = doc->LoadFile(sFilename.c_str());
  if(iErr != XML_NO_ERROR)
  {
		std::cout << "Error parsing XML file " << sFilename << ": Error " << iErr << std::endl;
		delete doc;
		return ret;
  }
  
  //Grab root element
  XMLElement* root = doc->RootElement();
  if(root == NULL)
  {
		std::cout << "Error: Root element NULL in XML file " << sFilename << std::endl;
		delete doc;
		return ret;
  }
  
  //Read entries
  for(XMLElement* entry = root->FirstChildElement("entry"); entry != NULL; entry = entry->NextSiblingElement("entry"))
  {
  	XMLElement* ref = entry->FirstChildElement("ref");
  	if(ref != NULL)
  	{
  		const char* href = ref->Attribute("href");
  		if(href != NULL)
  		{
  			std::string s = href;
  			ret.push_back(s);
			}
		}
  }
	
	delete doc;
	return ret;
}

std::list<std::string> playlist_load_PLS(std::string sFilename)
{
	std::list<std::string> ret;
	
	std::ifstream playlistFile(sFilename.c_str());
	int num = 0;
	if(!playlistFile.fail())
	{	
    std::string s;
    
    //Header starts with a [playlist] line
    while(!s.size() && !playlistFile.fail())
   	{
   		getline(playlistFile,s);
   		removecomment(s, ';');
   		strip_leading_whitespace(s);
   	}
    if(s != "[playlist]") return ret;	//malformed
   	s.clear();
   	
   	//Next up is a line that tells us how many items
    while(!s.size() && !playlistFile.fail())
   	{
   		getline(playlistFile,s);
   		removecomment(s, ';');
   		strip_leading_whitespace(s);
   	}
   	if(s.find("NumberOfEntries") == std::string::npos) return ret;	//malformed if this missing
   	s.erase(0, 15);	//Strip off "NumberOfEntries"
   	strip_leading_whitespace(s);	//Strip any whitespace
   	s.erase(0,1);	//Erase '=' character
   	strip_leading_whitespace(s);	//Strip any whitespace
   	std::istringstream iss(s);
   	iss >> num;
   	
	}
	int cur = 0;
  while(!playlistFile.fail() && !playlistFile.eof() && cur < num)
  {
    std::string s;
    getline(playlistFile, s);
    removecomment(s, ';');
   	strip_leading_whitespace(s);
    if(s.size())
    {
  		cur++;
  		std::ostringstream oss;
  		oss << "File" << cur;
  		if(s.find(oss.str()))	//"FileX" isn't the beginning of this line; ignore
  		{
  			cur--;
  			continue;
			}
			
			s.erase(0, oss.str().size());	//Strip off "FileX"
			strip_leading_whitespace(s);	//Strip any whitespace
		 	s.erase(0,1);	//Erase '=' character
		 	strip_leading_whitespace(s);	//Strip any whitespace
			
			//Done; add file
      ret.push_back(s);
    }
    
  }
  playlistFile.close();
	
	return ret;
}

std::list<std::string> playlist_load_WPL(std::string sFilename)
{
	std::list<std::string> ret;
	
	XMLDocument* doc = new XMLDocument;
  int iErr = doc->LoadFile(sFilename.c_str());
  if(iErr != XML_NO_ERROR)
  {
		std::cout << "Error parsing XML file " << sFilename << ": Error " << iErr << std::endl;
		delete doc;
		return ret;
  }
  
  //Grab root element
  XMLElement* root = doc->RootElement();
  if(root == NULL)
  {
		std::cout << "Error: Root element NULL in XML file " << sFilename << std::endl;
		delete doc;
		return ret;
  }
  
  //Read entries
  for(XMLElement* body = root->FirstChildElement("body"); body != NULL; body = body->NextSiblingElement("body"))
  {
  	for(XMLElement* seq = body->FirstChildElement("seq"); seq != NULL; seq = seq->NextSiblingElement("seq"))
  	{
  		for(XMLElement* media = seq->FirstChildElement("media"); media != NULL; media = media->NextSiblingElement("media"))
  		{
				const char* src = media->Attribute("src");
				if(src != NULL)
					ret.push_back(src);
			}
  	}
  }
	delete doc;
	return ret;
}

std::list<std::string> playlist_load_XSPF(std::string sFilename)
{
	std::list<std::string> ret;
	
	XMLDocument* doc = new XMLDocument;
  int iErr = doc->LoadFile(sFilename.c_str());
  if(iErr != XML_NO_ERROR)
  {
		std::cout << "Error parsing XML file " << sFilename << ": Error " << iErr << std::endl;
		delete doc;
		return ret;
  }
  
  //Grab root element
  XMLElement* root = doc->RootElement();
  if(root == NULL)
  {
		std::cout << "Error: Root element NULL in XML file " << sFilename << std::endl;
		delete doc;
		return ret;
  }
  
  //Read entries
  for(XMLElement* tracklist = root->FirstChildElement("trackList"); tracklist != NULL; tracklist = tracklist->NextSiblingElement("trackList"))
  {
  	for(XMLElement* track = tracklist->FirstChildElement("track"); track != NULL; track = track->NextSiblingElement("track"))
  	{
  		XMLElement* location = track->FirstChildElement("location");
  		if(location != NULL)
  		{
				const char* src = location->GetText();
				if(src != NULL)
					ret.push_back(src);
			}
  	}
  }
	
	
	delete doc;
	return ret;
}

std::list<std::string> playlist_load_VLC(std::string sFilename)
{
	std::cout << "TODO: vlc support" << std::endl;
	std::list<std::string> ret;
	return ret;
}

std::list<std::string> playlist_load_iTunes(std::string sFilename)
{
	std::cout << "TODO: iTunes support" << std::endl;
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
		std::string s = convert_to_path(*i);
		if(!ttvfs::IsDirectory(ttvfs::StripLastPath(s).c_str()))	//Relative path
			ret.push_back(ttvfs::FixPath(sPath + '/' + s));
		else																							//Absolute path
			ret.push_back(ttvfs::FixPath(s));
	}
	return ret;
}

std::string convert_to_path(std::string sURI)
{
	if(sURI.find("file://") != 0) return sURI;	//Already formatted fine
	
	sURI.erase(0, 7);	//Erase "file://" part
	if(sURI.find("localhost") == 0)
		sURI.erase(0, 9);	//Erase "localhost" part
	sURI.erase(0,1);		//Erase backslash
	
	//Now, parse through, and replace escape sequences with proper value
	for(size_t pos = sURI.find('%', 0); pos != std::string::npos; pos = sURI.find('%', pos))
	{		
		//Get the two-digit hex value of this sequence
		std::istringstream iss(sURI.substr(pos+1, 2));
		int hex;
		iss >> std::hex >> hex;
		
		//Replace this portion of the string with this sequence
		sURI.replace(pos, 3, 1, (char)hex);
		pos++;	//Skip over this character for next search, in case we just placed a '%' character
	}
	return sURI;
}























