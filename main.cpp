#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <map>
#include <list>
#include <sstream>
using namespace std;

#define MAPFILE "map.txt"


map<string, string> g_mFileEncodeMap;
list<string> g_lPlayList;

void convert(string sFilename)
{
	if(g_mFileEncodeMap.count(sFilename))
		return;
	string s = "./bin/lame --decode " + sFilename + " - 2>lame.txt | oggenc -o " + sFilename + ".ogg - 2>ogg.txt";
	cout << s << endl;
	system(s.c_str());
	g_mFileEncodeMap[sFilename] = sFilename + ".ogg";
}

void loadMap()
{
	ifstream infile(MAPFILE);
	string s;
	while(!infile.eof() && !infile.fail())
	{
		getline(infile, s);
		istringstream iss(s);
		string sFile, sVal;
		getline(iss, sFile, ':');
		getline(iss, sVal);
		g_mFileEncodeMap[sFile] = sVal;
	}
	infile.close();
}

void saveMap()
{
	ofstream ofile(MAPFILE);
	for(map<string, string>::iterator i = g_mFileEncodeMap.begin(); i != g_mFileEncodeMap.end(); i++)
	{
		ofile << i->first << ":" << i->second << endl;
	}
	ofile.close();
}

void usage()
{
	cout << "Usage: kissme [filenames]" << endl;
}

int main(int argc, char** argv)
{
	loadMap();
	
	// start SDL with audio support
	if(SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO)==-1) 
	{
		cout << "SDL_Init: " << SDL_GetError() << endl;
		exit(1);
	}

	// open 44.1KHz, signed 16bit, system byte order,
	//      stereo audio, using 1024 byte chunks
	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096)==-1) 
	{
		cout << "Mix_OpenAudio: " << Mix_GetError() << endl;
		exit(1);
	}

	SDL_Surface* screen = SDL_SetVideoMode(800, 600, 0, 0);

	atexit(SDL_Quit);
	atexit(Mix_CloseAudio);
	atexit(saveMap);

	// load the MP3 file "music.mp3" to play as music
	Mix_Music *music;
	if(argc > 1)
	{
		for(int i = 1; i < argc; i++)
		{
			string sFileToPlay = argv[i];
			g_lPlayList.push_back(sFileToPlay);
		}
	}
	else
	{
		usage();
		exit(0);
	}
	
	SDL_Event event;
	
	while(g_lPlayList.size())
	{
		string filename = g_lPlayList.front();
		g_lPlayList.pop_front();
		music=Mix_LoadMUS(filename.c_str());
		if(!music) 
		{
			cout << "Mix_LoadMUS: " << Mix_GetError() << endl;
			exit(1);
		}
		else
		{
			if(Mix_PlayMusic(music, 0)==-1) 
			{
		  	cout << "Mix_PlayMusic: " << Mix_GetError() << endl;
		  	exit(1);
			}
			if(!Mix_PlayingMusic())	//MP3 not in right format
			{
				convert(filename);
			
				Mix_FreeMusic(music);
				music=Mix_LoadMUS((g_mFileEncodeMap[filename]).c_str());
				cout << "Playing " << g_mFileEncodeMap[filename] << endl;
				Mix_PlayMusic(music, 0);
			}
		}
		
		//Wait for song to be over
		while(true) 
		{
		  while(SDL_PollEvent(&event)) 
		  {
		    switch(event.type) 
		    {
		    	case SDL_QUIT:
						return 0;
		    }
		  }

		  // So we don't hog the CPU 
		  SDL_Delay(500);
		  
		  if(!Mix_PlayingMusic())
		  {
		  	//Free music
				Mix_FreeMusic(music);
		  	break;	//On to next song
		  }
		}

  }

	return 0;
}




