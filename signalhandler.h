#ifndef GTKMM_EXAMPLEWINDOW_H
#define GTKMM_EXAMPLEWINDOW_H

#include <string>
#include <gtk/gtk.h>
//#include <gtkmm/main.h>

//#include <gtkmm.h>
using namespace std;

#define CH_GET_OBJECT( builder, name, type, data ) \
    data->name = type( gtk_builder_get_object( builder, #name ) )
#define CH_GET_WIDGET( builder, name, data ) \
    CH_GET_OBJECT( builder, name, GTK_WIDGET, data )

void play_song(string sFilename);

typedef struct _ChData ChData;
struct _ChData
{
    // Widgets
    GtkWidget *main_window;  // Main application window
    //GtkWidget *chart_area;   // Chart drawing area
};

extern "C"
{
    G_MODULE_EXPORT void button_addfile_clicked(GtkButton *button, ChData *data);
    G_MODULE_EXPORT void button_previous_clicked(GtkButton *button, ChData *data);
    G_MODULE_EXPORT void button_play_clicked(GtkButton *button, ChData *data);

    G_MODULE_EXPORT void volume_changed(GtkScaleButton *button, gdouble value, ChData *data);
}

#endif //GTKMM_EXAMPLEWINDOW_H
