#include "signalhandler.h"
#include "sound.h"
#include "cover.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <cmath>
#include <iomanip>

extern GtkBuilder *builder;
bool bPaused = true;
int iRepeatMode = REPEAT_NONE;
float g_fTotalPlaylistLength = 0.0;
std::string g_sLastAlbumArt = NO_IMAGE;    //For showing the last album art image we clicked on
std::string g_sCurPlayingSong;  //Filename of song we're currently playing

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
    gtk_file_filter_add_pattern(filter, "*.mp3");   //TODO
    gtk_file_filter_add_pattern(filter, "*.ogg");   //Ogg vorbis
    gtk_file_filter_add_pattern(filter, "*.wma");   //TODO?
    gtk_file_filter_add_pattern(filter, "*.wav");   //TODO
    gtk_file_filter_add_pattern(filter, "*.m4a");   //TODO?
    gtk_file_filter_add_pattern(filter, "*.flac");  //TODO
    gtk_file_filter_add_pattern(filter, "*.opus");  //Opus

    //libGME supported formats
    gtk_file_filter_add_pattern(filter, "*.ay");
    gtk_file_filter_add_pattern(filter, "*.gbs");
    gtk_file_filter_add_pattern(filter, "*.gym");
    gtk_file_filter_add_pattern(filter, "*.hes");
    gtk_file_filter_add_pattern(filter, "*.kss");
    gtk_file_filter_add_pattern(filter, "*.nsf");
    gtk_file_filter_add_pattern(filter, "*.nsfe");
    gtk_file_filter_add_pattern(filter, "*.sap");
    gtk_file_filter_add_pattern(filter, "*.spc");
    gtk_file_filter_add_pattern(filter, "*.vgm");
    gtk_file_filter_add_pattern(filter, "*.vgz");

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
            add_to_playlist(s);
            g_free(i->data);
        }
        g_slist_free(filenames);

        //Save list in case of crash
        save_playlist();
    }

    gtk_widget_destroy (dialog);
}

G_MODULE_EXPORT void button_removesongs_clicked(GtkButton *button, ChData *data)
{
    //TODO See if currently-playing song has been deleted, and stop playing it if so

    GtkTreeModel* model = GTK_TREE_MODEL(gtk_builder_get_object(builder, "Tracks"));
    //Get list of selected songs
    GList* selected = gtk_tree_selection_get_selected_rows(GTK_TREE_SELECTION(gtk_builder_get_object(builder, "selectedsongs")), &model);
    //Create list of references out of this list
    std::list<GtkTreeRowReference*> references;
    while(selected != NULL)
    {
        GtkTreePath* path = ((GtkTreePath*)selected->data);
        if(path != NULL)
        {
            GtkTreeRowReference* ref = gtk_tree_row_reference_new(model, path);
            references.push_back(ref);
        }
        selected = selected->next;
    }
    //Delete songs pointed to by these references
    for(std::list<GtkTreeRowReference*>::iterator i = references.begin(); i != references.end(); i++)
    {
        GtkTreePath* path = gtk_tree_row_reference_get_path(*i);
        if(path != NULL)
        {
            GtkTreeIter iter;
            if(gtk_tree_model_get_iter(model, &iter, path))
                gtk_list_store_remove(GTK_LIST_STORE(gtk_builder_get_object(builder, "Tracks")), &iter);
            gtk_tree_path_free(path);
        }
        gtk_tree_row_reference_free(*i);
    }

    //Free memory
    g_list_free_full(selected, (GDestroyNotify) gtk_tree_path_free);

    save_playlist();    //Update our changes
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

G_MODULE_EXPORT void button_albumart_clicked(GtkButton *button, ChData *data)
{
    if(g_sCurPlayingSong.empty())
        return;
    if(g_sLastAlbumArt == NO_IMAGE)
    {
        //Set album art
        GtkWidget *dialog;
        dialog = gtk_file_chooser_dialog_new ("Set cover art", GTK_WINDOW(data->main_window), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

        GtkFileChooser *filechooser = GTK_FILE_CHOOSER(dialog);

        //Add filters to our file chooser
        GtkFileFilter* filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, "Image Files (*.jpg, *.png)");
        gtk_file_filter_add_pattern(filter, "*.jpg");
        gtk_file_filter_add_pattern(filter, "*.png");

        gtk_file_chooser_add_filter(filechooser, filter);

        if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
        {
            //Get all filenames and add them
            gchar* sFilename = gtk_file_chooser_get_filename(filechooser);
            if(sFilename != NULL)
            {
                //std::cout << "Set image: " << sFilename << std::endl;
                if(set_album_art(g_sCurPlayingSong, sFilename))
                    draw_album_art(sFilename);
            }
        }

        gtk_widget_destroy (dialog);
    }
    else
    {
        GtkWidget* albumartwindow = GTK_WIDGET(gtk_builder_get_object(builder, "albumartwindow"));
        GtkWidget* img = gtk_image_new_from_file(g_sLastAlbumArt.c_str());
        //Destroy GtkImages currently in album art window (Why we can't just set this image, I have no idea...)
        for(GList* contList = gtk_container_get_children(GTK_CONTAINER(albumartwindow)); contList != NULL && contList->data != NULL; contList = contList->next)
            gtk_widget_destroy(GTK_WIDGET(contList->data));

        gtk_container_add (GTK_CONTAINER(albumartwindow), img);
        gtk_widget_show(img);
        gtk_widget_show(albumartwindow);

        //Set window title
        std::string sSongTitle = gtk_label_get_text(GTK_LABEL(gtk_builder_get_object(builder, "curtrack")));
        sSongTitle.erase(sSongTitle.find_first_of('\n'));
        gtk_window_set_title(GTK_WINDOW(albumartwindow), ("Album Art for " + sSongTitle).c_str());
    }
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

        g_sCurPlayingSong = name;
        draw_album_art(get_album_art(name));    //Load album art for this song

        //load_song(name);
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

        //TODO: Check for unparsable markup (like & instead of &amp;)
        gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(builder, "curtrack")), oss.str().c_str());
        g_free(name);

        //Also set window title to show we're playing this song
        if(sTitle.length())
            gtk_window_set_title(GTK_WINDOW(gtk_builder_get_object(builder, "window1")), ("kissme - " + sTitle).c_str());
        else
            gtk_window_set_title(GTK_WINDOW(gtk_builder_get_object(builder, "window1")), "kissme");

        //And show play icon
        gtk_tree_model_foreach(model, clear_play_icons, data);  //Clear previous play icons (Rather than keeping track of one, which doesn't work on drag/drop)
        set_table_data("treeview2", "Tracks", path, PLAY_ICON, 5);
    }
}

void set_table_data(std::string sTreeViewName, std::string sListStoreName, GtkTreePath *path, std::string new_text, gint column)
{
    //All this just to set the table value? OH COME ON!
    GtkTreeModel* mod = gtk_tree_view_get_model(GTK_TREE_VIEW(gtk_builder_get_object(builder, sTreeViewName.c_str())));
    GtkTreeIter i;
    gtk_tree_model_get_iter(mod, &i, path);
    GValue a = G_VALUE_INIT;
    g_value_init (&a, G_TYPE_STRING);
    g_value_set_static_string (&a, new_text.c_str());
    gtk_list_store_set_value(GTK_LIST_STORE(gtk_builder_get_object(builder, sListStoreName.c_str())), &i, column, &a);
}

bool tag_edited(gchar *path, gchar *new_text, tagType change)
{
    bool bReturn = false;
    //Get filename
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview2")));
    GtkTreeIter iter;
    GtkTreePath* tp = gtk_tree_path_new_from_string(path);
    //model = gtk_tree_view_get_model(tree_view);
    if (gtk_tree_model_get_iter(model, &iter, tp))
    {
        gchar *name;
        gtk_tree_model_get(model, &iter, 0, &name, -1);
        if(name != NULL)
            bReturn = change_tag(name, change, new_text);
    }
    gtk_tree_path_free(tp);
    return bReturn;
}

G_MODULE_EXPORT void title_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, ChData *data)
{
    if(tag_edited(path, new_text, CHANGE_TITLE))
        set_table_data("treeview2", "Tracks", gtk_tree_path_new_from_string(path), new_text, 1);
}

G_MODULE_EXPORT void artist_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, ChData *data)
{
    if(tag_edited(path, new_text, CHANGE_ARTIST))
        set_table_data("treeview2", "Tracks", gtk_tree_path_new_from_string(path), new_text, 2);
}

G_MODULE_EXPORT void album_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, ChData *data)
{
    if(tag_edited(path, new_text, CHANGE_ALBUM))
        set_table_data("treeview2", "Tracks", gtk_tree_path_new_from_string(path), new_text, 3);
}

G_MODULE_EXPORT void track_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, ChData *data)
{
    if(tag_edited(path, new_text, CHANGE_TRACK))
        set_table_data("treeview2", "Tracks", gtk_tree_path_new_from_string(path), new_text, 6);
}

G_MODULE_EXPORT void playlistname_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, ChData *data)
{
    set_table_data("treeview1", "Playlists", gtk_tree_path_new_from_string(path), new_text, 0);
    //TODO Save under some new name or such
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
    draw_album_art("logo.png");
}

bool bSlider = false;   //So we don't try to seek every time we update the play slider...
void update_play_slider(float fPos, float fLen)
{
    bSlider = true;
    if(fLen <= 0.0)
    {
        gtk_adjustment_set_value(GTK_ADJUSTMENT(gtk_builder_get_object(builder, "posadjustment")), 0.0);
        gtk_range_set_fill_level(GTK_RANGE(gtk_builder_get_object(builder, "playpos")), 0.0);
        gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder, "playposlabel")), "0:00 of 0:00");
        return;
    }
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

    GtkTreeModel* tree_model = GTK_TREE_MODEL(gtk_builder_get_object(builder, "Tracks"));
    GtkTreeIter iter;
    if(!gtk_tree_model_get_iter_first(tree_model, &iter))
        return playlist;

    //Loop through tree model, populating list
    while(true)
    {
        GValue value = G_VALUE_INIT;
        gtk_tree_model_get_value(tree_model, &iter, 0, &value);
        const gchar* text = g_value_get_string(&value);
        if(text != NULL)
            playlist.push_back(text);
        g_value_unset(&value);
        if(!gtk_tree_model_iter_next(tree_model, &iter))
            break;
    }

    return playlist;
}

void add_song(std::string sFilename, std::string sTitle, std::string sArtist, std::string sAlbum, uint track, float fLength)
{
    //Create new item in the list
    GtkTreeIter iter;
    gtk_list_store_append(GTK_LIST_STORE(gtk_builder_get_object(builder, "Tracks")), &iter);
    GtkTreeModel* mod = gtk_tree_view_get_model(GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview2")));
    GtkTreePath* path = gtk_tree_model_get_path(mod, &iter);
    std::ostringstream oss;
    if(track > 0)   //Fill track column if this is a valid track number
        oss << track;
    //Fill in list data
    set_table_data("treeview2", "Tracks", path, sFilename.c_str(), 0);
    set_table_data("treeview2", "Tracks", path, sTitle.c_str(), 1);
    set_table_data("treeview2", "Tracks", path, sArtist.c_str(), 2);
    set_table_data("treeview2", "Tracks", path, sAlbum.c_str(), 3);
    set_table_data("treeview2", "Tracks", path, oss.str().c_str(), 6);
    //Set length
    oss.str("");
    if(fLength > 0.0)
    {
        oss.fill('0');
        oss << (int)floorf(fLength/60.0) << ":" << std::setw(2) << (int)floorf(fLength) % 60;
        set_table_data("treeview2", "Tracks", path, oss.str(), 4);
    }

    //Update playlist length
    g_fTotalPlaylistLength += fLength;
    update_playlist_time();

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

void draw_album_art(std::string sFilename)
{
    g_sLastAlbumArt = sFilename;
    GtkImage *image = GTK_IMAGE(gtk_builder_get_object(builder, "album"));
    GdkPixbuf *pixbuf;  //TODO: Clean up?
    pixbuf=gdk_pixbuf_new_from_file(sFilename.c_str(),NULL);
    pixbuf=gdk_pixbuf_scale_simple(pixbuf, ALBUM_ART_ICON_WIDTH, ALBUM_ART_ICON_HEIGHT, GDK_INTERP_BILINEAR);
    gtk_image_set_from_pixbuf(image, pixbuf);
}
