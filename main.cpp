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

int main(int argc, char **argv)
{
    GtkWidget  *window;
    GError     *error = NULL;
    ChData     *data;

    //loadMap();
    init_sdl();
	//atexit(saveMap);

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

    gtk_builder_connect_signals( builder, data );

    // Set reasonable defaults //TODO: Read last settings from file
    //gtk_scale_button_set_value(GTK_WIDGET(gtk_builder_get_object(builder, "volumebutton1")), 1.0);

    // Show window. All other widgets are automatically shown by GtkBuilder
    gtk_widget_show( window );

    // Start main loop
    gtk_main();

//    Sound_Quit();
    Mix_CloseAudio();
    SDL_Quit();
    //saveMap();

    return( 0 );
}



