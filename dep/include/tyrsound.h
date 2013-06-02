#ifndef TYRSOUND_H
#define TYRSOUND_H

#include <stdlib.h>

#if defined(_MSC_VER)
    typedef __int64 tyrsound_int64;
    typedef unsigned __int64 tyrsound_uint64;
#else
#   include <stdint.h>
    typedef int64_t tyrsound_int64;
    typedef uint64_t tyrsound_uint64;
#endif

#ifdef _WIN32
#  define TYRSOUND_DLL_EXPORT __declspec(dllexport)
#else
#  define TYRSOUND_DLL_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct tyrsound_Format
{
    unsigned int hz;
    unsigned int sampleBits;
    unsigned int channels;
    unsigned int bufferSize;
    unsigned int bigendian;
    unsigned int signedSamples;
    unsigned int numBuffers;
};

typedef unsigned int tyrsound_Handle;
#define TYRSOUND_NULLHANDLE 0

struct tyrsound_Stream
{
    /* User-specified stream pointer */
    void *user;

    /* Function to get more bytes from the pointer.
     * Same semantics as fread(). NULL for write-only streams. */
    tyrsound_uint64 (*read)(void *dst, tyrsound_uint64 size, tyrsound_uint64 count, void *user);

    /* Seek function. Same semantics as fseek(). Seeks both read and write positions.
     * Can be NULL if stream is unseekable. */
    int (*seek)(void *user, tyrsound_uint64 offset, int whence);

    /* Stream poisition query function. Same semantics as ftell(). Can be NULL if unknown. */
    tyrsound_uint64 (*tell)(void *user);

    /* Closes the stream; will be called when the stream is no longer needed.
       Can be NULL if stream can not (or should not) be closed. */
    int (*close)(void *user);

    /* Writes data to the stream. Same semantics as fwrite(). NULL for read-only streams. */
    tyrsound_uint64 (*write)(const void *src, tyrsound_uint64 size, tyrsound_uint64 count, void *user);

    /* Flushes the stream. Same semantics as fflush(). Can be NULL if not required. */
    int (*flush)(void *user);
};

/* Error values */
enum tyrsound_Error
{
    TYRSOUND_ERR_OK                    = 0, /* No error */

    /* > 0: warnings */
    TYRSOUND_ERR_PARAMS_ADJUSTED       = 1, /* NOT YET USED */

    /* < 0: errors */
    TYRSOUND_ERR_UNSPECIFIED           = -1, /* Generic error */
    TYRSOUND_ERR_INVALID_HANDLE        = -2, /* Inavlid handle passed to function (TYRSOUND_NULLHANDLE is always invalid) */
    TYRSOUND_ERR_INVALID_VALUE         = -3, /* Parameter error */
    TYRSOUND_ERR_UNSUPPORTED           = -4, /* Action not suported by device / Stream format not recognized */
    TYRSOUND_ERR_NO_DEVICE             = -5, /* No device was found */
    TYRSOUND_ERR_SHIT_HAPPENED         = -6, /* Internal error */
    TYRSOUND_ERR_OUT_OF_MEMORY         = -7, /* Allocator returned NULL */
    TYRSOUND_ERR_UNSUPPORTED_FORMAT    = -8, /* The passed tyrsound_Format swas not suitable to complete the action */
    TYRSOUND_ERR_NOT_READY             = -9, /* Action can't be done right now (but possibly later) */
};


/********************
* Function pointers *
********************/


typedef tyrsound_Error (*tyrsound_positionCallback)(tyrsound_Handle, float position, void *user);

/* Generic memory allocation function, following the same semantics as realloc:
   * (ptr, 0) -> delete
   * (NULL, size) -> allocate size bytes
   * (ptr, size) -> reallocate
*/
typedef void *(*tyrsound_Alloc)(void *ptr, size_t size, void *user);


/* Startup the sound system.
 * fmt: Sets the format that should be used by tyrsound_init().
 *      Pass NULL to use default parameters (might not work).
 * output: Space-separated list of output devices to try, in that order.
           If "" or NULL is passed, try a default output device
           (Note: Currently only "openal" is supported) */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_init(const tyrsound_Format *fmt, const char *output);

/* Shuts down the sound system and resets the internal state.
 * Clears everything including the the custom allocator,
 * but NOT the custom mutex and related functions. */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_shutdown(void);

/* Sets up the library for multithreading,
 * allowing to call tyrsound_update() from one or more separate threads.
 *   mutex: Opaque pointer to a mutex that can be locked/unlocked at all times.
            The pointer must stay alive until tyrsound_shutdown() is called,
            or NULL is passed to this function.
            The mutex is never locked recursively.
 *   lockFunc: Function pointer that the mutex pointer is passed to.
               Expected to return 0 if locking the mutex failed
               (causing any action that triggered the call to fail),
               any other value to indicate success.
 *   unlockFunc: Function pointer that unlocks the mutex. Expected not to fail.
 */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_setUpdateMutex(void *mutex,
                                                           int (*lockFunc)(void*),
                                                           void (*unlockFunc)(void*));

/* Expected to be called in the main loop, or at least often enough
   to prevent buffer underrun and subsequent playback starving.
   Triggers callbacks.
   Can be called from one or more separate threads if an update mutex
   has been set. */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_update(void);


/* Fills a tyrsound_Format struct with the actually used parameters.
   Do not call this before a successful tyrsound_init() ! */
TYRSOUND_DLL_EXPORT void tyrsound_getFormat(tyrsound_Format *fmt);

/* Set a custom memory allocation function following the same semantics as realloc().
   See tyrsound_Alloc description. Passing NULL uses the default allocator (realloc()).*/
TYRSOUND_DLL_EXPORT void tyrsound_setAlloc(tyrsound_Alloc allocFunc, void *user);

/*****************************
* Sound creation/destruction *
*****************************/

/* Load a sound using a stream loader. Returns TYRSOUND_NULLHANDLE on failure.
 * The optional format parameter may be set to the desired output format;
 * if it is NULL, use the format currently used by the output device. */
TYRSOUND_DLL_EXPORT tyrsound_Handle tyrsound_load(tyrsound_Stream stream, const tyrsound_Format *fmt);

/* Stops a sound, and frees all related resources. */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_unload(tyrsound_Handle);


/**********************
 * Sound manipulation *
 *********************/

/* Starts playing a sound or unpauses a paused sound */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_play(tyrsound_Handle);

/* Pauses a playing sound. Pausing multiple times has no effect and does not fail. */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_pause(tyrsound_Handle);

/* Stops a sound. Subsequent tyrsound_play() will start from the beginning */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_stop(tyrsound_Handle);

/* Returns 1 when a sound is currently playing, i.e. not stopped and not paused. */
TYRSOUND_DLL_EXPORT int tyrsound_isPlaying(tyrsound_Handle);

/* Returns the current playback position in seconds. -1 if unknown. */
TYRSOUND_DLL_EXPORT float tyrsound_getPlayPosition(tyrsound_Handle);

/* Sets volume. 0 = silent, 1 = normal, > 1: louder than normal */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_setVolume(tyrsound_Handle, float);

/* Sets speed. (0, 1) slows down, 1 is normal, > 1 is faster. */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_setSpeed(tyrsound_Handle, float);

/* Sets sound world position in (x, y, z)-coordinates. */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_setPosition(tyrsound_Handle, float x, float y, float z);

/* Returns the total play time in seconds. < 0 if unknown. */
TYRSOUND_DLL_EXPORT float tyrsound_getLength(tyrsound_Handle);

/* Seeks to a position in the stream (in seconds). */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_seek(tyrsound_Handle, float seconds);

/* When the decoder hits stream EOF, seek back to position
 *    seconds: -1 to disable looping, any value >= 0 to seek to.
 *    loops: How often to loop. 0 disables looping (plays exactly once),
             1 repeats once (= plays 2 times, etc). -1 to loop infinitely. */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_setLoop(tyrsound_Handle, float seconds, int loops);

/************************
* Listener manipulation *
************************/

/* Sets listener world position in (x, y, z)-coordinates. */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_setListenerPosition(float x, float y, float z);

/* Set master volume. 0 = silent, 1 = normal, > 1: louder than normal. */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_setMasterVolume(float);

/* Set master speed, affecting all sound sources currently played.
 * (0, 1) slows down, 1 is normal, > 1 is faster. */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_setMasterSpeed(float);


/* Sets a callback that fires whenever a certain position is reached while playing.
   The function can be called any time (even from within a callback) to set a new callback.
   Use any number < 0 to trigger when the stream has finished playing.
   The actual position reached when the callback fires may be slightly off,
   therefore the current position is passed to the callback as well (-1 if end of stream).
   Only one callback can be set at a time.
   Note: Keep in mind that the callback will be triggered from the thread
         that calls tyrsound_update() ! */
/*TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_setPositionCallback(tyrsound_Handle,
                                                        tyrsound_positionCallback func,
                                                        float position,
                                                        void *user);
*/

/********************
* Helper functions  *
********************/

/* Decodes data from one stream and writes the result to the 2nd stream.
 * If dstfmt is not NULL, write format info to it.
 * The optional dstfmt parameter may be set to the desired output format;
 * if it is NULL, use the format currently used by the output device. */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_decodeStream(tyrsound_Stream dst,
                                                         tyrsound_Format *dstfmt,
                                                         tyrsound_Stream src,
                                                         tyrsound_Format *srcfmt);

/* Create stream from raw memory.
 *   closeFunc can be set to any function that will be called when the
 *   stream is no longer needed. Ignored if NULL. */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_createMemStream(tyrsound_Stream*,
                                                            void *ptr,
                                                            size_t size,
                                                            void (*closeFunc)(void *opaque));

/* Create stream from FILE* */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_createFileStream(tyrsound_Stream*,
                                                             void *fh, // FILE* here
                                                             int closeWhenDone);

/* Create stream from filename */
TYRSOUND_DLL_EXPORT tyrsound_Error tyrsound_createFileNameStream(tyrsound_Stream*,
                                                                 const char *filename,
                                                                 const char *mode);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif
