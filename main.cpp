#include <SDL/SDL.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <map>
#include <list>
#include <sstream>
//#include <gtkmm/main.h>
#include <gtk/gtk.h>
#include <VFS.h>
//#include "lame.h"
#include "signalhandler.h"
#include "sound.h"
using namespace std;

#define MAPFILE "map.txt"

map<string, string> g_mFileEncodeMap;
list<string> g_lPlayList;
//SDL_Surface* screen;
GtkBuilder *builder;

void on_button_folder_clicked()
{

}

/*void on_button_file_clicked()
{

}*/

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

int main(int argc, char **argv)
{
    GtkWidget  *window;
    GError     *error = NULL;
    ChData     *data;

    loadMap();
    init_sdl();
	atexit(saveMap);

    // Init GTK+
    gtk_init( &argc, &argv );

    //Create new GtkBuilder object
    builder = gtk_builder_new();
    // Load UI from file. If error occurs, report it and quit application.
    if( ! gtk_builder_add_from_file( builder, "kissme.glade", &error ) )
    {
        g_warning( "%s", error->message );
        g_free( error );
        return( 1 );
    }

    // Get main window pointer from UI
    window = GTK_WIDGET( gtk_builder_get_object( builder, "window1" ) );

    // Connect signals
    data = g_slice_new( ChData );

    // Get objects from UI
#define GW( name ) CH_GET_WIDGET( builder, name, data )
    GW( main_window );
    //GW( chart_area );
#undef GW

    // Connect signals
    gtk_builder_connect_signals( builder, data );

    // Destroy builder, since we don't need it anymore
    //g_object_unref( G_OBJECT( builder ) );

    // Show window. All other widgets are automatically shown by GtkBuilder
    gtk_widget_show( window );

    // Start main loop
    gtk_main();

//    Sound_Quit();
    Mix_CloseAudio();
    SDL_Quit();
    saveMap();

    return( 0 );
}



