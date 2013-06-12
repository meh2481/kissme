/*
Command-line application for embedding JPEG cover art in MP3, MP4 and OGG audio files.

Based on taglib-1.7 (debian libtag1-dev)
and base64encoding (http://code.google.com/p/base64encoding/) needed for OGG Vorbis embedded cover art
which uses libssl-dev

gcc -g base64.c -lssl -c
g++ -g -ggdb -I./dep/include -L./dep/lib setcover.cpp -o setcover base64.o -ltag -lcrypto

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

using namespace TagLib;
using namespace std;
unsigned int toUint(unsigned char* data)
{
	uint32_t i = ((uint32_t)data[0] << 12) + 
							 ((uint32_t)data[1] << 8) + 
							 ((uint32_t)data[2] << 4) + 
							 ((uint32_t)data[3]);
	return i;
}

bool myparse(unsigned char* data, string sOutFilename)
{
	TagLib::FLAC::Picture* d = new TagLib::FLAC::Picture();
  /*if(data.size() < 32) {
    printf("A picture block must contain at least 5 bytes.");
    return false;
  }*/

  uint pos = 0;
  cout << "Type: " << toUint(&data[pos]) << endl;
  pos += 4;
  uint mimeTypeLength = toUint(&data[pos]);
  pos += 4;
  /*if(pos + mimeTypeLength + 24 > data.size()) {
    printf("Invalid picture block.");
    return false;
  }*/
  string s = (const char*)&(data[pos]);
  s = s.substr(0, mimeTypeLength);
  cout << "mimeType: " << s << endl;
  if(s == "image/png")
  {
  	cout << "PNG image" << endl;
  	sOutFilename += ".png";
  }
  else if(s == "image/jpeg")
  {
  	cout << "JPEG image" << endl;
  	sOutFilename += ".jpg";
  }
  else
  {
  	cout << "Unknown image type: " << s << endl;
  }
  pos += mimeTypeLength;
  uint descriptionLength = toUint(&data[pos]);
  pos += 4;
  /*if(pos + descriptionLength + 20 > data.size()) {
    printf("Invalid picture block.");
    return false;
  }*/
  s = (const char*)&(data[pos]);
  s = s.substr(0, descriptionLength);
  cout << "description: " << s << endl;
  pos += descriptionLength;
  uint32_t width = toUint(&data[pos]);
  pos += 4;
  uint32_t height = toUint(&data[pos]);
  pos += 4;
  uint32_t colordepth = toUint(&data[pos]);
  pos += 4;
  uint32_t numcolors = toUint(&data[pos]);
  pos += 4;
  uint dataLength = toUint(&data[pos]);
  pos += 4;
  
  cout << "img: " << width << "x" << height << ", " << numcolors << ", depth: " << colordepth << ", length: " << dataLength << endl;
  /*if(pos + dataLength > data.size()) {
    printf("Invalid picture block.");
    return false;
  }*/
  //d->data = data.mid(pos, dataLength);*/
	
	FILE* fp = fopen(sOutFilename.c_str(), "wb");
	fwrite(&data[pos], 1, dataLength*14, fp);
	fclose(fp);
	
  return true;
}

int main(int argc, char *argv[])
{
    if (argc != 3) 
    {
        std::cout << "Usage: setcover <mp3|m4a|ogg> cover.jpg" << std::endl;
        return 1;
    }

    TagLib::String fileName = argv[1];
    TagLib::String fileType = fileName.substr(fileName.size() - 3).upper();
    
    ImageFile imageFile(argv[2]);
    TagLib::ByteVector imageData = imageFile.data();
    
    if (fileType == "M4A")
    {
      // read the image file
      TagLib::MP4::CoverArt coverArt((TagLib::MP4::CoverArt::Format) 0x0D, imageData);
      
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
      
      tag->save();
    }
    else if (fileType == "MP3")
    {
      TagLib::MPEG::File audioFile(argv[1]);

      TagLib::ID3v2::Tag *tag = audioFile.ID3v2Tag(true);
      TagLib::ID3v2::AttachedPictureFrame *frame = new TagLib::ID3v2::AttachedPictureFrame;

      //frame->setMimeType("image/jpeg");
      //frame->setPicture(imageData);

      //tag->addFrame(frame);
      //audioFile.save();      
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
      	std::string buffer = sl[0].toCString(true);
      	unsigned char* buf = (unsigned char*)b64_decode(buffer.c_str(), buffer.size());
      	buf[0] = 'h';
      	TagLib::ByteVector bv;
      	bv.fromCString((const char*)buf, (uint)buffer.size());
      	std::cout << "Size of buffer: " << buffer.size() << std::endl;
      	std::cout << "Size of bv: " << bv.size() << std::endl;
      	//std::cout << "bv Data: " << bv.data() << std::endl << std::endl;
      	TagLib::FLAC::Picture picture(bv);
      	myparse(buf, "test");
        picture.setData(bv);
        TagLib::ByteVector b = picture.data();
        std::cout << "Size of picture.data(): " << b.size() << std::endl;
      	FILE* fp = fopen("out.jpg", "wb");
      	fwrite(bv.data(), 1, bv.size(), fp);
      	fclose(fp);
      	fp = fopen("outbuf.jpg", "wb");
      	fwrite(buf, 1, buffer.size(), fp);
      	fclose(fp);
      }
    	if(tag->contains("COVERART"))
      {
      	std::cout << "Found COVERART in Ogg file" << std::endl;
      	TagLib::StringList sl = tag->fieldListMap()[ "COVERART" ];
      	if(!sl.size())
      	{
      		std::cout << "Empty COVERART" << std::endl;
      		return 1;
      	}
      }
        
      
      /*
      PROPOSED http://wiki.xiph.org/VorbisComment#METADATA_BLOCK_PICTURE
      
      TagLib::FLAC::Picture* picture = new TagLib::FLAC::Picture();
      picture->setData(imageData);
      picture->setType((TagLib::FLAC::Picture::Type)  0x03); // FrontCover
      picture->setMimeType("image/jpeg");
      picture->setDescription("Front Cover");
      
      TagLib::ByteVector block = picture->render();
      tag->addField("METADATA_BLOCK_PICTURE", b64_encode(block.data(), block.size()), true);
      
      
      /*
      UNOFFICIAL DEPRECATED http://wiki.xiph.org/VorbisComment#Unofficial_COVERART_field_.28deprecated.29
      
      block = imageData;
      
      tag->addField("COVERART",  b64_encode(block.data(), block.size()), true);*/
      
      //audioFile.save();
    }
    else
    {
       std::cout << fileType << " is unsupported." << std::endl;
    }
}
