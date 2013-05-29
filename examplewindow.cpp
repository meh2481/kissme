#include "examplewindow.h"
#include "sdlsound.h"
#include <iostream>
#include <vector>
using namespace std;

void play_song(string sFilename);

ExampleWindow::ExampleWindow()
: m_Button_File("Choose File"),
  m_Button_Folder("Choose Folder")
{
  set_title("Gtk::FileSelection example");

  add(m_ButtonBox);

  m_ButtonBox.pack_start(m_Button_File);
  m_Button_File.signal_clicked().connect(sigc::mem_fun(*this,
              &ExampleWindow::on_button_file_clicked) );

  show_all_children();
}

ExampleWindow::~ExampleWindow()
{
}


void ExampleWindow::on_button_file_clicked()
{
  Gtk::FileChooserDialog dialog("Please choose a file",
          Gtk::FILE_CHOOSER_ACTION_OPEN);
  dialog.set_transient_for(*this);
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
  }
}
