#include "playlist.h"
#include "signalhandler.h"
#include "sound.h"
#include "tinyxml2.h"
#include <fstream>
#include <VFSTools.h>
#include <VFSSystemPaths.h>
#include <sstream>
#include <map>

extern GtkBuilder *builder;
static std::map<std::string, std::list<std::string> > g_mPlaylists;

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
	
  tinyxml2::XMLDocument* doc = new tinyxml2::XMLDocument;
  int iErr = doc->LoadFile(sFilename.c_str());
  if(iErr != tinyxml2::XML_NO_ERROR)
  {
		std::cout << "Error parsing XML file " << sFilename << ": Error " << iErr << std::endl;
		delete doc;
		return ret;
  }
  
  //Grab root element
  tinyxml2::XMLElement* root = doc->RootElement();
  if(root == NULL)
  {
		std::cout << "Error: Root element NULL in XML file " << sFilename << std::endl;
		delete doc;
		return ret;
  }
  
  //Read entries
  for(tinyxml2::XMLElement* entry = root->FirstChildElement("entry"); entry != NULL; entry = entry->NextSiblingElement("entry"))
  {
  	tinyxml2::XMLElement* ref = entry->FirstChildElement("ref");
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
    if(s != "[playlist]") 
    {
    	playlistFile.close();
    	return ret;	//malformed
    }
   	s.clear();
   	
   	//Next up is a line that tells us how many items
    while(!s.size() && !playlistFile.fail())
   	{
   		getline(playlistFile,s);
   		removecomment(s, ';');
   		strip_leading_whitespace(s);
   	}
   	if(s.find("NumberOfEntries") == std::string::npos) 
   	{
   		playlistFile.close();
   		return ret;	//malformed if this missing
   	}
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
	
	tinyxml2::XMLDocument* doc = new tinyxml2::XMLDocument;
  int iErr = doc->LoadFile(sFilename.c_str());
  if(iErr != tinyxml2::XML_NO_ERROR)
  {
		std::cout << "Error parsing XML file " << sFilename << ": Error " << iErr << std::endl;
		delete doc;
		return ret;
  }
  
  //Grab root element
  tinyxml2::XMLElement* root = doc->RootElement();
  if(root == NULL)
  {
		std::cout << "Error: Root element NULL in XML file " << sFilename << std::endl;
		delete doc;
		return ret;
  }
  
  //Read entries
  for(tinyxml2::XMLElement* body = root->FirstChildElement("body"); body != NULL; body = body->NextSiblingElement("body"))
  {
  	for(tinyxml2::XMLElement* seq = body->FirstChildElement("seq"); seq != NULL; seq = seq->NextSiblingElement("seq"))
  	{
  		for(tinyxml2::XMLElement* media = seq->FirstChildElement("media"); media != NULL; media = media->NextSiblingElement("media"))
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
	
	tinyxml2::XMLDocument* doc = new tinyxml2::XMLDocument;
  int iErr = doc->LoadFile(sFilename.c_str());
  if(iErr != tinyxml2::XML_NO_ERROR)
  {
		std::cout << "Error parsing XML file " << sFilename << ": Error " << iErr << std::endl;
		delete doc;
		return ret;
  }
  
  //Grab root element
  tinyxml2::XMLElement* root = doc->RootElement();
  if(root == NULL)
  {
		std::cout << "Error: Root element NULL in XML file " << sFilename << std::endl;
		delete doc;
		return ret;
  }
  
  //Read entries
  for(tinyxml2::XMLElement* tracklist = root->FirstChildElement("trackList"); tracklist != NULL; tracklist = tracklist->NextSiblingElement("trackList"))
  {
  	for(tinyxml2::XMLElement* track = tracklist->FirstChildElement("track"); track != NULL; track = track->NextSiblingElement("track"))
  	{
  		tinyxml2::XMLElement* location = track->FirstChildElement("location");
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

//I considered briefly how I would know that a particular .xml file would be in this exact format. I then realized that
//Apple programmers are the only people stupid enough to actually name an XML-based playlist format with a .xml file
//extension. Problem avoided.
//
//Of course, a user may be that stupid as well, but the resulting playlist would just be blank, so no worries.
std::list<std::string> playlist_load_iTunes(std::string sFilename)
{
	std::list<std::string> ret;
	
	tinyxml2::XMLDocument* doc = new tinyxml2::XMLDocument;
  int iErr = doc->LoadFile(sFilename.c_str());
  if(iErr != tinyxml2::XML_NO_ERROR)
  {
		std::cout << "Error parsing XML file " << sFilename << ": Error " << iErr << std::endl;
		delete doc;
		return ret;
  }
  
  //Grab root element
  tinyxml2::XMLElement* root = doc->RootElement();
  if(root == NULL)
  {
		std::cout << "Error: Root element NULL in XML file " << sFilename << std::endl;
		delete doc;
		return ret;
  }
  
  //Am I being too harsh about Apple programmers? Oh, wait, what's this? Three nested XML elements, all with the same name? 
  //Nah, I'm not being too harsh.
  for(tinyxml2::XMLElement* dict = root->FirstChildElement("dict"); dict != NULL; dict = dict->NextSiblingElement("dict"))
  {
		for(tinyxml2::XMLElement* dict2 = dict->FirstChildElement("dict"); dict2 != NULL; dict2 = dict2->NextSiblingElement("dict"))
		{
			for(tinyxml2::XMLElement* dict3 = dict2->FirstChildElement("dict"); dict3 != NULL; dict3 = dict3->NextSiblingElement("dict"))
			{
				//Now we have to dig through a bajillion "key"s here to get the location...
				for(tinyxml2::XMLElement* key = dict3->FirstChildElement("key"); key != NULL; key = key->NextSiblingElement("key"))
				{
					const char* name = key->GetText();
					if(name != NULL && std::string(name) == "Location")	//Key with name "Location" should have the path next
					{
						//Found the right thing; next XML element should be "string" with the actual location
						//(Seriously, have they heard of XML attributes?)
						tinyxml2::XMLElement* string = key->NextSiblingElement("string");
						if(string != NULL)
						{
							const char* cPath = string->GetText();
							if(cPath != NULL)
							{
								ret.push_back(cPath);
								break;	//Stop searching through this lowest-level dict
							}
						}
					}
				}
			}
		}
	}
	
	delete doc;
	return ret;	//Worst. XML format. Ever.
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

void playlist_save_M3U(std::string sFilename, std::list<std::string> sFiles)
{
	std::ofstream pl(sFilename.c_str());
	if(pl.fail())
	{
		std::cout << "Error creating playlist file " << sFilename << std::endl;
		return;
	}
	pl << "#EXTM3U" << std::endl;	//Write header
	for(std::list<std::string>::iterator i = sFiles.begin(); i != sFiles.end(); i++)
  {
  	std::string sAlbum, sTitle, sArtist;
    uint iTrack;
    int iLength;
    song_get_tags(sFilename, sAlbum, sTitle, sArtist, iTrack, iLength);
    pl << "#EXTINF:" << iLength << "," << sTitle << " - " << sArtist << std::endl;	//Write metadata
  	pl << *i << std::endl;	//Write filename
  }
  pl.close();
}

void save_config()
{
	//TODO
		
		
		
    //std::list<std::string> playlist = get_cur_playlist();
    //playlist_save_kissme("last.kiss", playlist);
}

void load_config()
{
	//TODO		
		
		
	//	std::list<std::string> sFiles = playlist_load_kissme("last.kiss");
  //  for(std::list<std::string>::iterator i = sFiles.begin(); i != sFiles.end(); i++)
  //  	add_to_playlist(*i);
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

void playlist_play(std::string sName)
{
	clear_now_playing();
	std::list<std::string> sList = g_mPlaylists[sName];
	for(std::list<std::string>::iterator i = sList.begin(); i != sList.end(); i++)
  	add_to_playlist(*i);
  update_playlist_time();
}

void playlist_add(std::string sName, std::list<std::string> sSongs)
{
	g_mPlaylists[sName] = sSongs;
}

void save()
{
	save_config();
	save_cur_playlist();
}

void save_cur_playlist()
{
	std::list<std::string> playlist = get_cur_playlist();
	
	//Get current playlist name
	std::string sName = ttvfs::GetAppDir("kissme") + "/last";
	//Find selection
	GtkTreeIter iter;
	GtkTreeModel* tree_model = GTK_TREE_MODEL(gtk_builder_get_object(builder, "Playlists"));
	if(gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_builder_get_object(builder, "selectedplaylists")), &tree_model, &iter))
	{
		GValue value = G_VALUE_INIT;
    gtk_tree_model_get_value(tree_model, &iter, 0, &value);
    const gchar* text = g_value_get_string(&value);
    if(text != NULL)
    	sName = ttvfs::GetAppDir("kissme") + "/" + text;
    g_value_unset(&value);
  }
	sName += ".kiss";
  playlist_save_kissme(sName, playlist);
}















