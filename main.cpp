#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <sstream>
using namespace std;

int main(int argc, char** argv)
{
	// start SDL with audio support
	if(SDL_Init(SDL_INIT_AUDIO)==-1) 
	{
		cout << "SDL_Init: " << SDL_GetError() << endl;
		exit(1);
	}

	// open 44.1KHz, signed 16bit, system byte order,
	//      stereo audio, using 1024 byte chunks
	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096)==-1) 
	{
		cout << "Mix_OpenAudio: " << Mix_GetError() << endl;
		exit(1);
	}

	//SDL_Surface* screen = SDL_SetVideoMode(800, 600, 0, 0);

	atexit(SDL_Quit);
	atexit(Mix_CloseAudio);

	// load the MP3 file "music.mp3" to play as music
	Mix_Music *music;
	string filename = "output.mp3";
	if(argc > 1)
		filename = argv[1];
	music=Mix_LoadMUS(filename.c_str());
	if(!music) 
	{
		cout << "Mix_LoadMUS(\"music.mp3\"): " << Mix_GetError() << endl;
		exit(1);
	}
	else
	{
		//Mix_HookMusicFinished(done);
		if(Mix_PlayMusic(music, 0)==-1) 
		{
    	cout << "Mix_PlayMusic: " << Mix_GetError() << endl;
    	exit(1);
		}
		if(!Mix_PlayingMusic())	//MP3 not in right format
		{
			cout << "err" << endl;
			return 0;
			/*string s = "./bin/lame " + filename + " " + filename + ".mp3";
			//ostringstream oss(s);
			//oss << filename << " " << filename << ".mp3" << endl;
			cout << s << endl;
			system(s.c_str());
			
			Mix_FreeMusic(music);
			music=Mix_LoadMUS((filename + ".mp3").c_str());
			cout << "Playing " << endl;
			Mix_PlayMusic(music, 0);*/
		}
	}
	
	SDL_Event event;
	
	while(true) 
  {
    while(SDL_PollEvent(&event)) 
    {
      switch(event.type) 
      {
      	case SDL_QUIT:
					return 0;
      }
    }

    // So we don't hog the CPU 
    SDL_Delay(50);

  }

	Mix_FreeMusic(music);

	return 0;
}




