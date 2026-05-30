#ifndef PLATFORM_H
#define PLATFORM_H

#include <stddef.h>
#include <stdint.h>

#define global_variable static
#define local_persist static
#define internal static

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uint32_t bool32;
typedef uint32_t b32;
typedef uint8_t b8;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef size_t memory_index;

typedef float real32;
typedef double real64;
typedef float r32;
typedef double r64;

#include <float.h>
#define Real32Max FLT_MAX

#define Pi32 3.14159265358979f
#define Tau32 6.283185307179586f
#define DegreesToRadians (Tau32/360.0f)
#define RadiansToDegrees (360.0f/Tau32)

#define Minimum(A, B) ((A) < (B) ? (A) : (B))
#define Maximum(A, B) ((A) > (B) ? (A) : (B))

#define Kilobytes(X) (1024ULL*(X))
#define Megabytes(X) (1024ULL*Kilobytes(X))
#define Gigabytes(X) (1024ULL*Megabytes(X))
#define Terabytes(X) (1024ULL*Gigabytes(X))

#define ArrayCount(A) (sizeof(A)/sizeof(A[0]))

#if GAME_SLOW
#define Assert(Expression) if(!(Expression)) { *(volatile int *)0 = 0; }
#else
#define Assert(Expression)
#endif

#define InvalidCodePath Assert(!"Invalid code path")
#define InvalidDefaultCase default: { InvalidCodePath } break

internal void
ZeroSize(void *Memory, memory_index Size)
{
    uint8 *Dest = (uint8 *)Memory;
    while(Size--)
    {
        *Dest++ = 0;
    }
}

#define ZeroStruct(S) ZeroSize(&(S), sizeof(S))

#include "intrinsics.h"
#include "my_math.h"
#include "render_list.h"
#include "atlas.h"

#if !WASM_BUILD
#include <intrin.h>

enum perf_counter_id
{
    PerfCounter_GameUpdateAndRender, // 0
    PerfCounter_CollisionDetectionSystem, // 1
    PerfCounter_PopulateGrid, // 2
    PerfCounter_DetectCollisions, // 3
    PerfCounter_CheckCollision, // 4

    PerfCounter_Count,
};

struct perf_counter
{
    u64 Cycles;
    u64 Hits;
};

#define BEGIN_TIMED_BLOCK(Id) u64 BeginCounter_##Id = __rdtsc();
#define END_TIMED_BLOCK(Id) DebugGameMemory->PerfCounters[PerfCounter_##Id].Cycles += __rdtsc() - BeginCounter_##Id; ++DebugGameMemory->PerfCounters[PerfCounter_##Id].Hits;
#define END_TIMED_BLOCK_COUNTED(Id, Count) DebugGameMemory->PerfCounters[PerfCounter_##Id].Cycles += __rdtsc() - BeginCounter_##Id; DebugGameMemory->PerfCounters[PerfCounter_##Id].Hits += (Count);
#else
#define BEGIN_TIMED_BLOCK(Id)
#define END_TIMED_BLOCK(Id)
#define END_TIMED_BLOCK_COUNTED(Id, Hits)
#endif

struct game_memory
{
    void *PermanentStorage;
    u32 PermanentStorageSize;

    void *RenderList;
    u32 RenderListSize;
    u32 RenderListUsed;
    u32 RenderListBitmapCount;

#if !WASM_BUILD
    perf_counter PerfCounters[PerfCounter_Count];
#endif
};

struct game_button
{
    bool32 IsDown;
    bool32 JustWentDown;
};

struct game_input
{
    real32 dt;

    union
    {
        struct
        {
            game_button Left;
            game_button Right;
            game_button Up;
            game_button Down;
            game_button Action;
        };

        game_button Buttons[5];
    };
};

#define BITMAP_BYTES_PER_PIXEL 4
struct game_bitmap
{
    int32 Width;
    int32 Height;
    int32 Pitch;
    void *Pixels;
};

#define BACKBUFFER_WIDTH 1024
#define BACKBUFFER_HEIGHT 768

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory, game_input *Input, bitmap_info *BitmapInfos)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#endif
