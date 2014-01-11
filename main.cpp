#include <string>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <map>
#include <list>
#include <sstream>
#include <gtk/gtk.h>
#include <VFS.h>
#include "signalhandler.h"
#include "sound.h"
#include "fileoperations.h"
#include "playlist.h"

#define MAPFILE "map.txt"

GtkBuilder *builder;	//NOTE: VOLATILE

//HACK: GTK IS INCREDIBLY FINNICKY WITH THIS CALLBACK FOR SOME STRANGE REASON
extern int g_posx, g_posy;
void window_moved(GtkWindow *window, GdkEvent *event, gpointer data)
{
   gtk_window_get_position(window, &g_posx, &g_posy);	//Store this change of position
   //IF I DON'T DECLARE THIS, EVERYTHING BREAKS. GTK WHYYYYYYYYYY
   //SERIOUSLY. COMMENT THIS OUT AND WATCH IT DIE.
   char buf[10];
}

int main(int argc, char **argv)
{
    GtkWidget  *window;
    GError     *error = NULL;
    ChData     *data;

    init_sound();
    init_fileio();
    srand(time(NULL));

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
    
    gtk_widget_add_events(GTK_WIDGET(window), GDK_CONFIGURE);
  	g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(window_moved), NULL);

    // Every 10 ms, update tyrsound
    g_timeout_add(10, check_music_playing, data);
    // Every half-second, check window pos (because GTK doesn't let us do this after someone clicks the X button)
		//g_timeout_add(500, check_window_pos, data);
		// Every 5 minutes, autosave current playlist
		g_timeout_add(300000, save_cur_playlist, data);

    // Show window. All other widgets are automatically shown by GtkBuilder
    gtk_widget_show( window );

    //Initialize signal handler
    init_signal_handler();
    
    //Create user folder if it isn't there already
    const char* sUserDir = ttvfs::GetAppDir("kissme").c_str();
    if(!ttvfs::IsDirectory(sUserDir))
    	ttvfs::CreateDirRec(sUserDir);

    //Load our config
    load();
    
    //Parse our commandline
    playlist_add_commandline(argc, argv);

    // Start main loop
    gtk_main();

    //Save config and exit
    save_config();
    cleanup_sound();
    shutdown_fileio();

    return( 0 );
}



