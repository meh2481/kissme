#include <SDL/SDL.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <map>
#include <list>
#include <sstream>
#include <gtkmm/main.h>
#include <VFS.h>
//#include "lame.h"
#include "examplewindow.h"
#include "sdlsound.h"
using namespace std;

#define MAPFILE "map.txt"

map<string, string> g_mFileEncodeMap;
list<string> g_lPlayList;
SDL_Surface* screen;

void on_button_folder_clicked()
{

}

void repaint(int32_t w, int32_t h)
{
	SDL_Rect rc;
	rc.x = 5;
	rc.y = 5;
	rc.w = w-10;
	rc.h = h-10;
	SDL_FillRect(screen, &rc, SDL_MapRGB(screen->format, 128, 0, 0));

	SDL_Flip(screen);

	on_button_folder_clicked();
}

void resizeWindow(int32_t w, int32_t h)
{
	screen = SDL_SetVideoMode(w, h, 0, SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_RESIZABLE);
	repaint(w,h);
}

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
	if(!g_mFileEncodeMap.size()) return;
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



int main(int argc, char *argv[])
{
    loadMap();
    init_sdl();
	atexit(saveMap);
    Gtk::Main kit(argc, argv);

    ExampleWindow window;
    //Shows the window and returns when it is closed.
    Gtk::Main::run(window);

    Sound_Quit();
    //Mix_CloseAudio();
    SDL_Quit();
    saveMap();
    return 0;
}





