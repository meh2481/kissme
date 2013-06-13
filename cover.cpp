// Support for getting/setting cover art for audio files
// Modified examples from http://stackoverflow.com/questions/4752020/how-do-i-use-taglib-to-read-write-coverart-in-different-audio-formats
// Retrieved 6/12/13 by Mark Hutcheson

#include "cover.h"
#include "sound.h"
#include "signalhandler.h"
#include <vorbisfile.h>
#include <xiphcomment.h>
#include <flacpicture.h>
#include <mpegfile.h>
#include <attachedpictureframe.h>
#include <id3v2tag.h>
#include <mp4file.h>
#include <mp4tag.h>
#include <mp4coverart.h>
#include <stdio.h>
#include <stdint.h>

#include <iostream>

class ImageFile : public TagLib::File
{
public:
    ImageFile(const char *file) : TagLib::File(file)
    {

    }

    TagLib::ByteVector data()
    {
        return readBlock(length());
    }


private:
    virtual TagLib::Tag *tag() const { return 0; }
    virtual TagLib::AudioProperties *audioProperties() const { return 0; }
    virtual bool save() { return false; }
};


//TODO OS-independant
static const std::string sTempName = "/tmp/kissme";

std::string get_album_art(std::string sAudioFile)
{
    TagLib::String fileName = sAudioFile;
    TagLib::String fileType = fileName.substr(fileName.size() - 3).upper();

    std::string sFilename = NO_IMAGE;//sTempName;

    if (fileType == "M4A")
    {
        //TODO
    }
    else if (fileType == "MP3")
    {
        TagLib::MPEG::File audioFile(sAudioFile.c_str());

        TagLib::ID3v2::Tag *tag = audioFile.ID3v2Tag(true);

        TagLib::ID3v2::FrameList frames = tag->frameList("APIC");

        if(!frames.isEmpty())
        {
            TagLib::ID3v2::AttachedPictureFrame *frame = static_cast<TagLib::ID3v2::AttachedPictureFrame *>(frames.front());
            if(frame->mimeType() == "image/png")
                sFilename = sTempName + ".png";
            else
                sFilename = sTempName + ".jpg";
            FILE* fp = fopen(sFilename.c_str(), "wb");
            fwrite(frame->picture().data(), 1, frame->picture().size(), fp);
            fclose(fp);
        }
    }
    else if (fileType == "OGG")
    {
        TagLib::Ogg::Vorbis::File audioFile(sAudioFile.c_str());
        TagLib::Ogg::XiphComment *tag = audioFile.tag();

        if(tag->contains("METADATA_BLOCK_PICTURE"))
        {
            std::cout << "Found METADATA_BLOCK_PICTURE in Ogg file" << std::endl;
            TagLib::StringList sl = tag->fieldListMap()[ "METADATA_BLOCK_PICTURE" ];
            if(!sl.size())
            {
                std::cout << "Empty METADATA_BLOCK_PICTURE" << std::endl;
                return sFilename;
            }
            unsigned char* buf = (unsigned char*)b64_decode(sl[0].toCString(true), sl[0].size());
            TagLib::ByteVector bv;
            bv.setData((const char*)buf, (uint)sl[0].size());
            TagLib::FLAC::Picture picture(bv);
            TagLib::ByteVector b = picture.data();
            if(picture.mimeType() == "image/png")
                sFilename = sTempName + ".png";
            else    //Assume JPEG unless specified otherwise
                sFilename = sTempName + ".jpg";
            FILE* fp = fopen(sFilename.c_str(), "wb");
            fwrite(b.data(), 1, b.size(), fp);
            fclose(fp);
        }
        else if(tag->contains("COVERART"))	//Don't bother decoding both if there are more than one
        {
            std::cout << "Found COVERART in Ogg file" << std::endl;
            TagLib::StringList sl = tag->fieldListMap()[ "COVERART" ];
            if(!sl.size())
            {
                std::cout << "Empty COVERART" << std::endl;
                return sFilename;
            }
            unsigned char* buf = (unsigned char*)b64_decode(sl[0].toCString(true), sl[0].size());
            TagLib::ByteVector bv;
            bv.setData((const char*)buf, (uint)sl[0].size());
            TagLib::FLAC::Picture picture(bv);
            TagLib::ByteVector b = picture.data();
            if(picture.mimeType() == "image/png")
                sFilename = sTempName + ".png";
            else
                sFilename = sTempName + ".jpg";
            FILE* fp = fopen(sFilename.c_str(), "wb");
            fwrite(b.data(), 1, b.size(), fp);
            fclose(fp);
        }
        else
        {
            std::cout << "No album art found in file " << sAudioFile << std::endl;
        }
    }
    else
    {
        std::cout << fileType << " is unsupported for album art." << std::endl;
    }

    return sFilename;
}

bool set_album_art(std::string sSong, std::string sImg)
{
    TagLib::String fileName = sSong;
    TagLib::String fileType = fileName.substr(fileName.size() - 3).upper();

    ImageFile imageFile(sImg.c_str());
    TagLib::ByteVector imageData = imageFile.data();

    TagLib::String mimeType = sImg;
    mimeType = mimeType.substr(mimeType.size() - 3).upper();
    if(mimeType == "JPG")
    	mimeType = "image/jpeg";
    else if(mimeType == "PNG")
    	mimeType = "image/png";
    else
    	std::cout << "Err: Unknown picture format: " << mimeType << std::endl;

    if (fileType == "M4A")
    {
      // read the image file
      TagLib::MP4::CoverArt coverArt((TagLib::MP4::CoverArt::Format) 0x0D, imageData);

      // read the mp4 file
      TagLib::MP4::File audioFile(sSong.c_str());

      // get the tag ptr
      TagLib::MP4::Tag *tag = audioFile.tag();

      // get the items map
      TagLib::MP4::ItemListMap itemsListMap = tag->itemListMap();

      // create cover art list
      TagLib::MP4::CoverArtList coverArtList;

      // append instance
      coverArtList.append(coverArt);

      // convert to item
      TagLib::MP4::Item coverItem(coverArtList);

      // add item to map
      itemsListMap.insert("covr", coverItem);

      tag->save();
    }
    else if (fileType == "MP3")
    {
      TagLib::MPEG::File audioFile(sSong.c_str());

      TagLib::ID3v2::Tag *tag = audioFile.ID3v2Tag(true);
      TagLib::ID3v2::AttachedPictureFrame *frame = new TagLib::ID3v2::AttachedPictureFrame;

      frame->setMimeType(mimeType);
      frame->setPicture(imageData);

      tag->addFrame(frame);
      audioFile.save();
    }
    else if (fileType == "OGG")
    {
      TagLib::Ogg::Vorbis::File audioFile(sSong.c_str());
      TagLib::Ogg::XiphComment *tag = audioFile.tag();

      //PROPOSED http://wiki.xiph.org/VorbisComment#METADATA_BLOCK_PICTURE
      //-----------
      TagLib::FLAC::Picture* picture = new TagLib::FLAC::Picture();
      picture->setData(imageData);
      picture->setType((TagLib::FLAC::Picture::Type)  0x03); // FrontCover
      picture->setMimeType(mimeType);
      picture->setDescription("Front Cover");

      TagLib::ByteVector block = picture->render();
      tag->addField("METADATA_BLOCK_PICTURE", b64_encode(block.data(), block.size()), true);

      //-----------
      //UNOFFICIAL DEPRECATED http://wiki.xiph.org/VorbisComment#Unofficial_COVERART_field_.28deprecated.29
      //-----------
      //block = imageData;

      //tag->addField("COVERART",  b64_encode(block.data(), block.size()), true);
      //-----------

      audioFile.save();
    }
    else
    {
       std::cout << fileType << " is unsupported." << std::endl;
       return false;
    }

    return true;
}
