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
static std::map<std::string, std::list<song> > g_mPlaylists;

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

std::list<song> playlist_load(std::string sFilename)
{
	std::list<song> ret;
	if(sFilename.find(".m3u", sFilename.size() - 4) != std::string::npos ||
		 sFilename.find(".m3u8", sFilename.size() - 5) != std::string::npos)
		ret = fill_out(playlist_load_M3U(sFilename), ttvfs::StripLastPath(sFilename));
	else if(sFilename.find(".asx", sFilename.size() - 4) != std::string::npos)
		ret = fill_out(playlist_load_ASX(sFilename), ttvfs::StripLastPath(sFilename));
	else if(sFilename.find(".pls", sFilename.size() - 4) != std::string::npos)
		ret = fill_out(playlist_load_PLS(sFilename), ttvfs::StripLastPath(sFilename));
	else if(sFilename.find(".wpl", sFilename.size() - 4) != std::string::npos)
		ret = fill_out(playlist_load_WPL(sFilename), ttvfs::StripLastPath(sFilename));
	else if(sFilename.find(".xspf", sFilename.size() - 5) != std::string::npos)
		ret = fill_out(playlist_load_XSPF(sFilename), ttvfs::StripLastPath(sFilename));
	else if(sFilename.find(".xml", sFilename.size() - 4) != std::string::npos)
		ret = fill_out(playlist_load_iTunes(sFilename), ttvfs::StripLastPath(sFilename));
	else if(sFilename.find(".kiss", sFilename.size() - 5) != std::string::npos)
		ret = convert_to_global(playlist_load_kissme(sFilename), ttvfs::StripLastPath(sFilename));
	else
		std::cout << "Playlist file format " << sFilename.substr(sFilename.size() - 3) << " unsupported." << std::endl;
		
	return ret;
}

std::list<song> playlist_load_M3U(std::string sFilename)
{
	std::list<song> ret;
	
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
      {
      	song so;
      	so.filename = s;
      	ret.push_back(so);
      }
  }
  infile.close();
	
	return ret;
}

std::list<song> playlist_load_ASX(std::string sFilename)
{
	std::list<song> ret;
	
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
  			song so;
      	so.filename = s;
      	ret.push_back(so);
			}
		}
  }
	
	delete doc;
	return ret;
}

std::list<song> playlist_load_PLS(std::string sFilename)
{
	std::list<song> ret;
	
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
      song so;
    	so.filename = s;
    	ret.push_back(so);
    }
    
  }
  playlistFile.close();
	
	return ret;
}

std::list<song> playlist_load_WPL(std::string sFilename)
{
	std::list<song> ret;
	
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
				{
					song so;
		    	so.filename = src;
		    	ret.push_back(so);
				}
			}
  	}
  }
	delete doc;
	return ret;
}

std::list<song> playlist_load_XSPF(std::string sFilename)
{
	std::list<song> ret;
	
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
				{
					song so;
		    	so.filename = src;
		    	ret.push_back(so);
				}
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
std::list<song> playlist_load_iTunes(std::string sFilename)
{
	std::list<song> ret;
	
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
								song so;
								so.filename = cPath;
								ret.push_back(so);
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

std::list<song> playlist_load_kissme(std::string sFilename)
{
	std::list<song> ret;
	/*std::ifstream playlistFile(sFilename.c_str());
  while(!playlistFile.fail() && !playlistFile.eof())
  {
      std::string s;
      getline(playlistFile, s);
      if(s.size())
      {
          ret.push_back(song_get_tags(s));
      }
  }
  playlistFile.close();*/
  
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
  
	for(tinyxml2::XMLElement* songelem = root->FirstChildElement("song"); songelem != NULL; songelem = songelem->NextSiblingElement("song"))
	{
		song s;
		s.track = 0;
		s.length = -1;
		const char* cFilename = songelem->Attribute("file");
		const char* cTitle = songelem->Attribute("title");
		const char* cArtist = songelem->Attribute("artist");
		const char* cAlbum = songelem->Attribute("album");
		if(cFilename == NULL) continue;
		s.filename = cFilename;
		if(cTitle != NULL)
			s.title = cTitle;
		else
			s.title = ttvfs::StripFileExtension(ttvfs::PathToFileName(cFilename));	//At bare minimum, display filename as title
		if(cArtist != NULL)
			s.artist = cArtist;
		if(cAlbum != NULL)
			s.album = cAlbum;
			
		songelem->QueryUnsignedAttribute("track", &s.track);
		songelem->QueryIntAttribute("length", &s.length);
		
		ret.push_back(s);
	}  
  
	return ret;
}

void playlist_save_kissme(std::string sFilename, std::list<song> sFiles)
{
	/*std::ofstream playlistFile(sFilename.c_str());
  if(playlistFile.fail()) return;
  for(std::list<song>::iterator i = sFiles.begin(); i != sFiles.end(); i++)
      playlistFile << i->filename << std::endl;
  playlistFile.close();*/
  
  //Create XML
  tinyxml2::XMLDocument* doc = new tinyxml2::XMLDocument;
  tinyxml2::XMLElement* root = doc->NewElement("playlist");
  doc->InsertFirstChild(root);
  
  //Save songs
  for(std::list<song>::iterator i = sFiles.begin(); i != sFiles.end(); i++)
  {
  	tinyxml2::XMLElement* songelem = doc->NewElement("song");
  	root->InsertEndChild(songelem);
  	
  	//Populate song with important metadata
  	songelem->SetAttribute("file", i->filename.c_str());
  	songelem->SetAttribute("title", i->title.c_str());
  	songelem->SetAttribute("artist", i->artist.c_str());
  	songelem->SetAttribute("album", i->album.c_str());
  	songelem->SetAttribute("track", i->track);
  	songelem->SetAttribute("length", i->length);
	}
	
	//Done
	doc->SaveFile(sFilename.c_str());
  delete doc;
}

void playlist_save_M3U(std::string sFilename, std::list<song> sFiles)
{
	std::ofstream pl(sFilename.c_str());
	if(pl.fail())
	{
		std::cout << "Error creating playlist file " << sFilename << std::endl;
		return;
	}
	pl << "#EXTM3U" << std::endl;	//Write header
	for(std::list<song>::iterator i = sFiles.begin(); i != sFiles.end(); i++)
  {
  	std::string sAlbum, sTitle, sArtist;
    uint iTrack;
    int iLength;
    song_get_tags(sFilename, sAlbum, sTitle, sArtist, iTrack, iLength);
    pl << "#EXTINF:" << iLength << "," << sTitle << " - " << sArtist << std::endl;	//Write metadata
  	pl << i->filename << std::endl;	//Write filename
  }
  pl.close();
}

void save_config()
{
	std::string sConfigFilename = ttvfs::GetAppDir("kissme") + "/kissme.last";
	
	gint root_x, root_y;
	gint width, height;
	bool maximized = get_window_maximized();
	get_window_position(&root_x, &root_y);
	get_window_size(&width, &height);
	
	//Create XML document
	tinyxml2::XMLDocument* doc = new tinyxml2::XMLDocument;
  tinyxml2::XMLElement* root = doc->NewElement("config");
  doc->InsertFirstChild(root);
  
  //Save window data
  tinyxml2::XMLElement* window = doc->NewElement("window");
  root->InsertEndChild(window);
  window->SetAttribute("maximized", maximized);
  window->SetAttribute("posx", root_x);
  window->SetAttribute("posy", root_y);
  window->SetAttribute("width", width);
  window->SetAttribute("height", height);
  
  //Save audio data stuff
  tinyxml2::XMLElement* audio = doc->NewElement("audio");
  root->InsertEndChild(audio);
  audio->SetAttribute("volume", getVolume());
	
	//Done
	doc->SaveFile(sConfigFilename.c_str());
  delete doc;
}

void load_config()	//Silently fail if config isn't here already
{
	std::string sConfigFilename = ttvfs::GetAppDir("kissme") + "/kissme.last";
	
	//Open config file
	tinyxml2::XMLDocument* doc = new tinyxml2::XMLDocument;
  int iErr = doc->LoadFile(sConfigFilename.c_str());
  if(iErr != tinyxml2::XML_NO_ERROR)
  {
		delete doc;
		return;
  }
  
  //Grab root element
  tinyxml2::XMLElement* root = doc->RootElement();
  if(root == NULL)
  {
		delete doc;
		return;
  }
  
  //Load window
  GtkWindow* w = GTK_WINDOW(gtk_builder_get_object(builder, "window1"));
  tinyxml2::XMLElement* window = root->FirstChildElement("window");
  if(window != NULL)
  {
  	int width, height, posx, posy;
  	bool maximized = false;
  	
  	window->QueryBoolAttribute("maximized", &maximized);
  	
  	if(maximized)
  		gtk_window_maximize(w);
  	else
  	{
			if(window->QueryIntAttribute("posx", &posx) == tinyxml2::XML_NO_ERROR &&
				 window->QueryIntAttribute("posy", &posy) == tinyxml2::XML_NO_ERROR)
				gtk_window_move(w, posx, posy);
				
			if(window->QueryIntAttribute("width", &width) == tinyxml2::XML_NO_ERROR &&
				 window->QueryIntAttribute("height", &height) == tinyxml2::XML_NO_ERROR)
				gtk_window_resize(w, width, height);
  	}
	}
	
	//Load audio config
	tinyxml2::XMLElement* audio = root->FirstChildElement("audio");
	if(audio != NULL)
	{
		//volume
		float fVolume = 1.0;
		audio->QueryFloatAttribute("volume", &fVolume);
		setVolume(fVolume);
		//Update GUI to reflect this
		gtk_scale_button_set_value(GTK_SCALE_BUTTON(gtk_builder_get_object(builder, "volumebutton1")), fVolume);
	}
	
	delete doc;
}

//Load all playlists in user dir
void load_playlists()
{
	ttvfs::StringList lFiles;
	ttvfs::GetFileList(ttvfs::GetAppDir("kissme").c_str(), lFiles);
	
	//Loop through all files in user dir (where we keep playlists)
	for(ttvfs::StringList::iterator i = lFiles.begin(); i != lFiles.end(); i++)
	{
		if(i->find(".kiss", i->size() - 5) != std::string::npos)	//Playlist file
		{
			std::list<song> sFiles = playlist_load_kissme(ttvfs::GetAppDir("kissme") + "/" + *i);
			std::string sListName = ttvfs::StripFileExtension(*i);
			//Add playlist to our manager
      playlist_add(sListName, sFiles);
      
      //Add new playlist to our view
      GtkTreeIter iter;
      GtkListStore* playlists = GTK_LIST_STORE(gtk_builder_get_object(builder, "Playlists"));
      gtk_list_store_append(playlists, &iter);
      GValue a = G_VALUE_INIT;
      g_value_init (&a, G_TYPE_STRING);
      g_value_set_static_string (&a, sListName.c_str());
      gtk_list_store_set_value(playlists, &iter, 0, &a);
		}
	}
}

std::list<song> convert_to_global(std::list<song> sFilenames, std::string sPath)
{
	//std::list<song> ret;
	for(std::list<song>::iterator i = sFilenames.begin(); i != sFilenames.end(); i++)
	{
		std::string s = convert_to_path(i->filename);
		if(!ttvfs::IsDirectory(ttvfs::StripLastPath(s).c_str()))	//Relative path
			i->filename.assign(ttvfs::FixPath(sPath + '/' + s));
		else																							//Absolute path
			i->filename.assign(ttvfs::FixPath(s));
	}
	return sFilenames;
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
	std::list<song> sList = g_mPlaylists[sName];
	for(std::list<song>::iterator i = sList.begin(); i != sList.end(); i++)
  	add_to_playlist(*i);
  update_playlist_time();
}

void playlist_add(std::string sName, std::list<song> sSongs)
{
	g_mPlaylists[sName] = sSongs;
}

void save()
{
	save_config();
	save_cur_playlist(NULL);
}

void load()
{
	load_playlists();
	load_config();
}

gboolean save_cur_playlist(gpointer data)
{
	std::list<song> playlist = get_cur_playlist();
	
	//Get current playlist filename
	std::string sName = ttvfs::GetAppDir("kissme") + "/last";
	std::string sPaneName = "last";
	//Find selection
	GtkTreeIter iter;
	GtkTreeModel* tree_model = GTK_TREE_MODEL(gtk_builder_get_object(builder, "Playlists"));
	if(gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_builder_get_object(builder, "selectedplaylists")), &tree_model, &iter))
	{
		GValue value = G_VALUE_INIT;
    gtk_tree_model_get_value(tree_model, &iter, 0, &value);
    const gchar* text = g_value_get_string(&value);
    if(text != NULL)
    {
    	sName = ttvfs::GetAppDir("kissme") + "/" + text;
    	sPaneName = text;
    }
    g_value_unset(&value);
  }
	sName += ".kiss";
	
	if(sPaneName == "last" && !playlist.size()) return true;	//Skip empty "last.kiss" files
	
	//Update our internal list
	g_mPlaylists[sPaneName] = playlist;
	
	//Save the playlist
  playlist_save_kissme(sName, playlist);
  
  return true;
}

void save_cur_playlist(std::string sName)
{
	if(!sName.size()) return;
	
	std::list<song> playlist = get_cur_playlist();
	
	//Get current playlist filename
	std::string sFileName = ttvfs::GetAppDir("kissme") + "/" + sName + ".kiss";
	
	//Update our internal list
	g_mPlaylists[sName] = playlist;
	
	//Save the playlist
  playlist_save_kissme(sFileName, playlist);
}

std::list<song> fill_out(std::list<song> songs, std::string sPath)
{
	songs = convert_to_global(songs, sPath);
	for(std::list<song>::iterator i = songs.begin(); i != songs.end(); i++)
	{
		*i = song_get_tags(i->filename);
	}
	return songs;
}











