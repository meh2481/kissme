#include "signalhandler.h"
#include "sound.h"
#include "cover.h"
#include "fileoperations.h"
#include "playlist.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <string.h>

extern GtkBuilder *builder;
volatile bool bPaused = true;
volatile int iRepeatMode = REPEAT_NONE;
static float g_fTotalPlaylistLength = 0.0;
static std::string g_sLastAlbumArt = NO_IMAGE;    //For showing the last album art image we clicked on
static std::string g_sCurPlayingSong;  //Filename of song we're currently playing

//Local functions
gboolean clear_play_icons(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    set_table_data("treeview2", "Tracks", path, (gchar*)"", 5);
    return false;
}

gchar* g_cursong;
gboolean find_cur_song(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    gchar *icon;
		gtk_tree_model_get(model, iter, 5, &icon, -1);
		if(icon == NULL) return false;
		std::string s = icon;
		if(s == "")
			return false;
		else if(s == PLAY_ICON)	//This song currently playing
		{
			g_cursong = gtk_tree_path_to_string(path);
			return true;	//Done iterating
		}
    
    return false;
}

//Global functions declared in signalhandler.h

G_MODULE_EXPORT void button_addfile_clicked(GtkButton *button, ChData *data)
{
    //TODO: Should I be using GTK+ instead of flat GTK?
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
    std::set<std::string> slFilesSupported = get_filetypes_supported();
    for(std::set<std::string>::iterator i = slFilesSupported.begin(); i != slFilesSupported.end(); i++)
    {
        std::string sPattern = "*.";
        sPattern += *i;
        gtk_file_filter_add_pattern(filter, sPattern.c_str());
    }
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
        save();
    }

    gtk_widget_destroy (dialog);
}

G_MODULE_EXPORT void button_addfolder_clicked(GtkButton *button, ChData *data)
{
    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new ("Add Folders", GTK_WINDOW(data->main_window), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

    GtkFileChooser *filechooser = GTK_FILE_CHOOSER(dialog);

    gtk_file_chooser_set_select_multiple(filechooser, true);
    gtk_file_chooser_set_action(filechooser, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        //Get all filenames and add them
        GSList* filenames = gtk_file_chooser_get_filenames(filechooser);
        for(GSList* i = filenames; i != NULL; i=i->next)
        {
            std::string s = (char*)i->data;
            std::deque<std::string> sFilenames = get_files_from_dir_rec(s, get_filetypes_supported());
            for(std::deque<std::string>::iterator j = sFilenames.begin(); j != sFilenames.end(); j++)   //TODO: Can hang here for minutes on end as songs load.
            {
                if(!j->length())
                    continue;
                std::cout << "Adding file " << *j << " to playlist" << std::endl;
                add_to_playlist(*j);    //TODO: don't parse album data yet?
            }
            g_free(i->data);
        }
        g_slist_free(filenames);

        //Save list in case of crash
        save();
    }

    gtk_widget_destroy (dialog);
}

G_MODULE_EXPORT gboolean key_pressed(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	if(event->key.keyval == GDK_KEY_KP_Delete || event->key.keyval == GDK_KEY_Delete)
	{
		button_removesongs_clicked(NULL, NULL);
		return true;
	}

	return false;
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
    
    //Get current song that's playing
    g_cursong = NULL;
    GtkTreePath* curpath = NULL;
    gtk_tree_model_foreach(model, find_cur_song, NULL);
    if(g_cursong != NULL)
    	curpath = gtk_tree_path_new_from_string(g_cursong);
    bool bHit = false;
    bool bIsPlaying = is_playing();	//See if song is playing before we start
    
    //First loop so we don't invalidate any paths
    for(std::list<GtkTreeRowReference*>::iterator i = references.begin(); i != references.end(); i++)
    {
        GtkTreePath* path = gtk_tree_row_reference_get_path(*i);
        if(path != NULL)
        {
        		gchar* sIt = gtk_tree_path_to_string(path);
        		if(g_cursong != NULL && curpath != NULL && (std::string)sIt == (std::string)g_cursong)
        			bHit = true;	//If we hit a song, record it so we know to start the next song
        		else if(!bHit)
        			//If we haven't hit a song yet, we may be deleting songs above it before hitting it, so decrease path counter
        			//(If we don't end up hitting it later, no worries)
        			gtk_tree_path_prev(curpath);
        }
    }
    
    //Delete songs pointed to by these references
    for(std::list<GtkTreeRowReference*>::iterator i = references.begin(); i != references.end(); i++)
    {
        GtkTreePath* path = gtk_tree_row_reference_get_path(*i);
        if(path != NULL)
        {        			
        		//Remove this song
            GtkTreeIter iter;
            if(gtk_tree_model_get_iter(model, &iter, path))
            {
            	//Get how long this song was, and subtract that from our total length
            	gchar *name;
        			gtk_tree_model_get(model, &iter, 0, &name, -1);
						  if(name != NULL)
						  	g_fTotalPlaylistLength -= get_song_length(name);
            	
            	//Remove this item from list
            	gtk_list_store_remove(GTK_LIST_STORE(gtk_builder_get_object(builder, "Tracks")), &iter);
            }
            gtk_tree_path_free(path);
        }
        gtk_tree_row_reference_free(*i);
    }
		update_playlist_time();	//Update our little counter to show correct total playlist time
    
    //If we deleted the song we're playing, play the next song we should
    if(bHit)
    {
    	//See if we're past the total number of tracks
			int iNumTracks = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gtk_builder_get_object(builder, "Tracks")), NULL)-1;
			if(iNumTracks >= 0)	//If there are any tracks to play
			{
				std::istringstream iss(gtk_tree_path_to_string(curpath));
				int iCurTrack;
				iss >> iCurTrack;
				if(iCurTrack > iNumTracks)	//If we've passed how many tracks there are
				{
					std::ostringstream oss;
					oss << iNumTracks;
					curpath = gtk_tree_path_new_from_string(oss.str().c_str());	//Reset to last track
				}
								
		  	play_this_song(model, curpath);
		  	if(!bIsPlaying)
		  		pause_song();
    	}
    	else
    	{
    		stop_song();
    		clean_gui();
			}
		}
		
		//Select new current song
    if(curpath != NULL)
    {
    	GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview2")));
    	if(sel != NULL)
    		gtk_tree_selection_select_path(sel, curpath);
		}

    //Free memory
    g_list_free_full(selected, (GDestroyNotify) gtk_tree_path_free);

    save(); //Update our changes
}

G_MODULE_EXPORT void button_previous_clicked(GtkButton *button, ChData *data)
{
		GtkTreePath* path;
    gchar* pathspec;
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview2")));
    g_cursong = NULL;
    gtk_tree_model_foreach(model, find_cur_song, NULL);
    if(g_cursong != NULL)
    {
		  pathspec = g_cursong; 	//Store the song's path in a global variable, because easier than pointer-to-pointer
		  path = gtk_tree_path_new_from_string(pathspec);
		  std::string sTop = "0";
		  if(sTop == pathspec)	//Loop back 
		  {
		  	gtk_tree_path_free(path);
		  	std::ostringstream sFinal;
		  	sFinal << gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gtk_builder_get_object(builder, "Tracks")), NULL) - 1;
		  	path = gtk_tree_path_new_from_string(sFinal.str().c_str());
			}
			else
		  	gtk_tree_path_prev(path);
		  	
		  play_this_song(model, path);
		  
		  gtk_tree_path_free(path);
		  g_free(pathspec);
    }


    //rewind_song(); //TODO Song list
}

G_MODULE_EXPORT void button_next_clicked(GtkButton *button, ChData *data)
{
    next_song(true);
}

void next_song(bool bLoop)
{
		GtkTreePath* path;
    gchar* pathspec;
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview2")));
    g_cursong = NULL;
    gtk_tree_model_foreach(model, find_cur_song, NULL);
    if(g_cursong != NULL)
    {
		  pathspec = g_cursong;
		  path = gtk_tree_path_new_from_string(pathspec);
		  gtk_tree_path_next(path);
		  std::ostringstream sMax;	//Get # of list elements here
		  sMax << gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gtk_builder_get_object(builder, "Tracks")), NULL);
		  if(sMax.str() == gtk_tree_path_to_string(path))	//If we're incrementing past how many are here
		  {
		  	if(!bLoop)
		  	{
		  		rewind_song();
		  		pause_song();
		  	}
		  	else//if(bLoop)
		  	{
					gtk_tree_path_free(path);
					path = gtk_tree_path_new_from_string("0");	//Reset to top
					play_this_song(model, path);
		  	}
		  }
		  else
		  	play_this_song(model, path);
		  gtk_tree_path_free(path);
		  g_free(pathspec);
    }
}

G_MODULE_EXPORT void button_play_clicked(GtkButton *button, ChData *data)
{
		if(!song_is_valid())	//Not currently playing a valid song; select the top one and play it
		{
			//Play top selected song if there is one
			GtkTreeModel* model = GTK_TREE_MODEL(gtk_builder_get_object(builder, "Tracks"));
		  //Get list of selected songs
		  GList* selected = gtk_tree_selection_get_selected_rows(GTK_TREE_SELECTION(gtk_builder_get_object(builder, "selectedsongs")), &model);
		  //Grab first song
		  if(selected != NULL)
		  {
		      GtkTreePath* path = ((GtkTreePath*)selected->data);
		      if(path != NULL)
						play_this_song(GTK_TREE_MODEL(gtk_builder_get_object(builder, "Tracks")), path);	//Play first selected song in list
		      else	//Play top song
		      {
		      	path = gtk_tree_path_new_from_string("0");
						play_this_song(GTK_TREE_MODEL(gtk_builder_get_object(builder, "Tracks")), path);
						gtk_tree_path_free(path);
		      }
		  }
		  else	//Play top song
		  {
		  	GtkTreePath *path = gtk_tree_path_new_from_string("0");
				play_this_song(GTK_TREE_MODEL(gtk_builder_get_object(builder, "Tracks")), path);
				gtk_tree_path_free(path);
			}
		}
		else	//Otherwise, just play/pause
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
}

G_MODULE_EXPORT void button_repeat_clicked(GtkButton *button, ChData *data)
{
		//Cycle through repeat modes
    iRepeatMode++;
    if(iRepeatMode > REPEAT_ONE)
        iRepeatMode = REPEAT_NONE;
    //Set to the right icon, and set behavior accordingly
    switch(iRepeatMode)
    {
        case REPEAT_ALL:
            gtk_button_set_image(button, GTK_WIDGET(gtk_builder_get_object(builder, "ImgRepeatAll")));
            loop_song(false);
            break;
        case REPEAT_NONE:
            gtk_button_set_image(button, GTK_WIDGET(gtk_builder_get_object(builder, "ImgRepeatOff")));
            loop_song(false);
            break;
        case REPEAT_ONE:
            gtk_button_set_image(button, GTK_WIDGET(gtk_builder_get_object(builder, "ImgRepeatOne")));
            loop_song(true);
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
        //Set the image for the album art window to this image
        GtkWindow* albumartwindow = GTK_WINDOW(gtk_builder_get_object(builder, "albumartwindow"));
        GtkImage* img = GTK_IMAGE(gtk_builder_get_object(builder, "albumart_large"));
        gtk_image_set_from_file(img, g_sLastAlbumArt.c_str());
        gtk_widget_show(GTK_WIDGET(albumartwindow));

        //Set window title
        std::string sSongTitle = gtk_label_get_text(GTK_LABEL(gtk_builder_get_object(builder, "curtrack")));
        sSongTitle.erase(sSongTitle.find_first_of('\n'));
        gtk_window_set_title(albumartwindow, ("Album Art for " + sSongTitle).c_str());
    }
}

G_MODULE_EXPORT void button_newplaylist_clicked(GtkButton *button, ChData *data)
{
    //Clear last playlist name
    GtkEntry* textbox = GTK_ENTRY(gtk_builder_get_object(builder, "entry1"));
    gtk_entry_set_text(textbox, "");
    gtk_widget_grab_focus(GTK_WIDGET(textbox)); //In case this window was closed with "Cancel" button, this will regain the focus to the texbox

    //Show dialog for inputting new playlist name
    GtkDialog* newname = GTK_DIALOG(gtk_builder_get_object(builder, "dialog1"));
    gint ret = gtk_dialog_run(newname);
    gtk_widget_hide(GTK_WIDGET(newname));

    if(ret == GTK_RESPONSE_ACCEPT)
    {
        //On return, get name that the user entered
        const gchar* text = gtk_entry_get_text(textbox);
        if(text != NULL)
        {
            if(strlen(text))    //TODO Test playlist name to be sure it's not taken yet
            {
                GtkTreeIter iter;
                GtkListStore* playlists = GTK_LIST_STORE(gtk_builder_get_object(builder, "Playlists"));
                gtk_list_store_append(playlists, &iter);
                GValue a = G_VALUE_INIT;
                g_value_init (&a, G_TYPE_STRING);
                g_value_set_static_string (&a, text);
                gtk_list_store_set_value(playlists, &iter, 0, &a);
                
                //Highlight this newly-created playlist
                GtkTreeView* view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview1"));
                GtkTreePath* path = gtk_tree_model_get_path(gtk_tree_view_get_model(view), &iter);
                gtk_tree_selection_select_path(gtk_tree_view_get_selection(view), path);
                gtk_tree_path_free(path);
                
                //Create new playlist
                playlist_play(text);
                
                //Make sure alphabetically sorted
                resort_playlist_pane();
            }
        }
    }
}

G_MODULE_EXPORT void button_import_clicked(GtkButton *button, ChData *data)
{	
	GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new ("Import Playlist",
                                          GTK_WINDOW(data->main_window),
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                          NULL);

    GtkFileChooser *filechooser = GTK_FILE_CHOOSER(dialog);

    gtk_file_chooser_set_select_multiple(filechooser, false);

    //Add filters to our file chooser
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Playlist Files");
    std::set<std::string> slFilesSupported = get_playlisttypes_supported();
    for(std::set<std::string>::iterator i = slFilesSupported.begin(); i != slFilesSupported.end(); i++)
    {
        std::string sPattern = "*.";
        sPattern += *i;
        gtk_file_filter_add_pattern(filter, sPattern.c_str());
    }
    gtk_file_chooser_add_filter(filechooser, filter);

    GtkFileFilter* filter2 = gtk_file_filter_new();
    gtk_file_filter_set_name(filter2, "All Files");
    gtk_file_filter_add_pattern(filter2, "*");
    gtk_file_chooser_add_filter(filechooser, filter2);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        //Get filename and open
        std::string sFilename = gtk_file_chooser_get_filename(filechooser);
        std::list<song> sFiles = playlist_load(sFilename);
        
        std::string sListName = ttvfs::StripFileExtension(ttvfs::PathToFileName(sFilename.c_str()));	//Name of our new playlist 
        
        //Add playlist to our manager
        playlist_add(sListName, sFiles);
        
        //Add new playlist to our view
        GtkTreeIter iter;
        GtkListStore* playlists = GTK_LIST_STORE(gtk_builder_get_object(builder, "Playlists"));
        gtk_list_store_append(playlists, &iter);
        GValue a = G_VALUE_INIT;
        g_value_init (&a, G_TYPE_STRING);
        g_value_set_static_string (&a, sListName.c_str());
        gtk_list_store_set_value(playlists, &iter, 0, &a);

				//Highlight this new list
        GtkTreeView* view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview1"));
        GtkTreePath* path = gtk_tree_model_get_path(gtk_tree_view_get_model(view), &iter);
        gtk_tree_selection_select_path(gtk_tree_view_get_selection(view), path);
        gtk_tree_path_free(path);
    }

    gtk_widget_destroy (dialog);
    
    //Keep this alphabetically sorted
    resort_playlist_pane();
}

G_MODULE_EXPORT void button_deleteplaylist_clicked(GtkButton *button, ChData *data)
{
	//TODO
}

G_MODULE_EXPORT void newplaylist_ok(GtkButton *button, ChData *data)
{
    GtkDialog* dialog = GTK_DIALOG(gtk_builder_get_object(builder, "dialog1"));
    gtk_dialog_response(dialog, GTK_RESPONSE_ACCEPT);
    //gtk_dialog_response(dialog, GTK_RESPONSE_DELETE_EVENT);
}

G_MODULE_EXPORT void newplaylist_cancel(GtkButton *button, ChData *data)
{
    GtkDialog* dialog = GTK_DIALOG(gtk_builder_get_object(builder, "dialog1"));
    gtk_dialog_response(dialog, GTK_RESPONSE_CANCEL);
    //gtk_dialog_response(dialog, GTK_RESPONSE_DELETE_EVENT);
}

G_MODULE_EXPORT void volume_changed(GtkScaleButton *button, gdouble value, ChData *data)
{
    setVolume(value);
}

void play_this_song(GtkTreeModel *model, GtkTreePath *path)
{
    GtkTreeIter iter;
    
    if (gtk_tree_model_get_iter(model, &iter, path))
    {
        gchar *name;

        gtk_tree_model_get(model, &iter, 0, &name, -1);

        g_sCurPlayingSong = name;
        draw_album_art(get_album_art(name));    //Load album art for this song

        load_song(name);
        loop_song(iRepeatMode == REPEAT_ONE);

        g_free(name);

        //Get track name to display
        gtk_tree_model_get(model, &iter, 1, &name, -1);
        std::string sTitle = name;
        g_free(name);
        gtk_tree_model_get(model, &iter, 2, &name, -1); //And artist
        std::ostringstream oss;
        oss << "<b>" << g_markup_escape_text(sTitle.c_str(), sTitle.length()) << "</b>\n  <i>" << g_markup_escape_text(name, strlen(name)) << "</i>";
        //Draw artist name and track name

        gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(builder, "curtrack")), oss.str().c_str());
        g_free(name);

        //Also set window title to show we're playing this song
        if(sTitle.length())
            gtk_window_set_title(GTK_WINDOW(gtk_builder_get_object(builder, "window1")), ("kissme - " + sTitle).c_str());
        else
            gtk_window_set_title(GTK_WINDOW(gtk_builder_get_object(builder, "window1")), "kissme");

        //And show play icon
        //TODO: See if better way to do this when we have huge list?
        gtk_tree_model_foreach(model, clear_play_icons, NULL);  //Clear previous play icons (Rather than keeping track of one, which doesn't work on drag/drop)
        set_table_data("treeview2", "Tracks", path, (gchar*)PLAY_ICON, 5);
    }
}

G_MODULE_EXPORT void song_selected(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, ChData *data)
{
    GtkTreeModel *model;

    model = gtk_tree_view_get_model(tree_view);

    play_this_song(model, path);
}

void set_table_data(std::string sTreeViewName, std::string sListStoreName, GtkTreePath *path, gchar* new_text, gint column)
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
    int iMinutes = (int)floorf(g_fTotalPlaylistLength/60.0) % 60;
    int iHours = (int)floorf(g_fTotalPlaylistLength/3600.0) % 24;
    int iDays = (int)floorf(g_fTotalPlaylistLength/86400.0);
    std::ostringstream oss;
    oss << iNumSongs << " song";
    if(iNumSongs != 1)
    	oss << 's';
    oss << " - ";
    if(iDays)
    {
    	oss << iDays << " day";
    	if(iDays != 1)
    		oss << 's';
		}
		if(iHours)
		{
			if(iDays)
				oss << ", ";
			oss << iHours << " hour";
			if(iHours != 1)
				oss << 's';
		}
		if(iMinutes)
		{
			if(iHours || iDays)
				oss << ", ";
			oss << iMinutes << " minute";
			if(iMinutes != 1)
				oss << 's';
		}
    if(g_fTotalPlaylistLength < 60.0)
    	oss << (int)floorf(g_fTotalPlaylistLength) << " seconds";
    
    gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder, "playlisttime")), oss.str().c_str());
}

std::list<song> get_cur_playlist()
{
    std::list<song> playlist;

    GtkTreeModel* tree_model = GTK_TREE_MODEL(gtk_builder_get_object(builder, "Tracks"));
    GtkTreeIter iter;
    if(!gtk_tree_model_get_iter_first(tree_model, &iter))
        return playlist;

    //Loop through tree model, populating list
    while(true)
    {
    		song s;
        GValue value = G_VALUE_INIT;
        //Filename
        gtk_tree_model_get_value(tree_model, &iter, 0, &value);
        const gchar* text = g_value_get_string(&value);
        if(text != NULL)
          s.filename = text;
        g_value_unset(&value);
        
        //title
        gtk_tree_model_get_value(tree_model, &iter, 1, &value);
        text = g_value_get_string(&value);
        if(text != NULL)
          s.title = text;
        else
        	s.title = "";
        g_value_unset(&value);
        
        //artist
        gtk_tree_model_get_value(tree_model, &iter, 2, &value);
        text = g_value_get_string(&value);
        if(text != NULL)
          s.artist = text;
        else
        	s.artist = "";
        g_value_unset(&value);
        
        //album
        gtk_tree_model_get_value(tree_model, &iter, 3, &value);
        text = g_value_get_string(&value);
        if(text != NULL)
          s.album = text;
        else
        	s.album = "";
        g_value_unset(&value);
        
        //track
        std::istringstream track;
        gtk_tree_model_get_value(tree_model, &iter, 6, &value);
        text = g_value_get_string(&value);
        if(text != NULL)
            track.str(text);
        g_value_unset(&value);
        if(!(track >> s.track))
        	s.track = 0;
        
        //length
        std::string sLen;
        gtk_tree_model_get_value(tree_model, &iter, 4, &value);
        text = g_value_get_string(&value);
        if(text != NULL)
            sLen = text;
        g_value_unset(&value);
        size_t colon = sLen.find(':');
        if(colon != std::string::npos)
        	sLen.replace(colon, 1, 1, ' ');
        std::istringstream length(sLen);
        int minutes=0, seconds=0;
        if(!(length >> minutes >> seconds))
        	minutes = seconds = 0;
        s.length = minutes * 60 + seconds;
        
        playlist.push_back(s);
        
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
    set_table_data("treeview2", "Tracks", path, (gchar*)sFilename.c_str(), 0);
    set_table_data("treeview2", "Tracks", path, (gchar*)sTitle.c_str(), 1);
    set_table_data("treeview2", "Tracks", path, (gchar*)sArtist.c_str(), 2);
    set_table_data("treeview2", "Tracks", path, (gchar*)sAlbum.c_str(), 3);
    set_table_data("treeview2", "Tracks", path, (gchar*)oss.str().c_str(), 6);
    //Set length
    oss.str("");
    if(fLength > 0.0)
    {
        oss.fill('0');
        oss << (int)floorf(fLength/60.0) << ":" << std::setw(2) << (int)floorf(fLength) % 60;
        set_table_data("treeview2", "Tracks", path, (gchar*)oss.str().c_str(), 4);
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
    //TODO tyrsound doesn't fully support this yet.
    //set_music_loc(gtk_adjustment_get_value(adjustment)/100.0);
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

void clean_gui()
{
	draw_album_art("logo.png");
}

static std::string sCurPlaylist = "";
G_MODULE_EXPORT void playlist_selected(GtkTreeSelection *treeselection, gpointer user_data)
{
	//Save old playlist
	save_cur_playlist(playlist_currrently_viewing());
	save_config();
	
	//Find selection
	GtkTreeIter iter;
	GtkTreeModel* tree_model = GTK_TREE_MODEL(gtk_builder_get_object(builder, "Playlists"));
	if(gtk_tree_selection_get_selected(treeselection, &tree_model, &iter))
	{
		GValue value = G_VALUE_INIT;
    gtk_tree_model_get_value(tree_model, &iter, 0, &value);
    const gchar* text = g_value_get_string(&value);
    if(text != NULL)
    {
    	//The "changed" signal can be emitted at any time, so make sure we aren't switching to the same playlist we're already in
    	if(playlist_currrently_viewing() == text)
    		return;
    	playlist_play(text);
    	sCurPlaylist = text;
    }
    g_value_unset(&value);
	}
}

std::string playlist_currrently_viewing()
{
	return sCurPlaylist;
}

void clear_now_playing()
{
	gtk_list_store_clear(GTK_LIST_STORE(gtk_builder_get_object(builder, "Tracks")));
	g_fTotalPlaylistLength = 0;	//Reset total time counter
}

static bool g_bMaximized = false;
static int g_posx, g_posy;
static int g_sizex, g_sizey;
G_MODULE_EXPORT gboolean window_changed(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	if(event->window_state.new_window_state & GDK_WINDOW_STATE_MAXIMIZED)
		g_bMaximized = true;
	else
		g_bMaximized = false;
	
	return false;
}

bool get_window_maximized()
{
	return g_bMaximized;
}

void get_window_position(int* x, int* y)
{
	*x = g_posx;
	*y = g_posy;
}

void get_window_size(int* x, int* y)
{
	*x = g_sizex;
	*y = g_sizey;
}

//I don't know why we have to do this... I blame GTK
gboolean check_window_pos(gpointer data)
{
	GtkWindow* w = GTK_WINDOW(gtk_builder_get_object(builder, "window1"));
	gtk_window_get_position(w, &g_posx, &g_posy);
	gtk_window_get_size(w, &g_sizex, &g_sizey);
	return true;
}

G_MODULE_EXPORT void mainwindow_hidden(GtkWidget *widget, gpointer user_data)
{
	save();		//Because playlist
}












