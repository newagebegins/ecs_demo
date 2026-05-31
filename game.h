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

#define MAX_ENTITY_COUNT 2048

struct sprite
{
    bitmap_id BitmapID;
    r32 FrameTimer;
    r32 FrameDuration;
    u32 FrameIndex;
    color Color;
};

struct collision_event
{
    u32 EntityA;
    u32 EntityB;
    // NOTE(slava): Points to A
    v2 SeparationVector;
};

#define CELL_SIZE 64
#define CELL_COUNT_X BACKBUFFER_WIDTH/CELL_SIZE
#define CELL_COUNT_Y BACKBUFFER_HEIGHT/CELL_SIZE

struct grid_cell
{
    u32 Entities[32];
    u32 EntityCount;
};

enum component_masks
{
    ComponentMask_Position = (1 << 0),
    ComponentMask_RigidBody = (1 << 1),
    ComponentMask_HalfDim = (1 << 2),
    ComponentMask_Sprite = (1 << 3),
    ComponentMask_Bomber = (1 << 4),
};

struct rigid_body
{
    v2 Velocity;
    v2 Acceleration;
    r32 InvMass;
    r32 FrictionCoeff;
};

struct ecs
{
    u32 ComponentMasks[MAX_ENTITY_COUNT];

    v2 Positions[MAX_ENTITY_COUNT];
    rigid_body RigidBodies[MAX_ENTITY_COUNT];
    v2 HalfDims[MAX_ENTITY_COUNT];
    sprite Sprites[MAX_ENTITY_COUNT];

    u32 EntityCount;

    grid_cell Grid[CELL_COUNT_Y][CELL_COUNT_X];

    collision_event CollisionEvents[MAX_ENTITY_COUNT/4];
    u32 CollisionEventsCount;

    random_series *RandomSeries;
};

struct game_state
{
    b32 IsInitialized;
    memory_arena MainArena;
    bitmap_info *BitmapInfos;
    random_series GeneralEntropy;
    ecs ECS;
};

struct transient_state
{
    b32 IsInitialized;
    memory_arena TranArena;
};

global_variable game_memory *DebugGameMemory;

#endif
