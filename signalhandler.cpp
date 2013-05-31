#include "signalhandler.h"
#include "sound.h"
#include <iostream>
#include <vector>
using namespace std;

extern GtkBuilder *builder;
bool bPaused = true;
int iRepeatMode = REPEAT_NONE;

G_MODULE_EXPORT void button_addfile_clicked(GtkButton *button, ChData *data)
{
    //FIXME: Is this a deprecated way of doing things? Can I use Gtk::FileChooserDialog instead?
    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new ("Add Files",
                                          GTK_WINDOW(data->main_window),
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                          NULL);

    GtkFileChooser *filechooser = GTK_FILE_CHOOSER(dialog);

    gtk_file_chooser_set_select_multiple(filechooser, true);

    //Add filters to our file chooser
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Song Files");
    gtk_file_filter_add_pattern(filter, "*.mp3");
    gtk_file_filter_add_pattern(filter, "*.ogg");
    gtk_file_filter_add_pattern(filter, "*.wma");
    gtk_file_filter_add_pattern(filter, "*.wav");
    gtk_file_filter_add_pattern(filter, "*.m4a");
    gtk_file_filter_add_pattern(filter, "*.flac");
    gtk_file_chooser_add_filter(filechooser, filter);

    GtkFileFilter* filter2 = gtk_file_filter_new();
    gtk_file_filter_set_name(filter2, "All Files");
    gtk_file_filter_add_pattern(filter2, "*");
    gtk_file_chooser_add_filter(filechooser, filter2);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        filename = gtk_file_chooser_get_filename(filechooser);
        load_song(filename);
        //play_song(); //Start song playing
        g_free(filename);
        //TODO: Get all filenames
        /*GSList* filenames = gtk_file_chooser_get_filenames(filechooser);
        for(GSList* i = filenames; i != NULL; i=i->next)
        {
            string sFilename = (char*)i->data;
            //Do something with sFilename
            g_free(i->data);
        }
        g_slist_free(filenames);*/
    }

    gtk_widget_destroy (dialog);
  /*Gtk::FileChooserDialog dialog("Please choose a file",
          Gtk::FILE_CHOOSER_ACTION_OPEN);
  //dialog.set_transient_for(*GTK_WINDOW(gtk_widget_get_toplevel(data->main_window)));
  dialog.set_select_multiple(true);

  //Add response buttons the the dialog:
  dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

  //Add filters, so that only certain file types can be selected:

  Glib::RefPtr<Gtk::FileFilter> filter_songs = Gtk::FileFilter::create();
  filter_songs->set_name("Song files");
  filter_songs->add_pattern("*.mp3");
  filter_songs->add_pattern("*.wav");
  filter_songs->add_pattern("*.ogg");
  filter_songs->add_pattern("*.m4a");
  filter_songs->add_pattern("*.wma");
  dialog.add_filter(filter_songs);

  Glib::RefPtr<Gtk::FileFilter> filter_any = Gtk::FileFilter::create();
  filter_any->set_name("All files");
  filter_any->add_pattern("*");
  dialog.add_filter(filter_any);

  //Show the dialog and wait for a user response:
  int result = dialog.run();

  //Handle the response:
  switch(result)
  {
    case(Gtk::RESPONSE_OK):
    {
      //std::cout << "Open clicked." << std::endl;

      //Notice that this is a std::string, not a Glib::ustring.
      vector<string> filenames = dialog.get_filenames();
      if(filenames.size())
      //for(vector<string>::iterator i = filenames.begin(); i != filenames.end(); i++)
        //std::cout << "File selected: " << *i << std::endl;

      load_song(dialog.get_filename());
      break;
    }
    case(Gtk::RESPONSE_CANCEL):
    {
      //std::cout << "Cancel clicked." << std::endl;
      break;
    }
    default:
    {
      //std::cout << "Unexpected button clicked." << std::endl;
      break;
    }
  }*/
}

G_MODULE_EXPORT void button_previous_clicked(GtkButton *button, ChData *data)
{
    rewind_song(); //TODO Song list
}

G_MODULE_EXPORT void button_play_clicked(GtkButton *button, ChData *data)
{
    if(bPaused)
    {
        bPaused = !bPaused;
        play_song();
    }
    else
    {
        bPaused = !bPaused;
        pause_song();
    }
}

G_MODULE_EXPORT void button_repeat_clicked(GtkButton *button, ChData *data)
{
    iRepeatMode++;
    if(iRepeatMode > REPEAT_ONE)
        iRepeatMode = REPEAT_NONE;
    switch(iRepeatMode)
    {
        case REPEAT_ALL:
            gtk_button_set_image(button, GTK_WIDGET(gtk_builder_get_object(builder, "ImgRepeatAll")));
            break;
        case REPEAT_NONE:
            gtk_button_set_image(button, GTK_WIDGET(gtk_builder_get_object(builder, "ImgRepeatOff")));
            break;
        case REPEAT_ONE:
            gtk_button_set_image(button, GTK_WIDGET(gtk_builder_get_object(builder, "ImgRepeatOne")));
            break;
    }
}

G_MODULE_EXPORT void button_shuffle_enter(GtkButton *button, ChData *data)
{
    gtk_button_set_image(button, GTK_WIDGET(gtk_builder_get_object(builder, "ImgShuffle")));
}

G_MODULE_EXPORT void button_shuffle_leave(GtkButton *button, ChData *data)
{
    gtk_button_set_image(button, GTK_WIDGET(gtk_builder_get_object(builder, "ImgShuffleOff")));
}

G_MODULE_EXPORT void button_shuffle_clicked(GtkButton *button, ChData *data)
{
    //TODO Shuffle list
}

G_MODULE_EXPORT void volume_changed(GtkScaleButton *button, gdouble value, ChData *data)
{
    setVolume(value);
}

G_MODULE_EXPORT void song_selected(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, ChData *data)
{
    //cout << "path: " << gtk_tree_path_to_string(path) << endl;
    GtkTreeModel *model;
    GtkTreeIter   iter;

    model = gtk_tree_view_get_model(tree_view);

    if (gtk_tree_model_get_iter(model, &iter, path))
    {
        gchar *name;

        gtk_tree_model_get(model, &iter, 0, &name, -1);

        //cout << "Name: " << name << endl;
        load_song(name);
        play_song();

        g_free(name);
    }
}

void set_table_data(string sTreeViewName, string sListStoreName, gchar *path, gchar *new_text, gint column)
{
    //All this just to set the table value? OH COME ON!
    GtkTreeModel* mod = gtk_tree_view_get_model(GTK_TREE_VIEW(gtk_builder_get_object(builder, sTreeViewName.c_str())));
    GtkTreeIter i;
    GtkTreePath *p = gtk_tree_path_new_from_string(path);
    gtk_tree_model_get_iter(mod, &i, p);
    GValue a = G_VALUE_INIT;
    g_value_init (&a, G_TYPE_STRING);
    g_value_set_static_string (&a, new_text);
    gtk_list_store_set_value(GTK_LIST_STORE(gtk_builder_get_object(builder, sListStoreName.c_str())), &i, column, &a);
}

G_MODULE_EXPORT void title_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, ChData *data)
{
    set_table_data("treeview2", "Tracks", path, new_text, 1);
    //TODO: Edit tags or some kinda thing
}

G_MODULE_EXPORT void artist_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, ChData *data)
{
    set_table_data("treeview2", "Tracks", path, new_text, 2);
}

G_MODULE_EXPORT void album_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, ChData *data)
{
    set_table_data("treeview2", "Tracks", path, new_text, 3);
}

G_MODULE_EXPORT void playlistname_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, ChData *data)
{
    set_table_data("treeview1", "Playlists", path, new_text, 0);
    //TODO Save under some new name or such
}


void add_file(string sFilename)
{

}

void show_play()
{
    gtk_button_set_image(GTK_BUTTON(gtk_builder_get_object(builder, "Play")), GTK_WIDGET(gtk_builder_get_object(builder, "ImgPlay")));
    bPaused = true;
}

void show_pause()
{
    gtk_button_set_image(GTK_BUTTON(gtk_builder_get_object(builder, "Play")), GTK_WIDGET(gtk_builder_get_object(builder, "ImgPause")));
    bPaused = false;
}














