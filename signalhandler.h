#ifndef GTKMM_EXAMPLEWINDOW_H
#define GTKMM_EXAMPLEWINDOW_H

#include <string>
#include <gtk/gtk.h>
//using namespace std;

#define CH_GET_OBJECT( builder, name, type, data ) \
    data->name = type( gtk_builder_get_object( builder, #name ) )
#define CH_GET_WIDGET( builder, name, data ) \
    CH_GET_OBJECT( builder, name, GTK_WIDGET, data )

#define REPEAT_NONE 0
#define REPEAT_ALL  1
#define REPEAT_ONE  2

//void load_song(std::string sFilename);
//void add_file(std::string sFilename);    //Add filename to widget

typedef struct _ChData ChData;
struct _ChData
{
    // Widgets
    GtkWidget *main_window;  // Main application window
    //GtkWidget *chart_area;   // Chart drawing area
};

extern "C"
{
    //Buttons
    G_MODULE_EXPORT void button_addfile_clicked(GtkButton *button, ChData *data);
    G_MODULE_EXPORT void button_previous_clicked(GtkButton *button, ChData *data);
    G_MODULE_EXPORT void button_play_clicked(GtkButton *button, ChData *data);

    G_MODULE_EXPORT void button_repeat_clicked(GtkButton *button, ChData *data);
    G_MODULE_EXPORT void button_shuffle_enter(GtkButton *button, ChData *data);
    G_MODULE_EXPORT void button_shuffle_leave(GtkButton *button, ChData *data);
    G_MODULE_EXPORT void button_shuffle_clicked(GtkButton *button, ChData *data);

    //Volume control
    G_MODULE_EXPORT void volume_changed(GtkScaleButton *button, gdouble value, ChData *data);

    //Playlist pane control
    G_MODULE_EXPORT void song_selected(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, ChData *data);

    //Called whenever a data field in the playlist is edited
    G_MODULE_EXPORT void title_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, ChData *data);
    G_MODULE_EXPORT void artist_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, ChData *data);
    G_MODULE_EXPORT void album_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, ChData *data);
    G_MODULE_EXPORT void playlistname_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, ChData *data);

    //Sorting function callbacks
    G_MODULE_EXPORT void drag_begins(GtkWidget *widget, GdkDragContext *drag_context, gpointer user_data);

    //For when the user drags on the slider for the current song
    G_MODULE_EXPORT void slider_move(GtkAdjustment *adjustment, gpointer user_data);
}

//Helper functions
void set_table_data(std::string sTreeViewName, std::string sListStoreName, gchar *path, gchar *new_text, gint column);
void add_song(std::string sFilename, std::string sTitle, std::string sArtist, std::string sAlbum, std::string sLength);
void show_play();   //Show play icon (for when music isn't playing)
void show_pause();  //Show pause icon (for when music is playing)
void init_signal_handler(); //Initialize variables used by the signal handler functions
void update_play_slider(float fPos, float fLen);  //Update where our current song is playing

#endif //GTKMM_EXAMPLEWINDOW_H
