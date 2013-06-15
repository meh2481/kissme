#ifndef COVER_H
#define COVER_H

#include <string>
#include "base64.h"

#define NO_IMAGE "logo.png"

//#define BOTCHED_TAGGING

std::string get_album_art(std::string sFilename);    //Find the album art for a particular song (save to temp location)
bool set_album_art(std::string sSong, std::string sImg);    //Set the album art for the given song (returns true on success, false on error)























#endif
