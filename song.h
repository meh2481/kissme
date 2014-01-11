#ifndef SONG_H
#define SONG_H
#include <string>

typedef struct
{
	std::string filename;
	std::string title;
	std::string artist;
	std::string album;
	unsigned int track;
	int length;
	bool error;
} song; 

#endif
