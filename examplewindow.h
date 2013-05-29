#ifndef GTKMM_EXAMPLEWINDOW_H
#define GTKMM_EXAMPLEWINDOW_H

#include <gtkmm.h>

class ExampleWindow : public Gtk::Window
{
public:
  ExampleWindow();
  virtual ~ExampleWindow();

protected:
  //Signal handlers:
  void on_button_file_clicked();

  //Child widgets:
  Gtk::VButtonBox m_ButtonBox;
  Gtk::Button m_Button_File, m_Button_Folder;
};

#endif //GTKMM_EXAMPLEWINDOW_H
