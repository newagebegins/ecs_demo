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
        bitmap_info *Info = RenderGroup->BitmapInfos + Sprite->BitmapID;

        Sprite->FrameTimer += dt;
        if(Sprite->FrameTimer >= Sprite->FrameDuration)
        {
            Sprite->FrameTimer -= Sprite->FrameDuration;
            Sprite->FrameIndex = (Sprite->FrameIndex + 1) % Info->FrameCount;
        }

        entity_id EntityID = ECS->sprite_comp_Pool->DenseToEntity[SpriteIndex];
        s32 PositionIndex = ECS->position_comp_Pool->EntityToDense[EntityID.Value];
        Assert(PositionIndex >= 0);
        position_comp *Position = GetComp(ECS, position_comp, PositionIndex);

        PushBitmap(RenderGroup, Sprite->BitmapID,
                   (s32)(Position->Position.x - (r32)Info->FrameWidth*0.5f),
                   (s32)(Position->Position.y - (r32)Info->FrameHeight*0.5f),
                   Sprite->FrameIndex);

#if 0
        s32 HitboxIndex = ECS->hitbox_comp_Pool->EntityToDense[EntityID.Value];
        Assert(HitboxIndex >= 0);
        hitbox_comp *Hitbox = GetComp(ECS, hitbox_comp, HitboxIndex);
        v2 HitboxMin = Position->Position - Hitbox->HalfDim;
        v2 HitboxMax = Position->Position + Hitbox->HalfDim;
        PushRect(RenderGroup, (s32)HitboxMin.x, (s32)HitboxMin.y, (s32)HitboxMax.x, (s32)HitboxMax.y, Color_Cyan);
#endif
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

internal void
AddCollisionEvent(ecs *ECS, entity_id EntityA, entity_id EntityB, v2 SeparationVector)
{
    if(ECS->CollisionEventsCount < MAX_COLLISION_EVENTS_COUNT)
    {
        collision_event *Event = ECS->CollisionEvents + ECS->CollisionEventsCount++;
        Event->EntityA = EntityA;
        Event->EntityB = EntityB;
        Event->SeparationVector = SeparationVector;
    }
    else
    {
        Assert(!"No more space for collision events");
    }
}

internal void
CollisionDetectionSystem(ecs *ECS)
{
    ECS->CollisionEventsCount = 0;

    for(s32 HitboxAIndex = 0;
        HitboxAIndex < ECS->hitbox_comp_Pool->Count;
        ++HitboxAIndex)
    {
        hitbox_comp *HitboxA = GetComp(ECS, hitbox_comp, HitboxAIndex);

        entity_id EntityA = ECS->hitbox_comp_Pool->DenseToEntity[HitboxAIndex];
        s32 PositionAIndex = ECS->position_comp_Pool->EntityToDense[EntityA.Value];
        Assert(PositionAIndex >= 0);
        position_comp *PositionA = GetComp(ECS, position_comp, PositionAIndex);

        for(s32 HitboxBIndex = HitboxAIndex + 1;
            HitboxBIndex < ECS->hitbox_comp_Pool->Count;
            ++HitboxBIndex)
        {
            hitbox_comp *HitboxB = GetComp(ECS, hitbox_comp, HitboxBIndex);

            entity_id EntityB = ECS->hitbox_comp_Pool->DenseToEntity[HitboxBIndex];
            s32 PositionBIndex = ECS->position_comp_Pool->EntityToDense[EntityB.Value];
            Assert(PositionBIndex >= 0);
            position_comp *PositionB = GetComp(ECS, position_comp, PositionBIndex);

            r32 PosDiffX = PositionA->Position.x - PositionB->Position.x;
            r32 PosDiffY = PositionA->Position.y - PositionB->Position.y;

            r32 DistanceX = AbsoluteValue(PosDiffX);
            r32 DistanceY = AbsoluteValue(PosDiffY);

            r32 OverlapX = (HitboxA->HalfDim.x + HitboxB->HalfDim.x) - DistanceX;
            r32 OverlapY = (HitboxA->HalfDim.y + HitboxB->HalfDim.y) - DistanceY;

            if(OverlapX > 0.0f && OverlapY > 0.0f)
            {
                v2 SeparationVector;
                if(OverlapX < OverlapY)
                {
                    SeparationVector = {OverlapX * SignOf(PosDiffX), 0.0f};
                }
                else
                {
                    SeparationVector = {0.0f, OverlapY * SignOf(PosDiffY)};
                }
                AddCollisionEvent(ECS, EntityA, EntityB, SeparationVector);
            }
        }
    }
}

internal void
CollisionResponseSystem(ecs *ECS)
{
    for(u32 EventIndex = 0;
        EventIndex < ECS->CollisionEventsCount;
        ++EventIndex)
    {
        collision_event *Event = ECS->CollisionEvents + EventIndex;

        s32 PositionAIndex = ECS->position_comp_Pool->EntityToDense[Event->EntityA.Value];
        Assert(PositionAIndex >= 0);
        position_comp *PositionA = GetComp(ECS, position_comp, PositionAIndex);

        s32 PositionBIndex = ECS->position_comp_Pool->EntityToDense[Event->EntityB.Value];
        Assert(PositionBIndex >= 0);
        position_comp *PositionB = GetComp(ECS, position_comp, PositionBIndex);

        s32 VelocityAIndex = ECS->velocity_comp_Pool->EntityToDense[Event->EntityA.Value];
        Assert(VelocityAIndex >= 0);
        velocity_comp *VelocityA = GetComp(ECS, velocity_comp, VelocityAIndex);

        s32 VelocityBIndex = ECS->velocity_comp_Pool->EntityToDense[Event->EntityB.Value];
        Assert(VelocityBIndex >= 0);
        velocity_comp *VelocityB = GetComp(ECS, velocity_comp, VelocityBIndex);

        PositionA->Position += Event->SeparationVector*0.5f;
        PositionB->Position -= Event->SeparationVector*0.5f;

        if(Event->SeparationVector.x != 0.0f)
        {
            VelocityA->Velocity.x = -VelocityA->Velocity.x;
            VelocityB->Velocity.x = -VelocityB->Velocity.x;
        }
        else
        {
            VelocityA->Velocity.y = -VelocityA->Velocity.y;
            VelocityB->Velocity.y = -VelocityB->Velocity.y;
        }
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
    ECS->hitbox_comp_Pool = AllocateCompPool(Arena, sizeof(hitbox_comp));
    ECS->sprite_comp_Pool = AllocateCompPool(Arena, sizeof(sprite_comp));

    ECS->EntityCount = 0;

    ECS->CollisionEvents = PushArray(Arena, MAX_COLLISION_EVENTS_COUNT, collision_event);
    ECS->CollisionEventsCount = 0;

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

                    hitbox_comp *HitboxComp = AddComponent(GameState->ECS, EntityID, hitbox_comp);
                    HitboxComp->HalfDim = V2(5.0f, 6.0f);

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
    CollisionDetectionSystem(GameState->ECS);
    CollisionResponseSystem(GameState->ECS);
    SpriteSystem(GameState->ECS, &RenderGroup, Input->dt);

    Memory->RenderListUsed = (u32)RenderArena.Used;
    Memory->RenderListBitmapCount = RenderGroup.BitmapCount;
}
