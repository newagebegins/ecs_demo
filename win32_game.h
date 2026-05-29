#ifndef WIN32_GAME_H
#define WIN32_GAME_H

#define WIN32_MAX_FILE_PATH_LENGTH MAX_PATH

struct win32_backbuffer
{
    int32 Width;
    int32 Height;
    int32 Pitch;
    void *Memory;
    BITMAPINFO Info;
};

struct win32_game
{
    FILETIME LastWriteTime;
    HMODULE DLL;
    game_update_and_render *UpdateAndRender;
};

#if GAME_INTERNAL
enum input_recorder_state
{
    InputRecorderState_Off,
    InputRecorderState_Recording,
    InputRecorderState_Playing,
};
#endif

#endif
