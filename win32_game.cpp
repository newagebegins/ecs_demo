#include <windows.h>
#include <stdio.h>

#include "platform.h"
#include "win32_game.h"

global_variable bool32 GlobalRunning;
global_variable win32_backbuffer GlobalBackbuffer;

struct win32_client_dim
{
    int32 X;
    int32 Y;
};

inline win32_client_dim
Win32GetClientDim(HWND Window)
{
    win32_client_dim Result;
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.X = ClientRect.right - ClientRect.left;
    Result.Y = ClientRect.bottom - ClientRect.top;
    return(Result);
}

internal void *
Win32ReadEntireFile(char *FilePath)
{
    HANDLE File = CreateFileA(FilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, 0);
    Assert(File != INVALID_HANDLE_VALUE);

    LARGE_INTEGER FileSizeLI;
    BOOL GetGetFileSizeExSuccess = GetFileSizeEx(File, &FileSizeLI);
    Assert(GetGetFileSizeExSuccess);
    DWORD FileSize = FileSizeLI.LowPart;
    Assert(FileSizeLI.HighPart == 0);

    void *FileContents = VirtualAlloc(0, FileSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(FileContents);

    DWORD BytesRead;
    BOOL ReadFileSuccess = ReadFile(File, FileContents, FileSize, &BytesRead, 0);
    Assert(ReadFileSuccess);
    Assert(BytesRead == FileSize);

    BOOL CloseHandleSuccess = CloseHandle(File);
    Assert(CloseHandleSuccess);

    return(FileContents);
}

inline LARGE_INTEGER
Win32GetCounter(void)
{
    LARGE_INTEGER Result;
    BOOL Success = QueryPerformanceCounter(&Result);
    Assert(Success);
    return(Result);
}

internal void
Win32DisplayBackbuffer(win32_backbuffer *Backbuffer, HDC DeviceContext,
                       int32 ClientWidth, int32 ClientHeight)
{
    int32 ScaleX = Maximum(ClientWidth / Backbuffer->Width, 1);
    int32 ScaleY = Maximum(ClientHeight / Backbuffer->Height, 1);
    int32 Scale = Minimum(ScaleX, ScaleY);

    int32 OutputWidth = Scale*Backbuffer->Width;
    int32 OutputHeight = Scale*Backbuffer->Height;

    int32 OffsetX = Maximum((ClientWidth - OutputWidth) / 2, 0);
    int32 OffsetY = Maximum((ClientHeight - OutputHeight) / 2, 0);

    DWORD BorderOp = BLACKNESS;
    // NOTE(slava): Left border
    PatBlt(DeviceContext, 0, 0, OffsetX, ClientHeight, BorderOp);
    // NOTE(slava): Right border
    PatBlt(DeviceContext, OffsetX + OutputWidth, 0, OffsetX, ClientHeight, BorderOp);
    // NOTE(slava): Top border
    PatBlt(DeviceContext, 0, 0, ClientWidth, OffsetY, BorderOp);
    // NOTE(slava): Bottom border
    PatBlt(DeviceContext, 0, OffsetY + OutputHeight, ClientWidth, OffsetY, BorderOp);

    StretchDIBits(DeviceContext,
                  OffsetX, OffsetY, OutputWidth, OutputHeight,
                  0, 0, Backbuffer->Width, Backbuffer->Height,
                  Backbuffer->Memory, &Backbuffer->Info, DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT
Win32WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_CLOSE:
        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            Assert(DeviceContext);
            win32_client_dim ClientDim = Win32GetClientDim(Window);
            Win32DisplayBackbuffer(&GlobalBackbuffer, DeviceContext, ClientDim.X, ClientDim.Y);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
}

internal FILETIME
Win32GetLastWriteTime(char *FilePath)
{
    FILETIME Result = {};
    WIN32_FILE_ATTRIBUTE_DATA FileAttributes;
    if(GetFileAttributesExA(FilePath, GetFileExInfoStandard, &FileAttributes))
    {
        Result = FileAttributes.ftLastWriteTime;
    }
    return(Result);
}

internal bool32
Win32FileExists(char *FilePath)
{
    WIN32_FILE_ATTRIBUTE_DATA Ignored;
    bool32 Result = GetFileAttributesExA(FilePath, GetFileExInfoStandard, &Ignored);
    return(Result);
}

internal void
ConcatenateStrings(char *Source0, uint32 Count0,
                   char *Source1, uint32 Count1,
                   char *Dest, uint32 DestMaxCount)
{
    uint32 DestIndex = 0;
    for(uint32 SourceIndex = 0;
        SourceIndex < Count0;
        ++SourceIndex, ++DestIndex)
    {
        Dest[DestIndex] = Source0[SourceIndex];
    }
    for(uint32 SourceIndex = 0;
        SourceIndex < Count1;
        ++SourceIndex, ++DestIndex)
    {
        Dest[DestIndex] = Source1[SourceIndex];
    }
    Assert(DestIndex <= DestMaxCount);
}

internal win32_game
Win32LoadGame(char *GameDLLFullPath, char *GameTempDLLFullPath)
{
    win32_game Result = {};

    BOOL CopyFileSuccess = CopyFile(GameDLLFullPath, GameTempDLLFullPath, false);
    Assert(CopyFileSuccess);

    Result.DLL = LoadLibraryA(GameTempDLLFullPath);
    Assert(Result.DLL);

    Result.LastWriteTime = Win32GetLastWriteTime(GameDLLFullPath);
    
    Result.UpdateAndRender = (game_update_and_render *)GetProcAddress(Result.DLL, "GameUpdateAndRender");
    Assert(Result.UpdateAndRender);

    return(Result);
}

inline void
Win32RewindFile(HANDLE File)
{
    DWORD SetFilePointerResult = SetFilePointer(File, 0, 0, FILE_BEGIN);
    Assert(SetFilePointerResult != INVALID_SET_FILE_POINTER);
}

#if GAME_INTERNAL
inline void
Win32SaveMemory(HANDLE InputRecorderFile, game_memory *Memory)
{
    DWORD NumberOfBytesWritten;
    BOOL WriteFileSuccess = WriteFile(InputRecorderFile,
                                      Memory->PermanentStorage,
                                      Memory->PermanentStorageSize,
                                      &NumberOfBytesWritten,
                                      0);
    Assert(WriteFileSuccess);
    Assert(NumberOfBytesWritten == Memory->PermanentStorageSize);
}

inline void
Win32LoadMemory(HANDLE InputRecorderFile, game_memory *Memory)
{
    DWORD NumberOfBytesRead;
    BOOL ReadFileSuccess = ReadFile(InputRecorderFile,
                                    Memory->PermanentStorage,
                                    Memory->PermanentStorageSize,
                                    &NumberOfBytesRead,
                                    0);
    Assert(ReadFileSuccess);
    Assert(NumberOfBytesRead == Memory->PermanentStorageSize);
}

inline void
Win32SaveInput(HANDLE InputRecorderFile, game_input *Input)
{
    DWORD NumberOfBytesWritten;
    BOOL WriteFileSuccess = WriteFile(InputRecorderFile,
                                      Input,
                                      sizeof(*Input),
                                      &NumberOfBytesWritten,
                                      0);
    Assert(WriteFileSuccess);
    Assert(NumberOfBytesWritten == sizeof(*Input));
}

inline bool32
Win32LoadInput(HANDLE InputRecorderFile, game_input *Input)
{
    DWORD NumberOfBytesRead;
    BOOL ReadFileSuccess = ReadFile(InputRecorderFile,
                                    Input,
                                    sizeof(*Input),
                                    &NumberOfBytesRead,
                                    0);
    Assert(ReadFileSuccess);
    bool32 ReachedEndOfFile = (NumberOfBytesRead == 0);
    if(!ReachedEndOfFile)
    {
        Assert(NumberOfBytesRead == sizeof(*Input));
    }
    return(ReachedEndOfFile);
}
#endif

internal void
DrawRect(game_bitmap *Bitmap, int32 MinX, int32 MinY, int32 MaxX, int32 MaxY,
         v4 Color = {1.0f, 1.0f, 1.0f, 1.0f})
{
    if(MinX < 0)
    {
        MinX = 0;
    }
    if(MinY < 0)
    {
        MinY = 0;
    }
    if(MaxX > Bitmap->Width)
    {
        MaxX = Bitmap->Width;
    }
    if(MaxY > Bitmap->Height)
    {
        MaxY = Bitmap->Height;
    }
    
    uint8 *DestRow = (uint8 *)Bitmap->Pixels + MinY*Bitmap->Pitch + MinX*BITMAP_BYTES_PER_PIXEL;

    for(int32 Y = MinY;
        Y < MaxY;
        ++Y)
    {
        uint32 *Dest = (uint32*)DestRow;
        for(int32 X = MinX;
            X < MaxX;
            ++X)
        {
            uint32 R = (uint32)(255.0f*Color.r + 0.5f);
            uint32 G = (uint32)(255.0f*Color.g + 0.5f);
            uint32 B = (uint32)(255.0f*Color.b + 0.5f);
            uint32 Color32 = (R << 16) | (G << 8) | B;
            *Dest = Color32;
            Dest++;
        }
        DestRow += Bitmap->Pitch;
    }
}

internal void
DrawBitmap(game_bitmap *Output, game_bitmap *Bitmap, render_entry_bitmap *Entry)
{
    int32 MinX = FloorReal32ToInt32(Entry->Offset.x);
    int32 MinY = FloorReal32ToInt32(Entry->Offset.y);
    int32 MaxX = MinX + FloorReal32ToInt32(Entry->Scale.x);
    int32 MaxY = MinY + FloorReal32ToInt32(Entry->Scale.y);

    int32 ClipMinX = 0;
    int32 ClipMaxX = Output->Width;
    int32 ClipMinY = 0;
    int32 ClipMaxY = Output->Height;

    s32 SourceStartX = FloorReal32ToInt32(Entry->UVOffset.x*(r32)Bitmap->Width);
    if(Entry->UVScale.x < 0.0f)
    {
        --SourceStartX;
    }
    s32 SourceStartY = FloorReal32ToInt32(Entry->UVOffset.y*(r32)Bitmap->Height);
    s32 SourceAdvanceX = (s32)SignOf(Entry->UVScale.x);
    s32 SourceAdvanceY = (s32)SignOf(Entry->UVScale.y);

    if(MinX < ClipMinX)
    {
        SourceStartX += SourceAdvanceX*(ClipMinX - MinX);
        MinX = ClipMinX;
    }
    if(MinY < ClipMinY)
    {
        SourceStartY += SourceAdvanceY*(ClipMinY - MinY);
        MinY = ClipMinY;
    }
    if(MaxX > ClipMaxX)
    {
        MaxX = ClipMaxX;
    }
    if(MaxY > ClipMaxY)
    {
        MaxY = ClipMaxY;
    }

    if(SourceStartX >= Bitmap->Width)
    {
        SourceStartX = Bitmap->Width - 1;
    }
    else if(SourceStartX < 0)
    {
        SourceStartX = 0;
    }

    Assert(SourceStartY >= 0);
    Assert(SourceStartY < Bitmap->Height);

    uint8 *DestRow = (uint8 *)Output->Pixels + MinY*Output->Pitch + MinX*BITMAP_BYTES_PER_PIXEL;
    uint8 *SourceRow = (uint8 *)Bitmap->Pixels + SourceStartY*Bitmap->Pitch + SourceStartX*BITMAP_BYTES_PER_PIXEL;

    for(int32 Y = MinY;
        Y < MaxY;
        ++Y)
    {
        uint32 *Source = (uint32 *)SourceRow;
        uint32 *Dest = (uint32*)DestRow;
        for(int32 X = MinX;
            X < MaxX;
            ++X)
        {
            uint32 Dest32 = *Dest;
            uint32 Source32 = *Source;

            real32 Inv255 = 1.0f / 255.0f;

            real32 SB = (real32)(Source32 & 0xFF) * Inv255;
            real32 SG = (real32)((Source32 >> 8) & 0xFF) * Inv255;
            real32 SR = (real32)((Source32 >> 16) & 0xFF) * Inv255;
            uint32 SA = (Source32 >> 24) & 0xFF;

            SR *= Entry->Color.r;
            SG *= Entry->Color.g;
            SB *= Entry->Color.b;

            if(SA)
            {
                *Dest = ((uint32)(SB*255.0f + 0.5f) |
                         ((uint32)(SG*255.0f + 0.5f) << 8) |
                         ((uint32)(SR*255.0f + 0.5f) << 16));
            }

            Dest++;
            Source += SourceAdvanceX;
        }
        SourceRow += Bitmap->Pitch;
        DestRow += Output->Pitch;
    }
}

internal void
Win32OutputRenderList(void *RenderList, u32 UsedSize, game_bitmap *AtlasBitmap, game_bitmap *OutputBitmap)
{
    render_entry_base *Base = (render_entry_base *)RenderList;
    render_entry_base *End = (render_entry_base *)((uint8 *)RenderList + UsedSize);
    while(Base < End)
    {
        switch(Base->ID)
        {
            case RenderEntry_Clear:
            {
                render_entry_clear *Entry = (render_entry_clear *)(Base + 1);
                DrawRect(OutputBitmap, 0, 0, OutputBitmap->Width, OutputBitmap->Height, Entry->Color);
                Base = (render_entry_base *)(Entry + 1);
            } break;

            case RenderEntry_Rect:
            {
                render_entry_rect *Entry = (render_entry_rect *)(Base + 1);
                DrawRect(OutputBitmap, Entry->MinX, Entry->MinY, Entry->MaxX, Entry->MaxY, Entry->Color);
                Base = (render_entry_base *)(Entry + 1);
            } break;

            case RenderEntry_Bitmap:
            {
                render_entry_bitmap *Entry = (render_entry_bitmap *)(Base + 1);
                DrawBitmap(OutputBitmap, AtlasBitmap, Entry);
                Base = (render_entry_base *)(Entry + 1);
            } break;

            InvalidDefaultCase;
        }
    }
}

internal atlas
Win32LoadAtlas()
{
    atlas Atlas = {};
    void *Contents = Win32ReadEntireFile("atlas.bin");
    atlas_header *Header = (atlas_header *)Contents;
    Atlas.Infos = (bitmap_info *)((u8 *)Contents + Header->InfosOffset);
    Atlas.Pixels = (u8 *)Contents + Header->PixelsOffset;
    return(Atlas);
}

internal void
Win32HandlePerfCounters(game_memory *Memory)
{
    char Buffer[128];
    OutputDebugStringA("DEBUG PERFORMANCE COUNTERS:\n");
    for(u32 CounterId = 0;
        CounterId < PerfCounter_Count;
        ++CounterId)
    {
        perf_counter *Counter = Memory->PerfCounters + CounterId;
        if(Counter->Hits)
        {
            sprintf_s(Buffer, sizeof(Buffer),
                      "%u: %I64ucy %I64uh %I64ucy/h\n",
                      CounterId, Counter->Cycles, Counter->Hits, Counter->Cycles/Counter->Hits);
            OutputDebugStringA(Buffer);
            Counter->Cycles = 0;
            Counter->Hits = 0;
        }
    }
}

int APIENTRY
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, char *CommandLine, int ShowCmd)
{
    char EXEFullPath[WIN32_MAX_FILE_PATH_LENGTH];
    DWORD CharsCopied = GetModuleFileNameA(0, EXEFullPath, sizeof(EXEFullPath));
    Assert(CharsCopied > 0);
    Assert(CharsCopied < sizeof(EXEFullPath));

    uint32 LastSlashIndex = 0;
    for(uint32 CharIndex = 0;
        CharIndex < CharsCopied;
        ++CharIndex)
    {
        if(EXEFullPath[CharIndex] == '\\')
        {
            LastSlashIndex = CharIndex;
        }
    }
    Assert(LastSlashIndex > 0);
    Assert(LastSlashIndex < CharsCopied);
    uint32 OnePastLastSlashIndex = LastSlashIndex + 1;

    char GameDLLFullPath[WIN32_MAX_FILE_PATH_LENGTH];
    char GameDLLName[] = "game.dll";
    ConcatenateStrings(EXEFullPath, OnePastLastSlashIndex,
                       GameDLLName, sizeof(GameDLLName),
                       GameDLLFullPath, sizeof(GameDLLFullPath));

    char GameTempDLLFullPath[WIN32_MAX_FILE_PATH_LENGTH];
    char GameTempDLLName[] = "game_temp.dll";
    ConcatenateStrings(EXEFullPath, OnePastLastSlashIndex,
                       GameTempDLLName, sizeof(GameTempDLLName),
                       GameTempDLLFullPath, sizeof(GameTempDLLFullPath));

    char LockFullPath[WIN32_MAX_FILE_PATH_LENGTH];
    char LockName[] = "lock.tmp";
    ConcatenateStrings(EXEFullPath, OnePastLastSlashIndex,
                       LockName, sizeof(LockName),
                       LockFullPath, sizeof(LockFullPath));

    win32_game Game = Win32LoadGame(GameDLLFullPath, GameTempDLLFullPath);

    LARGE_INTEGER CounterFrequency;
    BOOL QueryPerformanceFrequencySuccess = QueryPerformanceFrequency(&CounterFrequency);
    Assert(QueryPerformanceFrequencySuccess);
    int64 CountsPerSecond = CounterFrequency.QuadPart;

    MMRESULT TimeBeginPeriodResult = timeBeginPeriod(1);
    bool32 SleepIsGranular = (TimeBeginPeriodResult == TIMERR_NOERROR);

    win32_backbuffer *Backbuffer = &GlobalBackbuffer;

    Backbuffer->Width = BACKBUFFER_WIDTH;
    Backbuffer->Height = BACKBUFFER_HEIGHT;
    Backbuffer->Pitch = Backbuffer->Width*BITMAP_BYTES_PER_PIXEL;

    Backbuffer->Info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    Backbuffer->Info.bmiHeader.biWidth = Backbuffer->Width;
    Backbuffer->Info.bmiHeader.biHeight = Backbuffer->Height;
    Backbuffer->Info.bmiHeader.biPlanes = 1;
    Backbuffer->Info.bmiHeader.biBitCount = 8*BITMAP_BYTES_PER_PIXEL;
    Backbuffer->Info.bmiHeader.biCompression = BI_RGB;
    Backbuffer->Info.bmiHeader.biSizeImage = 0;
    Backbuffer->Info.bmiHeader.biXPelsPerMeter = 0;
    Backbuffer->Info.bmiHeader.biYPelsPerMeter = 0;
    Backbuffer->Info.bmiHeader.biClrUsed = 0;
    Backbuffer->Info.bmiHeader.biClrImportant = 0;

    int32 BackbufferSize = Backbuffer->Width*Backbuffer->Height*BITMAP_BYTES_PER_PIXEL;
    Backbuffer->Memory = VirtualAlloc(0, BackbufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(Backbuffer->Memory);

    WNDCLASSA WindowClass;
    WindowClass.style = 0;
    WindowClass.lpfnWndProc = Win32WindowProc;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = Instance;
    WindowClass.hIcon = 0;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    WindowClass.hbrBackground = 0;
    WindowClass.lpszMenuName = 0;
    WindowClass.lpszClassName = "MyWindowClass";

    ATOM WindowClassAtom = RegisterClassA(&WindowClass);
    Assert(WindowClassAtom);

    DWORD WindowStyle = WS_OVERLAPPEDWINDOW|WS_VISIBLE;
    int32 BackbufferScale = 1;
    RECT WindowRect = {0, 0, BackbufferScale*Backbuffer->Width, BackbufferScale*Backbuffer->Height};
    AdjustWindowRect(&WindowRect, WindowStyle, false);

    HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "ECS Demo",
                                  WindowStyle,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  WindowRect.right - WindowRect.left,
                                  WindowRect.bottom - WindowRect.top,
                                  0, 0, Instance, 0);
    Assert(Window);

    int32 TargetFramesPerSecond = 60;
    int64 CountsPerFrame = CountsPerSecond / TargetFramesPerSecond;
    real32 TargetSecondsPerFrame = 1.0f / (real32)TargetFramesPerSecond;
    real32 dt = TargetSecondsPerFrame;

#if GAME_INTERNAL
    LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
    LPVOID BaseAddress = 0;
#endif
    
    game_memory Memory = {};

    Memory.PermanentStorageSize = Megabytes(2);
    Memory.RenderListSize = Kilobytes(256);
    memory_index TotalMemorySize = Memory.PermanentStorageSize + Memory.RenderListSize;

    Memory.PermanentStorage = VirtualAlloc(BaseAddress, TotalMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(Memory.PermanentStorage);
    Memory.RenderList = (uint8 *)Memory.PermanentStorage + Memory.PermanentStorageSize;

    atlas Atlas = Win32LoadAtlas();

    game_bitmap AtlasBitmap = {};
    AtlasBitmap.Width = ATLAS_WIDTH;
    AtlasBitmap.Height = ATLAS_HEIGHT;
    AtlasBitmap.Pitch = ATLAS_PITCH;
    AtlasBitmap.Pixels = Atlas.Pixels;

    game_input Inputs[2] = {};
    game_input *OldInput = &Inputs[0];
    game_input *NewInput = &Inputs[1];

#if GAME_INTERNAL
    input_recorder_state InputRecorderState = InputRecorderState_Off;

    char InputRecorderFilePath[WIN32_MAX_FILE_PATH_LENGTH];
    char InputRecorderFileName[] = "recorded_input.bin";
    ConcatenateStrings(EXEFullPath, OnePastLastSlashIndex,
                       InputRecorderFileName, sizeof(InputRecorderFileName),
                       InputRecorderFilePath, sizeof(InputRecorderFilePath));

    HANDLE InputRecorderFile = CreateFileA(InputRecorderFilePath,
                                                 GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                                                 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    Assert(InputRecorderFile != INVALID_HANDLE_VALUE);
#endif

    GlobalRunning = true;

    LARGE_INTEGER FrameStartCounter = Win32GetCounter();

    while(GlobalRunning)
    {
        FILETIME NewLastWriteTime = Win32GetLastWriteTime(GameDLLFullPath);
        if((CompareFileTime(&Game.LastWriteTime, &NewLastWriteTime) != 0) &&
           !Win32FileExists(LockFullPath))
        {
            BOOL FreeLibrarySuccess = FreeLibrary(Game.DLL);
            Assert(FreeLibrarySuccess);

            Game = Win32LoadGame(GameDLLFullPath, GameTempDLLFullPath);
        }

        MSG Message;
        while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
        {
            switch(Message.message)
            {
                case WM_QUIT:
                {
                    GlobalRunning = false;
                } break;

                case WM_KEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                {
                    uint32 VKCode  = (uint32)Message.wParam;
                    bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                    bool32 IsDown  = ((Message.lParam & (1 << 31)) == 0);

                    if(IsDown != WasDown)
                    {
                        if(VKCode == 'A')
                        {
                            NewInput->Left.IsDown = IsDown;
                        }
                        if(VKCode == 'D')
                        {
                            NewInput->Right.IsDown = IsDown;
                        }
                        if(VKCode == 'W')
                        {
                            NewInput->Up.IsDown = IsDown;
                        }
                        if(VKCode == 'S')
                        {
                            NewInput->Down.IsDown = IsDown;
                        }
                        if(VKCode == VK_SPACE)
                        {
                            NewInput->Action.IsDown = IsDown;
                        }
#if GAME_INTERNAL
                        if(VKCode == 'L' && IsDown)
                        {
                            switch(InputRecorderState)
                            {
                                case InputRecorderState_Off:
                                {
                                    Win32RewindFile(InputRecorderFile);
                                    Win32SaveMemory(InputRecorderFile, &Memory);
                                    InputRecorderState = InputRecorderState_Recording;
                                } break;

                                case InputRecorderState_Playing:
                                {
                                    InputRecorderState = InputRecorderState_Off;
                                    // NOTE(slava): To avoid buttons becoming stuck
                                    *NewInput = {};
                                } break;

                                case InputRecorderState_Recording:
                                {
                                    Win32RewindFile(InputRecorderFile);
                                    Win32LoadMemory(InputRecorderFile, &Memory);
                                    InputRecorderState = InputRecorderState_Playing;
                                } break;

                                InvalidDefaultCase;
                            }
                        }
#endif
                    }
                } break;

                default:
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                } break;
            }
        }

#if GAME_INTERNAL
        switch(InputRecorderState)
        {
            case InputRecorderState_Off:
            {
                // NOTE(slava): Do nothing
            } break;

            case InputRecorderState_Playing:
            {
                if(Win32LoadInput(InputRecorderFile, NewInput))
                {
                    Win32RewindFile(InputRecorderFile);
                    Win32LoadMemory(InputRecorderFile, &Memory);
                    bool32 ReachedEndOfFile = Win32LoadInput(InputRecorderFile, NewInput);
                    Assert(!ReachedEndOfFile);
                }
            } break;

            case InputRecorderState_Recording:
            {
                Win32SaveInput(InputRecorderFile, NewInput);
            } break;

            InvalidDefaultCase;
        }
#endif

        for(int32 ButtonIndex = 0;
            ButtonIndex < ArrayCount(NewInput->Buttons);
            ++ButtonIndex)
        {
            game_button *NewButton = NewInput->Buttons + ButtonIndex;
            game_button *OldButton = OldInput->Buttons + ButtonIndex;
            NewButton->JustWentDown = NewButton->IsDown && !OldButton->IsDown;
        }

        NewInput->dt = dt;
        Memory.RenderListUsed = 0;
        Game.UpdateAndRender(&Memory, NewInput, Atlas.Infos);
#if !WASM_BUILD
        Win32HandlePerfCounters(&Memory);
#endif
        Win32OutputRenderList(Memory.RenderList, Memory.RenderListUsed, &AtlasBitmap, (game_bitmap *)Backbuffer);

        LARGE_INTEGER FrameEndCounter = Win32GetCounter();
        int64 CountsElapsed = FrameEndCounter.QuadPart - FrameStartCounter.QuadPart;
        while(CountsElapsed < CountsPerFrame)
        {
            int64 SleepCounts = CountsPerFrame - CountsElapsed;
            int32 SleepMS = (int32)((1000*SleepCounts) / CountsPerSecond);
            if((SleepMS > 0) && SleepIsGranular)
            {
                Sleep(SleepMS);
            }

            FrameEndCounter = Win32GetCounter();
            CountsElapsed = FrameEndCounter.QuadPart - FrameStartCounter.QuadPart;
        }

        FrameStartCounter = FrameEndCounter;

        // NOTE(slava): Frame flip

        HDC DeviceContext = GetDC(Window);
        win32_client_dim ClientDim = Win32GetClientDim(Window);
        Win32DisplayBackbuffer(Backbuffer, DeviceContext, ClientDim.X, ClientDim.Y);
        ReleaseDC(Window, DeviceContext);

        game_input *TempInput = NewInput;
        NewInput = OldInput;
        OldInput = TempInput;
        *NewInput = *OldInput;

#if 1
        real32 SecondsElapsed = (real32)CountsElapsed / (real32)CountsPerSecond;
        real32 MSElapsed = 1000.0f*SecondsElapsed;
        real32 FramesPerSecond = 1.0f / SecondsElapsed;

        char Buffer[256];
        sprintf_s(Buffer, "%.2fms %.2ffps\n", MSElapsed, FramesPerSecond);
        OutputDebugStringA(Buffer);
#endif
    }

    return(0);
}
