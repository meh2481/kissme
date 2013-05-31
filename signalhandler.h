#ifndef GTKMM_EXAMPLEWINDOW_H
#define GTKMM_EXAMPLEWINDOW_H

#include <string>
#include <gtk/gtk.h>
using namespace std;

#define CH_GET_OBJECT( builder, name, type, data ) \
    data->name = type( gtk_builder_get_object( builder, #name ) )
#define CH_GET_WIDGET( builder, name, data ) \
    CH_GET_OBJECT( builder, name, GTK_WIDGET, data )

#define REPEAT_NONE 0
#define REPEAT_ALL  1
#define REPEAT_ONE  2

void load_song(string sFilename);
void add_file(string sFilename);    //Add filename to widget
void show_play();   //Show play icon (for when music isn't playing)
void show_pause();  //Show pause icon (for when music is playing)

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
}

//Helper functions
void set_table_data(string sTreeViewName, string sListStoreName, gchar *path, gchar *new_text, gint column);
void add_song(string sFilename, string sTitle, string sArtist, string sAlbum, string sLength);

#endif //GTKMM_EXAMPLEWINDOW_H
