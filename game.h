#ifndef GAME_H
#define GAME_H

#include "platform.h"
#include "random.h"

struct memory_arena
{
    void *Memory;
    memory_index Size;
    memory_index Used;
    int32 TempCount;
};

inline memory_arena
MakeArena(void *Memory, memory_index Size)
{
    memory_arena Arena = {};
    Arena.Memory = Memory;
    Arena.Size = Size;
    return(Arena);
}

struct temp_memory
{
    memory_arena *Arena;
    memory_index Used;
};

inline temp_memory
BeginTemporaryMemory(memory_arena *Arena)
{
    temp_memory Result = {Arena, Arena->Used};
    ++Arena->TempCount;
    return(Result);
}

inline void
EndTemporaryMemory(temp_memory TempMem)
{
    TempMem.Arena->Used = TempMem.Used;
    --TempMem.Arena->TempCount;
}

inline void
CheckArena(memory_arena *Arena)
{
    Assert(Arena->TempCount == 0);
}

inline void *
PushSize(memory_arena *Arena, memory_index Size)
{
    Assert(Arena->Used + Size <= Arena->Size);
    void *Result = (uint8 *)Arena->Memory + Arena->Used;
    Arena->Used += Size;
    return(Result);
}

#define PushStruct(Arena, type) (type *)PushSize(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize(Arena, (Count)*sizeof(type))

inline memory_index
GetRemainingSize(memory_arena *Arena)
{
    Assert(Arena->Size >= Arena->Used);
    memory_index Result = Arena->Size - Arena->Used;
    return(Result);
}

inline memory_arena
SubArena(memory_arena *Parent, memory_index Size = 0)
{
    if(Size == 0)
    {
        Size = GetRemainingSize(Parent);
    }
    memory_arena Arena = MakeArena(PushSize(Parent, Size), Size);
    return(Arena);
}

inline void
ClearArena(memory_arena *Arena)
{
    Assert(Arena->TempCount == 0);
    Arena->Used = 0;
}

#include "render_group.h"

#define MAX_ENTITY_COUNT 8192

struct entity_id
{
    s32 Value;
};

struct position_comp
{
    v2 Position;
};

struct velocity_comp
{
    v2 Velocity;
};

struct hitbox_comp
{
    v2 HalfDim;
};

struct sprite_comp
{
    bitmap_id BitmapID;
    r32 FrameTimer;
    r32 FrameDuration;
    s32 FrameIndex;
    color Color;
};

struct comp_pool
{
    s32 Count;
    void *Dense;
    entity_id *DenseToEntity;
    s32 *EntityToDense;
};

struct collision_event
{
    entity_id EntityA;
    entity_id EntityB;
    v2 SeparationVector;
};

#define MAX_COLLISION_EVENTS_COUNT 4096

struct ecs
{
    comp_pool *position_comp_Pool;
    comp_pool *velocity_comp_Pool;
    comp_pool *hitbox_comp_Pool;
    comp_pool *sprite_comp_Pool;

    s32 EntityCount;

    collision_event *CollisionEvents;
    u32 CollisionEventsCount;
    b8 *WasPushedThisFrame;
};

#define GetComp(ECS, CompType, Index) (CompType *)((u8 *)ECS->CompType##_Pool->Dense + Index*sizeof(CompType))

struct game_state
{
    b32 IsInitialized;
    memory_arena MainArena;
    bitmap_info *BitmapInfos;
    random_series GeneralEntropy;
    ecs *ECS;
};

struct transient_state
{
    b32 IsInitialized;
    memory_arena TranArena;
};

#endif
