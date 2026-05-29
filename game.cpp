#include "game.h"

#include "render_group.cpp"

internal void
SpriteSystem(ecs *ECS, render_group *RenderGroup, r32 dt)
{
    for(s32 SpriteIndex = 0;
        SpriteIndex < ECS->sprite_comp_Pool->Count;
        ++SpriteIndex)
    {
        sprite_comp *Sprite = GetComp(ECS, sprite_comp, SpriteIndex);

        Sprite->FrameTimer += dt;
        if(Sprite->FrameTimer >= Sprite->FrameDuration)
        {
            Sprite->FrameTimer -= Sprite->FrameDuration;
            bitmap_info *Info = RenderGroup->BitmapInfos + Sprite->BitmapID;
            Sprite->FrameIndex = (Sprite->FrameIndex + 1) % Info->FrameCount;
        }

        entity_id EntityID = ECS->sprite_comp_Pool->DenseToEntity[SpriteIndex];
        s32 PositionIndex = ECS->position_comp_Pool->EntityToDense[EntityID.Value];
        Assert(PositionIndex >= 0);
        position_comp *Position = GetComp(ECS, position_comp, PositionIndex);

        PushBitmap(RenderGroup, Sprite->BitmapID,
                   (s32)Position->Position.x, (s32)Position->Position.y,
                   Sprite->FrameIndex);
    }
}

internal void
MovementSystem(ecs *ECS, r32 dt)
{
    for(s32 VelocityCompIndex = 0;
        VelocityCompIndex < ECS->velocity_comp_Pool->Count;
        ++VelocityCompIndex)
    {
        velocity_comp *VelocityComp = GetComp(ECS, velocity_comp, VelocityCompIndex);

        entity_id EntityID = ECS->velocity_comp_Pool->DenseToEntity[VelocityCompIndex];
        s32 PositionCompIndex = ECS->position_comp_Pool->EntityToDense[EntityID.Value];
        Assert(PositionCompIndex >= 0);
        position_comp *PositionComp = GetComp(ECS, position_comp, PositionCompIndex);

        v2 P = PositionComp->Position;
        P += VelocityComp->Velocity * dt;

        if(P.x < 0.0f)
        {
            P.x += BACKBUFFER_WIDTH;
        }
        else if(P.x >= BACKBUFFER_WIDTH)
        {
            P.x -= BACKBUFFER_WIDTH;
        }

        if(P.y < 0.0f)
        {
            P.y += BACKBUFFER_HEIGHT;
        }
        else if(P.y >= BACKBUFFER_HEIGHT)
        {
            P.y -= BACKBUFFER_HEIGHT;
        }

        PositionComp->Position = P;
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
    for(u32 Index = 0;
        Index < MAX_ENTITY_COUNT;
        ++Index)
    {
        Pool->EntityToDense[Index] = -1;
        Pool->DenseToEntity[Index].Value = -1;
    }

    return(Pool);
}

internal ecs *
AllocateECS(memory_arena *Arena)
{
    ecs *ECS = PushStruct(Arena, ecs);
    ECS->position_comp_Pool = AllocateCompPool(Arena, sizeof(position_comp));
    ECS->velocity_comp_Pool = AllocateCompPool(Arena, sizeof(velocity_comp));
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
    Assert(Pool->DenseToEntity[DenseIndex].Value == -1);
    Pool->DenseToEntity[DenseIndex] = EntityID;
    Assert(Pool->EntityToDense[EntityID.Value] == -1);
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
        GameState->GeneralEntropy = RandomSeries(17);
        GameState->ECS = AllocateECS(&GameState->MainArena);

        u32 CellSize = 16;
        u32 CellCountX = BACKBUFFER_WIDTH/CellSize;
        u32 CellCountY = BACKBUFFER_HEIGHT/CellSize;

        for(u32 CellY = 0;
            CellY < CellCountY;
            ++CellY)
        {
            for(u32 CellX = 0;
                CellX < CellCountX;
                ++CellX)
            {
                if(RandomChoice(&GameState->GeneralEntropy, 8) == 1)
                {
                    r32 X = (r32)(CellX*CellSize);
                    r32 Y = (r32)(CellY*CellSize);

                    entity_id EntityID = AddEntity(GameState->ECS);

                    position_comp *PositionComp = AddComponent(GameState->ECS, EntityID, position_comp);
                    PositionComp->Position = V2(X, Y);

                    velocity_comp *VelocityComp = AddComponent(GameState->ECS, EntityID, velocity_comp);
                    VelocityComp->Velocity =
                        20.0f*Normalize(V2(RandomBilateral(&GameState->GeneralEntropy),
                                           RandomBilateral(&GameState->GeneralEntropy)));

                    sprite_comp *SpriteComp = AddComponent(GameState->ECS, EntityID, sprite_comp);
                    SpriteComp->BitmapID = Bitmap_Guy;
                    SpriteComp->FrameTimer = 0.0f;
                    SpriteComp->FrameDuration = 0.2f;
                    SpriteComp->FrameIndex = 0;
                }
            }
        }

        GameState->IsInitialized = true;
    }

    memory_arena RenderArena = MakeArena(Memory->RenderList, Memory->RenderListSize);
    render_group RenderGroup;
    InitializeRenderGroup(&RenderGroup, &RenderArena, GameState->BitmapInfos);
    PushClear(&RenderGroup, Color_Black);

    MovementSystem(GameState->ECS, Input->dt);
    SpriteSystem(GameState->ECS, &RenderGroup, Input->dt);

    Memory->RenderListUsed = (u32)RenderArena.Used;
    Memory->RenderListBitmapCount = RenderGroup.BitmapCount;
}
