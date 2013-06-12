#include "sound.h"
#include "signalhandler.h"
#include <iostream>
#include <list>
#include <fstream>
#include <sstream>

//Global variables for use by our functions here
tyrsound_Handle handle = TYRSOUND_NULLHANDLE;
extern int      iRepeatMode;
std::list<std::string>    g_lCurPlaylist; //Current list of songs we're playing

void init_sound()
{
    if(tyrsound_init(NULL, NULL) != TYRSOUND_ERR_OK)
    {
        std::cout << "Failed to init tyrsound." << std::endl;
        exit(1);
    }
	atexit(cleanup_sound);
}

void cleanup_sound()
{
    tyrsound_shutdown();
}

void load_song(std::string sFilename)
{
    if(handle != TYRSOUND_NULLHANDLE)
    {
        tyrsound_unload(handle);
    }

    tyrsound_Stream strm;
    if(tyrsound_createFileNameStream(&strm, sFilename.c_str(), "rb") != TYRSOUND_ERR_OK)
    {
        std::cout << "File not found: " << sFilename << std::endl;
        exit(1);
    }

    handle = tyrsound_load(strm, NULL);

    if(handle == TYRSOUND_NULLHANDLE)
    {
        std::cout << "Invalid handle for song " << sFilename << std::endl;
        exit(1);
    }


    tyrsound_Error err = tyrsound_play(handle);

    if(err != TYRSOUND_ERR_OK)
    {
        std::cout << "Failed to start playback on file " << sFilename << " Err: " << err << std::endl;
        exit(1);
    }
}

gboolean check_music_playing(gpointer data)
{
    if(!tyrsound_isPlaying(handle))
    {
        switch(iRepeatMode)
        {
            case REPEAT_ALL:
                //TODO
                break;
            case REPEAT_NONE:
                //TODO stop
                break;
            case REPEAT_ONE:
                tyrsound_seek(handle, 0.0);
                tyrsound_play(handle);
                break;
        }
    }
    if(tyrsound_isPlaying(handle))
        show_pause();
    else
        show_play();

    tyrsound_update();  //Update OpenAL

    if(handle != TYRSOUND_NULLHANDLE)
        update_play_slider(tyrsound_getPlayPosition(handle), tyrsound_getLength(handle));   //Update the slider to show where our current song is playing

    return true;
}

void play_song()
{
    tyrsound_play(handle);
}

void pause_song()
{
    tyrsound_pause(handle);
}

void rewind_song()
{
    tyrsound_seek(handle, 0.0f);
}

void setVolume(float fVol)
{
    tyrsound_setVolume(handle, fVol);
}

void add_to_playlist(std::string sFilename)
{
    //Get data for song
    //ID3_Tag mp3Tag(sFilename.c_str());
    TagLib::FileRef f(sFilename.c_str());
    if(f.isNull())
    {
        add_song(sFilename, "", "", "", 0, 0.0);
        g_lCurPlaylist.push_back(sFilename);
        return;
    }
    std::string sAlbum = f.tag()->album().to8Bit(true);
    std::string sTitle = f.tag()->title().to8Bit(true);
    std::string sArtist = f.tag()->artist().to8Bit(true);
    uint iTrack = f.tag()->track();

    //Write this all to the proper location in the table
    add_song(sFilename, sTitle, sArtist, sAlbum, iTrack, f.audioProperties()->length());
    g_lCurPlaylist.push_back(sFilename);
}

void save_playlist()
{
    //For now, just shove all the data out to the file, without caring about format
    std::ofstream playlistFile("kissme.last");
    if(playlistFile.fail()) return;
    std::list<std::string> playlist = get_cur_playlist();
    for(std::list<std::string>::iterator i = playlist.begin(); i != playlist.end(); i++)
        playlistFile << *i << std::endl;
    playlistFile.close();
}

void load_playlist()
{
    std::ifstream playlistFile("kissme.last");
    while(!playlistFile.fail() && !playlistFile.eof())
    {
        std::string s;
        getline(playlistFile, s);
        if(s.size())
        {
            add_to_playlist(s);
        }
    }
    playlistFile.close();
}

void set_music_loc(float fPos)
{
    if(handle != TYRSOUND_NULLHANDLE)
        tyrsound_seek(handle, fPos*tyrsound_getLength(handle));
}

bool change_tag(std::string sFilename, tagType tagToChange, std::string sNewTag)
{
    TagLib::FileRef f(sFilename.c_str());

    if(f.isNull())  //Can't really do anything
    {
        std::cout << "Warning: can't change tag on file " << sFilename << std::endl;
        return false;
    }

    std::istringstream iss(sNewTag);
    int32_t track = 0;
    switch(tagToChange)
    {
        case CHANGE_ARTIST:
            f.tag()->setArtist(sNewTag);
            break;
        case CHANGE_ALBUM:
            f.tag()->setAlbum(sNewTag);
            break;
        case CHANGE_TITLE:
            f.tag()->setTitle(sNewTag);
            break;
        case CHANGE_TRACK:
            if(!sNewTag.size()) //Erase tag by clearing it
            {
                f.tag()->setTrack(0);
                break;
            }
            if(!(iss >> track) || track <= 0)   //Only accept valid numbers; no negative numbers allowed    //TODO: What if we type something like "1noodle"? How should I handle this?
                return false;
            f.tag()->setTrack(track);
            break;
        default:
            std::cout << "Err: Unknown tag type " << tagToChange << ". Ignoring..." << std::endl;
            break;
    }

    if(!f.save())
    {
        std::cout << "Err: Unable to save tag changes to file " << sFilename << "." << std::endl;
        return false;
    }

    return true;
}

float get_song_length()
{
    if(handle == TYRSOUND_NULLHANDLE)
        return 0.0f;
    return tyrsound_getLength(handle);
}

//#include <flacfile.h>
//#include <tlist.h>
//#include <vorbisfile.h>

/*#include<tbytevector.h>//ByteVector
#include<mpegfile.h>//mp3 file
#include<id3v2tag.h>//tag
#include<id3v2frame.h>//frame
#include <attachedpictureframe.h>//attachedPictureFrame*/

#include <vorbisfile.h>
#include <xiphcomment.h>
#include <flacpicture.h>
std::string get_album_art(std::string sFilename)
{
    std::string ret = "logo.png";   //Default: Show kissme logo
 /*   std::cout << "Getting album art for file " << sFilename << std::endl;
    TagLib::Ogg::Vorbis::File file(sFilename.c_str());
//    const TagLib::List<TagLib::FLAC::Picture*>& picList = file.pictureList();
    //TagLib::FLAC::Picture* pic = //picList[0];
    //TagLib::Ogg::Vorbis::File audioFile(sFilename);
    TagLib::Ogg::XiphComment *tag = file.tag();
    std::cout << "Title: " << tag->title() << std::endl;
    if(tag->contains("METADATA_BLOCK_PICTURE"))
        std::cout << "Found METADATA_BLOCK_PICTURE in Ogg file" << std::endl;
    if(tag->contains("COVERART"))
        std::cout << "Found COVERART in Ogg file" << std::endl;
    const TagLib::Ogg::FieldListMap& map = tag->fieldListMap();

    std::cout << "Num of ogg tag fields: " << map.size() << std::endl;
    for(TagLib::Ogg::FieldListMap::ConstIterator i = map.begin(); i != map.end(); i++)
    {

        std::cout << "Tag :" << i->first << ", " << std::endl;// << i->second << std::endl;
    }

    TagLib::FLAC::Picture* picture = new TagLib::FLAC::Picture();
    picture->setData(imageData);
    picture->setType((TagLib::FLAC::Picture::Type) 0x03); // FrontCover
    picture->setMimeType("image/jpeg");
    picture->setDescription("Front Cover");
    TagLib::ByteVector block = picture->render();
    tag->addField("METADATA_BLOCK_PICTURE", b64_encode(block.data(), block.size()), true);*/

    //TagLib::MPEG::File mp3File(sFilename.c_str());
    //TagLib::ID3v2::Tag * mp3Tag;
    //TagLib::ID3v2::FrameList listOfMp3Frames;
    /*TagLib::ID3v2::AttachedPictureFrame * pictureFrame;

    mp3Tag= mp3File.ID3v2Tag();
    if(mp3Tag)
    {
        listOfMp3Frames = mp3Tag->frameListMap()["APIC"];//look for picture frames only
        if(!listOfMp3Frames.isEmpty())
        {
            TagLib::ID3v2::FrameList::ConstIterator it= listOfMp3Frames.begin();
            for(; it != listOfMp3Frames.end() ; it++)
            {
                pictureFrame = static_cast<TagLib::ID3v2::AttachedPictureFrame *> (*it);//cast Frame * to AttachedPictureFrame*

                //Warning. format of picture assumed to be jpg. This may be false, for example it may be png.
                FILE * fout = fopen("outputFile.jpg", "wb");
                std::cout<<"processing the file "<< sFilename <<std::endl<<std::endl;
                fwrite(pictureFrame->picture().data(), pictureFrame->picture().size(), 1, fout);
                fclose(fout);
                std::cout<<" The picture has been written to \t outputFile.jpg  \nRemember that the file type .jpg is just assumed for simplicity"<<std::endl<<std::endl;
            }
        }
        else std::cerr<<"there seem to be no picture frames (APIC) frames in this file"<<std::endl<<std::endl;
    }
    else std::cerr<<"the file "<<sFilename<<"does not appear to have any mp3 tags"<<std::endl<<std::endl;*/

    TagLib::Ogg::Vorbis::File audioFile(sFilename.c_str());
    TagLib::Ogg::XiphComment *tag = audioFile.tag();
    //const TagLib::Ogg::FieldListMap& map = tag->fieldListMap();

    TagLib::StringList art_list = tag->fieldListMap()[ "COVERART" ];
    std::cout << "Got coverart" << std::endl;

    if(tag->contains("METADATA_BLOCK_PICTURE"))
    {
        std::cout << "Found METADATA_BLOCK_PICTURE in Ogg file" << std::endl;
        const TagLib::StringList s = tag->fieldListMap()[ "METADATA_BLOCK_PICTURE" ];
        TagLib::FLAC::Picture* picture = new TagLib::FLAC::Picture();
        picture->setData(tag->render());
        TagLib::ByteVector b = picture->data();
        std::cout << "width x height: " << picture->width() << ", " << picture->height() << std::endl;
    }
    if(tag->contains("COVERART"))
    {
        std::cout << "Found COVERART in Ogg file" << std::endl;
    }










    return ret;
}








