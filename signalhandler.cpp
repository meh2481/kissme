#include "signalhandler.h"
#include "sound.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <cmath>
#include <iomanip>

extern GtkBuilder *builder;
bool bPaused = true;
int iRepeatMode = REPEAT_NONE;
float g_fTotalPlaylistLength = 0.0;
//GtkTreeRowReference* curPlay = NULL;

G_MODULE_EXPORT void button_addfile_clicked(GtkButton *button, ChData *data)
{
    //FIXME: Should I be using GTK+ instead of flat GTK?
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
    gtk_file_filter_add_pattern(filter, "*.spc");
    gtk_file_filter_add_pattern(filter, "*.vgm");
    gtk_file_filter_add_pattern(filter, "*.nsf");
    gtk_file_filter_add_pattern(filter, "*.opus");

    gtk_file_chooser_add_filter(filechooser, filter);

    GtkFileFilter* filter2 = gtk_file_filter_new();
    gtk_file_filter_set_name(filter2, "All Files");
    gtk_file_filter_add_pattern(filter2, "*");
    gtk_file_chooser_add_filter(filechooser, filter2);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        //Get all filenames and add them
        GSList* filenames = gtk_file_chooser_get_filenames(filechooser);
        for(GSList* i = filenames; i != NULL; i=i->next)
        {
            std::string s = (char*)i->data;
            std::cout << s << std::endl;
            add_to_playlist(s);
            g_free(i->data);
        }
        g_slist_free(filenames);

        //Save list in case of crash
        save_playlist();
    }

    gtk_widget_destroy (dialog);
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
    //Disable sorting
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(gtk_builder_get_object(builder, "Tracks")), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

    //Shuffle list
    int iNum = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gtk_builder_get_object(builder, "Tracks")), NULL); //Get number of items in list
    gint new_order[iNum+1];
    //Grab random numbers until we get one that hasn't been used yet. Not the best way to do it, but it works
    for(int i = 0; i < iNum; i++)
    {
        bool bSame = false;
        do
        {
            bSame = false;
            new_order[i] = rand() % iNum;
            for(int j = i-1; j >= 0; j--)
            {
                if(new_order[i] == new_order[j])
                {
                    bSame = true;
                    break;
                }
            }
        }
        while(bSame);
    }
    new_order[iNum] = 0;

    gtk_list_store_reorder(GTK_LIST_STORE(gtk_builder_get_object(builder, "Tracks")), new_order);
}

G_MODULE_EXPORT void volume_changed(GtkScaleButton *button, gdouble value, ChData *data)
{
    setVolume(value);
}

gboolean clear_play_icons(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    set_table_data("treeview2", "Tracks", path, "", 5);
    return false;
}

G_MODULE_EXPORT void song_selected(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, ChData *data)
{
    GtkTreeModel *model;
    GtkTreeIter   iter;

    model = gtk_tree_view_get_model(tree_view);

    if (gtk_tree_model_get_iter(model, &iter, path))
    {
        gchar *name;

        gtk_tree_model_get(model, &iter, 0, &name, -1);

        load_song(name);
        //play_song();

        g_free(name);

        //Get track name to display
        gtk_tree_model_get(model, &iter, 1, &name, -1);
        std::string sTitle = name;
        g_free(name);
        gtk_tree_model_get(model, &iter, 2, &name, -1); //And artist
        std::ostringstream oss;
        oss << "<b>" << sTitle << "</b>\n  <i>" << name << "</i>";
        //Draw artist name and track name
        gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(builder, "curtrack")), oss.str().c_str());
        g_free(name);

        //Also set window title to show we're playing this song
        if(sTitle.length())
            gtk_window_set_title(GTK_WINDOW(gtk_builder_get_object(builder, "window1")), ("kissme - " + sTitle).c_str());
        else
            gtk_window_set_title(GTK_WINDOW(gtk_builder_get_object(builder, "window1")), "kissme");

        //And show play icon
        /*if(curPlay != NULL) //Hide play icon on previous song
        {
            GtkTreePath* oldp = gtk_tree_row_reference_get_path(curPlay);
            if(oldp != NULL)
            {
                set_table_data("treeview2", "Tracks", oldp, "", 5);
                gtk_tree_path_free(oldp);
            }
        }*/
        gtk_tree_model_foreach(model, clear_play_icons, data);  //Clear previous play icons
        set_table_data("treeview2", "Tracks", path, PLAY_ICON, 5);
        //curPlay = gtk_tree_row_reference_new(model, path);
    }
}

void set_table_data(std::string sTreeViewName, std::string sListStoreName, GtkTreePath *path, gchar *new_text, gint column)
{
    //All this just to set the table value? OH COME ON!
    GtkTreeModel* mod = gtk_tree_view_get_model(GTK_TREE_VIEW(gtk_builder_get_object(builder, sTreeViewName.c_str())));
    GtkTreeIter i;
    gtk_tree_model_get_iter(mod, &i, path);
    GValue a = G_VALUE_INIT;
    g_value_init (&a, G_TYPE_STRING);
    g_value_set_static_string (&a, new_text);
    gtk_list_store_set_value(GTK_LIST_STORE(gtk_builder_get_object(builder, sListStoreName.c_str())), &i, column, &a);
}

G_MODULE_EXPORT void title_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, ChData *data)
{
    set_table_data("treeview2", "Tracks", gtk_tree_path_new_from_string(path), new_text, 1);
    //TODO: Edit tags or some kinda thing
}

G_MODULE_EXPORT void artist_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, ChData *data)
{
    set_table_data("treeview2", "Tracks", gtk_tree_path_new_from_string(path), new_text, 2);
}

G_MODULE_EXPORT void album_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, ChData *data)
{
    set_table_data("treeview2", "Tracks", gtk_tree_path_new_from_string(path), new_text, 3);
}

G_MODULE_EXPORT void playlistname_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, ChData *data)
{
    set_table_data("treeview1", "Playlists", gtk_tree_path_new_from_string(path), new_text, 0);
    //TODO Save under some new name or such
}


void add_file(std::string sFilename)
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

void init_signal_handler()
{

}
bool bSlider = false;
void update_play_slider(float fPos, float fLen)
{
    bSlider = true;
    gtk_adjustment_set_value(GTK_ADJUSTMENT(gtk_builder_get_object(builder, "posadjustment")), fPos/fLen*100.0);
    gtk_range_set_fill_level(GTK_RANGE(gtk_builder_get_object(builder, "playpos")), fPos/fLen*100.0);

    //Also update our label underneath the slider
    std::ostringstream oss;
    oss.fill('0');
    oss << (int)floorf(fPos/60.0) << ":" << std::setw(2) << (int)floorf(fPos) % 60 << " of "
        << std::setw(1) << (int)floorf(fLen/60.0) << ":" << std::setw(2) << (int)floorf(fLen) % 60;
    gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder, "playposlabel")), oss.str().c_str());
}

void update_playlist_time()
{
    int iNumSongs = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gtk_builder_get_object(builder, "Tracks")), NULL);
    int iMinutes = (int)floorf(g_fTotalPlaylistLength/60.0);
    std::ostringstream oss;
    oss << iNumSongs << " songs, " << iMinutes << " total minutes";
    gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder, "playlisttime")), oss.str().c_str());
}

std::list<std::string> get_cur_playlist()
{
    std::list<std::string> playlist;



    return playlist;
}

void add_song(std::string sFilename, std::string sTitle, std::string sArtist, std::string sAlbum, float fLength)
{
    //Create new item in the list
    GtkTreeIter iter;
    gtk_list_store_append(GTK_LIST_STORE(gtk_builder_get_object(builder, "Tracks")), &iter);
    GtkTreeModel* mod = gtk_tree_view_get_model(GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview2")));
    GtkTreePath* path = gtk_tree_model_get_path(mod, &iter);
    //Fill in list data
    set_table_data("treeview2", "Tracks", path, (gchar*)sFilename.c_str(), 0);
    set_table_data("treeview2", "Tracks", path, (gchar*)sTitle.c_str(), 1);
    set_table_data("treeview2", "Tracks", path, (gchar*)sArtist.c_str(), 2);
    set_table_data("treeview2", "Tracks", path, (gchar*)sAlbum.c_str(), 3);
    //TODO set_table_data("treeview2", "Tracks", path, (gchar*)sLength.c_str(), 4);
    g_fTotalPlaylistLength += fLength;
    update_playlist_time();

    //set_table_data("treeview2", "Tracks", path), "media-playback-start", 5);

    //Cleanup
    gtk_tree_path_free(path);
}

G_MODULE_EXPORT void drag_begins(GtkWidget *widget, GdkDragContext *drag_context, gpointer user_data)
{
    //Let the user reorganize the rows by disabling sorting on the columns
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(gtk_builder_get_object(builder, "Tracks")), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, GTK_SORT_ASCENDING);
}

G_MODULE_EXPORT void slider_move(GtkAdjustment *adjustment, gpointer user_data)
{
    if(bSlider)
    {
        bSlider = false;
        return;
    }
    //std::cout << "Seek to: " << gtk_adjustment_get_value(adjustment) << std::endl;
    //TODO set_music_loc(gtk_adjustment_get_value(adjustment)/100.0);
}

int numColumns(GtkTreeView *tree_view)
{
    GList* l = gtk_tree_view_get_columns(tree_view);
    int num = 0;
    for(GList* i = l; i != NULL; i = i->next)
        num++;
    g_list_free(l);
    return num;
}

G_MODULE_EXPORT void columns_changed(GtkTreeView *tree_view, gpointer user_data)
{
//    if(gtk_tree_view_get_n_columns(tree_view) == NUM_COLUMNS) //TODO GTK 3.4
    if(numColumns(tree_view) == NUM_COLUMNS)        //To supress GTK errors (Since the first column is deleted first)
        gtk_tree_view_move_column_after(tree_view, GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "Playing")), NULL);
}


