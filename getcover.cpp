/*
Command-line application for embedding JPEG cover art in MP3, MP4 and OGG audio files.

Based on taglib-1.7 (debian libtag1-dev)
and base64encoding (http://code.google.com/p/base64encoding/) needed for OGG Vorbis embedded cover art
which uses libssl-dev

gcc -g base64.c -lssl -c
g++ -g -ggdb -I./dep/include -L./dep/lib getcover.cpp -o getcover base64.o -ltag -lcrypto

*/

/* Copyright (C) 2004 Scott Wheeler <wheeler@kde.org> ImageFile and MP3 example
 * Copyright (C) 2011 Guy McArthur <guymac@sourcecurve.com> MP4 and OGG examples
 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <mpegfile.h>
#include <attachedpictureframe.h>
#include <id3v2tag.h>
#include <mp4file.h>
#include <mp4tag.h>
#include <mp4coverart.h>
#include <vorbisfile.h>
#include <xiphcomment.h>
#include <flacpicture.h>
#include <stdio.h>
#include <stdint.h>

#include <iostream>

extern "C"
{
#include "base64.h"
}

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

#include <string.h>
int main(int argc, char *argv[])
{
    if (argc != 3) 
    {
        std::cout << "Usage: setcover <mp3|m4a|ogg> [output filename]" << std::endl;
        return 1;
    }

    TagLib::String fileName = argv[1];
    TagLib::String fileType = fileName.substr(fileName.size() - 3).upper();
    
    //ImageFile imageFile(argv[2]);
    //TagLib::ByteVector imageData = imageFile.data();
    
    if (fileType == "M4A")
    {
    	//TODO
      // read the image file
      /*TagLib::MP4::CoverArt coverArt((TagLib::MP4::CoverArt::Format) 0x0D, imageData);
      
      // read the mp4 file
      TagLib::MP4::File audioFile(argv[1]);
      
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
      
      tag->save();*/
    }
    else if (fileType == "MP3")
    {
    	TagLib::MPEG::File audioFile(argv[1]);

      TagLib::ID3v2::Tag *tag = audioFile.ID3v2Tag(true);

      TagLib::ID3v2::FrameList frames = tag->frameList("0PIC");

      if(!frames.isEmpty())
      {
          TagLib::ID3v2::AttachedPictureFrame *frame = static_cast<TagLib::ID3v2::AttachedPictureFrame *>(frames.front());
          if(frame->mimeType() == "image/jpeg")
              fileName += ".jpg";
          else
              fileName += ".png";
          FILE* fp = fopen(fileName.toCString(), "wb");
          fwrite(frame->picture().data(), 1, frame->picture().size(), fp);
          fclose(fp);
      }
      else
      	std::cout << "No MP3 image frame " << std::endl;
    }
    else if (fileType == "OGG")
    {
      TagLib::Ogg::Vorbis::File audioFile(argv[1]);
      TagLib::Ogg::XiphComment *tag = audioFile.tag();
      
      if(tag->contains("METADATA_BLOCK_PICTURE"))
      {
      	std::cout << "Found METADATA_BLOCK_PICTURE in Ogg file" << std::endl;
      	TagLib::StringList sl = tag->fieldListMap()[ "METADATA_BLOCK_PICTURE" ];
      	if(!sl.size())
      	{
      		std::cout << "Empty METADATA_BLOCK_PICTURE" << std::endl;
      		return 1;
      	}
      	unsigned char* buf = (unsigned char*)b64_decode(sl[0].toCString(true), sl[0].size());
      	TagLib::ByteVector bv;
      	bv.setData((const char*)buf, (uint)sl[0].size());
      	TagLib::FLAC::Picture picture(bv);
        TagLib::ByteVector b = picture.data();
        std::string sFilename = argv[2];
        if(picture.mimeType() == "image/jpeg")
        	sFilename += ".jpg";
        else
        	sFilename += ".png";
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
      		return 1;
      	}
      	unsigned char* buf = (unsigned char*)b64_decode(sl[0].toCString(true), sl[0].size());
      	TagLib::ByteVector bv;
      	bv.setData((const char*)buf, (uint)sl[0].size());
      	TagLib::FLAC::Picture picture(bv);
        TagLib::ByteVector b = picture.data();
      	std::string sFilename = argv[2];
        if(picture.mimeType() == "image/jpeg")
        	sFilename += ".jpg";
        else
        	sFilename += ".png";
      	FILE* fp = fopen(sFilename.c_str(), "wb");
      	fwrite(b.data(), 1, b.size(), fp);
      	fclose(fp);
      }
      else
      {
      	std::cout << "No album art found in file " << argv[0] << std::endl;
      }
    }
    else
    {
       std::cout << fileType << " is unsupported." << std::endl;
    }
}
