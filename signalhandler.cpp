#include "signalhandler.h"
#include "sound.h"
#include <iostream>
#include <vector>
using namespace std;

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
    gtk_file_chooser_add_filter(filechooser, filter);

    GtkFileFilter* filter2 = gtk_file_filter_new();
    gtk_file_filter_set_name(filter2, "All Files");
    gtk_file_filter_add_pattern(filter2, "*");
    gtk_file_chooser_add_filter(filechooser, filter2);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        filename = gtk_file_chooser_get_filename(filechooser);
        play_song(filename);
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

      play_song(dialog.get_filename());
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
    Mix_RewindMusic();  //TODO Song list
}

extern GtkBuilder *builder;
bool bPaused = true;
G_MODULE_EXPORT void button_play_clicked(GtkButton *button, ChData *data)
{
    if(bPaused)
    {
        gtk_button_set_image(button, GTK_WIDGET( gtk_builder_get_object( builder, "ImgPause" ) ));
        //SDL_PauseAudio(0);
        bPaused = !bPaused;
        Mix_ResumeMusic();
    }
    else
    {
        gtk_button_set_image(button, GTK_WIDGET( gtk_builder_get_object( builder, "ImgPlay" ) ));
        //SDL_PauseAudio(1);
        bPaused = !bPaused;
        Mix_PauseMusic();
    }
}




