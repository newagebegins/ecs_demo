#include "game.h"

#include "render_group.cpp"

internal void
SpriteSystem(ecs *ECS, render_group *RenderGroup)
{
    for(u32 SpriteIndex = 0;
        SpriteIndex < ECS->sprite_comp_Pool->Count;
        ++SpriteIndex)
    {
        sprite_comp *Sprite = GetComp(ECS, sprite_comp, SpriteIndex);
        entity_id EntityID = ECS->sprite_comp_Pool->DenseToEntity[SpriteIndex];
        s32 PositionIndex = ECS->position_comp_Pool->EntityToDense[EntityID.Value];
        Assert(PositionIndex >= 0);
        position_comp *Position = GetComp(ECS, position_comp, PositionIndex);
        PushBitmap(RenderGroup, Sprite->BitmapID,
                   (s32)Position->Position.x, (s32)Position->Position.y);
    }
}

internal comp_pool *
AllocateCompPool(memory_arena *Arena, u32 CompSize)
{
    comp_pool *Pool = PushStruct(Arena, comp_pool);
    Pool->Count = 0;
    Pool->Dense = PushSize(Arena, MAX_ENTITY_COUNT*CompSize);
    Pool->DenseToEntity = PushArray(Arena, MAX_ENTITY_COUNT, entity_id);

    Pool->EntityToDense = PushArray(Arena, MAX_ENTITY_COUNT, s32);
    for(u32 EntityID = 0;
        EntityID < MAX_ENTITY_COUNT;
        ++EntityID)
    {
        Pool->EntityToDense[EntityID] = -1;
    }

    return(Pool);
}

internal ecs *
AllocateECS(memory_arena *Arena)
{
    ecs *ECS = PushStruct(Arena, ecs);
    ECS->position_comp_Pool = AllocateCompPool(Arena, sizeof(position_comp));
    ECS->sprite_comp_Pool = AllocateCompPool(Arena, sizeof(sprite_comp));
    return(ECS);
}

internal entity_id
AddEntity(ecs *ECS)
{
    entity_id Result = {ECS->EntityCount++};
    return(Result);
}

internal void *
AddComponent_(comp_pool *Pool, entity_id EntityID, u32 CompSize)
{
    Assert(Pool->Count < MAX_ENTITY_COUNT);
    u32 DenseIndex = Pool->Count++;
    void *Component = (u8 *)Pool->Dense + DenseIndex*CompSize;
    Pool->DenseToEntity[DenseIndex] = EntityID;
    Pool->EntityToDense[EntityID.Value] = DenseIndex;
    return(Component);
}

#define AddComponent(ECS, EntityID, CompType) (CompType *)AddComponent_(ECS->CompType##_Pool, EntityID, sizeof(CompType))

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!GameState->IsInitialized)
    {
        GameState->MainArena = MakeArena(GameState + 1, Memory->PermanentStorageSize - sizeof(game_state));
        GameState->BitmapInfos = BitmapInfos;
        GameState->ECS = AllocateECS(&GameState->MainArena);

        for(u32 EntityIndex = 0;
            EntityIndex < 10;
            ++EntityIndex)
        {
            entity_id EntityID = AddEntity(GameState->ECS);
            position_comp *PositionComp = AddComponent(GameState->ECS, EntityID, position_comp);
            PositionComp->Position = V2(20.0f + EntityIndex*20.0f, 30.0f);
            sprite_comp *SpriteComp = AddComponent(GameState->ECS, EntityID, sprite_comp);
            SpriteComp->BitmapID = Bitmap_Guy;
        }

        GameState->IsInitialized = true;
    }

    memory_arena RenderArena = MakeArena(Memory->RenderList, Memory->RenderListSize);
    render_group RenderGroup;
    InitializeRenderGroup(&RenderGroup, &RenderArena, GameState->BitmapInfos);
    PushClear(&RenderGroup, Color_Black);

    SpriteSystem(GameState->ECS, &RenderGroup);

    Memory->RenderListUsed = (u32)RenderArena.Used;
    Memory->RenderListBitmapCount = RenderGroup.BitmapCount;
}
