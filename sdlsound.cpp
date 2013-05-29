#include "sdlsound.h"
#include <iostream>
using namespace std;


static volatile playsound_global_state global_state;

void init_sdl()
{

    if (SDL_Init(SDL_INIT_AUDIO) == -1)
    {
        cout << "SDL_Init() failed! reason: " << SDL_GetError() << endl;
        exit(1);
    }

    if (!Sound_Init())
    {
        cout << "Sound_Init() failed! reason: " << Sound_GetError() << endl;
        SDL_Quit();
        exit(1);
    }

    memset((void *) &global_state, '\0', sizeof (global_state));
    global_state.volume = 1.0;
    global_state.bytes_before_next_seek = -1;
    //if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096)==-1)
    //{
    //    cout << "Mix_OpenAudio: " << Mix_GetError() << endl;
    //    exit(1);
    //}

	atexit(SDL_Quit);
	//atexit(Mix_CloseAudio);
}


static Uint32 cvtMsToBytePos(Sound_AudioInfo *info, Uint32 ms)
{
    /* "frames" == "sample frames" */
    float frames_per_ms = ((float) info->rate) / 1000.0;
    Uint32 frame_offset = (Uint32) (frames_per_ms * ((float) ms));
    Uint32 frame_size = (Uint32) ((info->format & 0xFF) / 8) * info->channels;
    return(frame_offset * frame_size);
} /* cvtMsToBytePos */

int done_flag = 0;
static void do_seek(Sound_Sample *sample)
{
    Uint32 *seek_list = global_state.seek_list;
    Uint32 seek_index = global_state.seek_index;
    Uint32 total_seeks = global_state.total_seeks;

    fprintf(stdout, "Seeking to %.2d:%.2d:%.4d...\n",
            (int) ((seek_list[seek_index] / 1000) / 60),
            (int) ((seek_list[seek_index] / 1000) % 60),
            (int) ((seek_list[seek_index] % 1000)));


        if (!Sound_Seek(sample, seek_list[seek_index]))
        {
            fprintf(stderr, "Sound_Seek() failed: %s\n", Sound_GetError());
            done_flag = 1;
        } /* if */

    seek_index++;
    if (seek_index >= total_seeks)
        global_state.bytes_before_next_seek = -1;  /* no more seeks. */
    else
    {
        global_state.bytes_before_next_seek = cvtMsToBytePos(&sample->desired,
                                                        seek_list[seek_index]);
        seek_index++;
    } /* else */

    global_state.seek_index = seek_index;
} /* do_seek */


/*
 * This updates (decoded_bytes) and (decoded_ptr) with more audio data,
 *  taking into account potential looping, seeking and predecoding.
 */
static int read_more_data(Sound_Sample *sample)
{
    if (done_flag)              /* probably a sigint; stop trying to read. */
    {
        global_state.decoded_bytes = 0;
        return(0);
    } /* if */

    if ((global_state.bytes_before_next_seek >= 0) &&
        (global_state.decoded_bytes > global_state.bytes_before_next_seek))
    {
        global_state.decoded_bytes = global_state.bytes_before_next_seek;
    } /* if */

    if (global_state.decoded_bytes > 0) /* don't need more data; just return. */
        return(global_state.decoded_bytes);

        /* Need more audio data. See if we're supposed to seek... */
    if ((global_state.bytes_before_next_seek == 0) &&
        (global_state.seek_index < global_state.total_seeks))
    {
        do_seek(sample);  /* do it, baby! */
        return(read_more_data(sample));  /* handle loops conditions. */
    } /* if */

        /* See if there's more to be read... */
    if ( (global_state.bytes_before_next_seek != 0) &&
         (!(sample->flags & (SOUND_SAMPLEFLAG_ERROR | SOUND_SAMPLEFLAG_EOF))) )
    {
        global_state.decoded_bytes = Sound_Decode(sample);
        if (sample->flags & SOUND_SAMPLEFLAG_ERROR)
        {
            fprintf(stderr, "Error in decoding sound file!\n"
                            "  reason: [%s].\n", Sound_GetError());
        } /* if */

        global_state.decoded_ptr = (Uint8*)sample->buffer;
        return(read_more_data(sample));  /* handle loops conditions. */
    } /* if */

    /* No more to be read from stream, but we may want to loop the sample. */

    if (!global_state.looping)
        return(0);

    global_state.looping--;

    global_state.seek_index = 0;
    global_state.bytes_before_next_seek =
        (global_state.total_seeks > 0) ? 0 : -1;

    /* we just need to point predecoded samples to the start of the buffer. */
    if (global_state.predecode)
    {
        global_state.decoded_bytes = sample->buffer_size;
        global_state.decoded_ptr = (Uint8*)sample->buffer;
    } /* if */
    else
    {
        Sound_Rewind(sample);  /* error is checked in recursion. */
    } /* else */

    return(read_more_data(sample));
} /* read_more_data */


static void memcpy_with_volume(Sound_Sample *sample,
                               Uint8 *dst, Uint8 *src, int len)
{
    int i;
    Uint16 *u16src = NULL;
    Uint16 *u16dst = NULL;
    Sint16 *s16src = NULL;
    Sint16 *s16dst = NULL;
    float volume = global_state.volume;

    if (!global_state.wants_volume_change)
    {
        memcpy(dst, src, len);
        return;
    } /* if */

    /* !!! FIXME: This would be more efficient with a lookup table. */
    switch (sample->desired.format)
    {
        case AUDIO_U8:
            for (i = 0; i < len; i++, src++, dst++)
                *dst = (Uint8) (((float) (*src)) * volume);
            break;

        case AUDIO_S8:
            for (i = 0; i < len; i++, src++, dst++)
                *dst = (Sint8) (((float) (*src)) * volume);
            break;

        case AUDIO_U16LSB:
            u16src = (Uint16 *) src;
            u16dst = (Uint16 *) dst;
            for (i = 0; i < len; i += sizeof (Uint16), u16src++, u16dst++)
            {
                *u16dst = (Uint16) (((float) (SDL_SwapLE16(*u16src))) * volume);
                *u16dst = SDL_SwapLE16(*u16dst);
            } /* for */
            break;

        case AUDIO_S16LSB:
            s16src = (Sint16 *) src;
            s16dst = (Sint16 *) dst;
            for (i = 0; i < len; i += sizeof (Sint16), s16src++, s16dst++)
            {
                *s16dst = (Sint16) (((float) (SDL_SwapLE16(*s16src))) * volume);
                *s16dst = SDL_SwapLE16(*s16dst);
            } /* for */
            break;

        case AUDIO_U16MSB:
            u16src = (Uint16 *) src;
            u16dst = (Uint16 *) dst;
            for (i = 0; i < len; i += sizeof (Uint16), u16src++, u16dst++)
            {
                *u16dst = (Uint16) (((float) (SDL_SwapBE16(*u16src))) * volume);
                *u16dst = SDL_SwapBE16(*u16dst);
            } /* for */
            break;

        case AUDIO_S16MSB:
            s16src = (Sint16 *) src;
            s16dst = (Sint16 *) dst;
            for (i = 0; i < len; i += sizeof (Sint16), s16src++, s16dst++)
            {
                *s16dst = (Sint16) (((float) (SDL_SwapBE16(*s16src))) * volume);
                *s16dst = SDL_SwapBE16(*s16dst);
            } /* for */
            break;
    } /* switch */
} /* memcpy_with_volume */


static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    Sound_Sample *sample = (Sound_Sample *) userdata;
    int bw = 0; // bytes written to stream this time through the callback

    while (bw < len)
    {
        int cpysize;  // bytes to copy on this iteration of the loop.

        if (!read_more_data(sample)) // read more data, if needed.
        {
            // ...there isn't any more data to read!
            memset(stream + bw, '\0', len - bw);
            done_flag = 1;
            return;
        }

        // decoded_bytes and decoder_ptr are updated as necessary...

        cpysize = len - bw;
        if (cpysize > global_state.decoded_bytes)
            cpysize = global_state.decoded_bytes;

        if (cpysize > 0)
        {
            memcpy_with_volume(sample, stream + bw,
                               (Uint8 *) global_state.decoded_ptr,
                               cpysize);

            bw += cpysize;
            global_state.decoded_ptr += cpysize;
            global_state.decoded_bytes -= cpysize;
            if (global_state.bytes_before_next_seek >= 0)
                global_state.bytes_before_next_seek -= cpysize;
        } // if
    } // while
} // audio_callback









#define DEFAULT_DECODEBUF 16384
#define DEFAULT_AUDIOBUF 4096
SDL_AudioSpec sdl_desired;
void play_song(string sFilename)
{
    static bool first = true;
    if(!first)
    {
        SDL_PauseAudio(1);
        SDL_CloseAudio();
    }
    first = false;
    memset(&sdl_desired, '\0', sizeof (SDL_AudioSpec));
    //if(g_channel != -1)
    //{
    //    Mix_Chunk* chunk =
    //}
    Sound_Sample* sample = Sound_NewSampleFromFile(sFilename.c_str(), NULL, DEFAULT_DECODEBUF);

    sdl_desired.freq = sample->actual.rate;
    sdl_desired.format = sample->actual.format;
    sdl_desired.channels = sample->actual.channels;
    sdl_desired.samples = DEFAULT_AUDIOBUF;
    sdl_desired.callback = audio_callback;
    sdl_desired.userdata = sample;


    if (SDL_OpenAudio(&sdl_desired, NULL) < 0)
    {
        fprintf(stderr, "Couldn't open audio device!\n"
                        "  reason: [%s].\n", SDL_GetError());
        Sound_Quit();
        SDL_Quit();
        //return(42);
    } /* if */

    cout << "Now playing " << sFilename << endl;


    SDL_PauseAudio(0);
}
