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
#include <stdio.h>
#include <stdint.h>

static const std::string sTempName = "/tmp/kissme";

std::string get_album_art(std::string sAudioFile)
{
    TagLib::String fileName = sAudioFile;
    TagLib::String fileType = fileName.substr(fileName.size() - 3).upper();

    std::string sFilename = "logo.png";//sTempName;

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
            if(frame->mimeType() == "image/jpeg")
                sFilename = sTempName + ".jpg";
            else
                sFilename = sTempName + ".png";
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
            if(picture.mimeType() == "image/jpeg")
                sFilename = sTempName + ".jpg";
            else
                sFilename = sTempName + ".png";
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
            if(picture.mimeType() == "image/jpeg")
                sFilename = sTempName + ".jpg";
            else
                sFilename = sTempName + ".png";
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
        std::cout << fileType << " is unsupported." << std::endl;
    }

    return sFilename;
}
