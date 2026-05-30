#include "game.h"

#include "render_group.cpp"

internal void
SpriteSystem(ecs *ECS, render_group *RenderGroup, r32 dt)
{
    for(s32 SpriteIndex = 0;
        SpriteIndex < ECS->sprite_comp_Pool.Count;
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

        entity_id EntityID = ECS->sprite_comp_Pool.DenseToEntity[SpriteIndex];
        s32 PositionIndex = ECS->position_comp_Pool.EntityToDense[EntityID.Value];
        Assert(PositionIndex >= 0);
        position_comp *Position = GetComp(ECS, position_comp, PositionIndex);

        PushBitmap(RenderGroup, Sprite->BitmapID,
                   (s32)(Position->Position.x - (r32)Info->FrameWidth*0.5f),
                   (s32)(Position->Position.y - (r32)Info->FrameHeight*0.5f),
                   Sprite->FrameIndex, false, Sprite->Color);

#if 0
        s32 HitboxIndex = ECS->hitbox_comp_Pool.EntityToDense[EntityID.Value];
        Assert(HitboxIndex >= 0);
        hitbox_comp *Hitbox = GetComp(ECS, hitbox_comp, HitboxIndex);
        v2 HitboxMin = Position->Position - Hitbox->HalfDim;
        v2 HitboxMax = Position->Position + Hitbox->HalfDim;
        PushRect(RenderGroup, (s32)HitboxMin.x, (s32)HitboxMin.y, (s32)HitboxMax.x, (s32)HitboxMax.y, Color_Cyan);
#endif
    }
}

inline v2
AddAndWrap(v2 A, v2 B)
{
    v2 Result = {AddModN(A.x, B.x, BACKBUFFER_WIDTH),
                 AddModN(A.y, B.y, BACKBUFFER_HEIGHT)};
    return(Result);
}

internal void
MovementSystem(ecs *ECS, r32 dt)
{
    for(s32 VelocityCompIndex = 0;
        VelocityCompIndex < ECS->velocity_comp_Pool.Count;
        ++VelocityCompIndex)
    {
        velocity_comp *VelocityComp = GetComp(ECS, velocity_comp, VelocityCompIndex);

        entity_id EntityID = ECS->velocity_comp_Pool.DenseToEntity[VelocityCompIndex];
        s32 PositionCompIndex = ECS->position_comp_Pool.EntityToDense[EntityID.Value];
        Assert(PositionCompIndex >= 0);
        position_comp *PositionComp = GetComp(ECS, position_comp, PositionCompIndex);
        PositionComp->Position = AddAndWrap(PositionComp->Position, VelocityComp->Velocity * dt);
    }
}

internal void
AddCollisionEvent(ecs *ECS, entity_id EntityA, entity_id EntityB, v2 SeparationVector)
{
    if(ECS->CollisionEventsCount < ArrayCount(ECS->CollisionEvents))
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
    BEGIN_TIMED_BLOCK(CollisionDetectionSystem);

    BEGIN_TIMED_BLOCK(PopulateGrid);

    for(u32 CellY = 0;
        CellY < CELL_COUNT_Y;
        ++CellY)
    {
        for(u32 CellX = 0;
            CellX < CELL_COUNT_X;
            ++CellX)
        {
            ECS->Grid[CellY][CellX].EntityCount = 0;
        }
    }

    for(s32 PositionIndex = 0;
        PositionIndex < ECS->position_comp_Pool.Count;
        ++PositionIndex)
    {
        position_comp *Position = GetComp(ECS, position_comp, PositionIndex);
        entity_id Entity = ECS->position_comp_Pool.DenseToEntity[PositionIndex];

        s32 CellX = (s32)(Position->Position.x / (r32)CELL_SIZE);
        s32 CellY = (s32)(Position->Position.y / (r32)CELL_SIZE);

        grid_cell *Cell = &ECS->Grid[CellY][CellX];
        if(Cell->EntityCount < ArrayCount(Cell->Entities))
        {
            Cell->Entities[Cell->EntityCount++] = Entity;
        }
        else
        {
            Assert(!"Too many entities in a grid cell");
        }
    }

    END_TIMED_BLOCK(PopulateGrid);

    BEGIN_TIMED_BLOCK(DetectCollisions);

    ECS->CollisionEventsCount = 0;
    u64 CheckCount = 0;

    BEGIN_TIMED_BLOCK(CheckCollision);

    for(s32 HitboxAIndex = 0;
        HitboxAIndex < ECS->hitbox_comp_Pool.Count;
        ++HitboxAIndex)
    {
        hitbox_comp *HitboxA = GetComp(ECS, hitbox_comp, HitboxAIndex);

        entity_id EntityA = ECS->hitbox_comp_Pool.DenseToEntity[HitboxAIndex];
        s32 PositionAIndex = ECS->position_comp_Pool.EntityToDense[EntityA.Value];
        Assert(PositionAIndex >= 0);
        position_comp *PositionA = GetComp(ECS, position_comp, PositionAIndex);

        s32 CellX = (s32)(PositionA->Position.x / (r32)CELL_SIZE);
        s32 CellY = (s32)(PositionA->Position.y / (r32)CELL_SIZE);

        for(s32 DeltaY = -1;
            DeltaY <= 1;
            ++DeltaY)
        {
            s32 TestY = ModuloN(CellY + DeltaY, CELL_COUNT_Y);

            for(s32 DeltaX = -1;
                DeltaX <= 1;
                ++DeltaX)
            {
                s32 TestX = ModuloN(CellX + DeltaX, CELL_COUNT_X);

                grid_cell *Cell = &ECS->Grid[TestY][TestX];

                for(u32 EntityIndex = 0;
                    EntityIndex < Cell->EntityCount;
                    ++EntityIndex)
                {
                    entity_id EntityB = Cell->Entities[EntityIndex];

                    if(EntityA.Value < EntityB.Value)
                    {
                        ++CheckCount;

                        s32 HitboxBIndex = ECS->hitbox_comp_Pool.EntityToDense[EntityB.Value];
                        Assert(HitboxBIndex >= 0);
                        hitbox_comp *HitboxB = GetComp(ECS, hitbox_comp, HitboxBIndex);                    

                        s32 PositionBIndex = ECS->position_comp_Pool.EntityToDense[EntityB.Value];
                        Assert(PositionBIndex >= 0);
                        position_comp *PositionB = GetComp(ECS, position_comp, PositionBIndex);

                        r32 DiffX = PositionA->Position.x - PositionB->Position.x;
                        r32 DiffY = PositionA->Position.y - PositionB->Position.y;

                        r32 DistanceX = AbsoluteValue(DiffX);
                        r32 DistanceY = AbsoluteValue(DiffY);

                        r32 SeparationDirX = SignOf(DiffX);
                        r32 SeparationDirY = SignOf(DiffY);

                        Assert(DistanceX < BACKBUFFER_WIDTH);
                        Assert(DistanceY < BACKBUFFER_HEIGHT);

                        if(DistanceX > 0.5f*BACKBUFFER_WIDTH)
                        {
                            DistanceX = BACKBUFFER_WIDTH - DistanceX;
                            SeparationDirX = -SeparationDirX;
                        }
                        if(DistanceY > 0.5f*BACKBUFFER_HEIGHT)
                        {
                            DistanceY = BACKBUFFER_HEIGHT - DistanceY;
                            SeparationDirY = -SeparationDirY;
                        }

                        r32 OverlapX = (HitboxA->HalfDim.x + HitboxB->HalfDim.x) - DistanceX;
                        r32 OverlapY = (HitboxA->HalfDim.y + HitboxB->HalfDim.y) - DistanceY;

                        if(OverlapX > 0.0f && OverlapY > 0.0f)
                        {
                            v2 SeparationVector;
                            if(OverlapX < OverlapY)
                            {
                                SeparationVector = {OverlapX * SeparationDirX, 0.0f};
                            }
                            else
                            {
                                SeparationVector = {0.0f, OverlapY * SeparationDirY};
                            }
                            AddCollisionEvent(ECS, EntityA, EntityB, SeparationVector);
                        }
                    }
                }
            }
        }
    }

    END_TIMED_BLOCK_COUNTED(CheckCollision, CheckCount);

    END_TIMED_BLOCK(DetectCollisions);

    END_TIMED_BLOCK(CollisionDetectionSystem);
}

internal void
CollisionResponseSystem(ecs *ECS)
{
    ZeroSize(ECS->WasPushedThisFrame, MAX_ENTITY_COUNT*sizeof(ECS->WasPushedThisFrame[0]));

    for(u32 EventIndex = 0;
        EventIndex < ECS->CollisionEventsCount;
        ++EventIndex)
    {
        collision_event *Event = ECS->CollisionEvents + EventIndex;

        s32 PositionAIndex = ECS->position_comp_Pool.EntityToDense[Event->EntityA.Value];
        Assert(PositionAIndex >= 0);
        position_comp *PositionA = GetComp(ECS, position_comp, PositionAIndex);

        s32 PositionBIndex = ECS->position_comp_Pool.EntityToDense[Event->EntityB.Value];
        Assert(PositionBIndex >= 0);
        position_comp *PositionB = GetComp(ECS, position_comp, PositionBIndex);

        s32 VelocityAIndex = ECS->velocity_comp_Pool.EntityToDense[Event->EntityA.Value];
        velocity_comp *VelocityA = (VelocityAIndex >= 0) ? GetComp(ECS, velocity_comp, VelocityAIndex) : 0;

        s32 VelocityBIndex = ECS->velocity_comp_Pool.EntityToDense[Event->EntityB.Value];
        velocity_comp *VelocityB = (VelocityBIndex >= 0) ? GetComp(ECS, velocity_comp, VelocityBIndex) : 0;

        Assert(VelocityA || VelocityB);

        v2 SeparationVector = Event->SeparationVector;

        if(VelocityA && VelocityB)
        {
            SeparationVector *= 0.5f;
        }

        r32 VelXScale;
        r32 VelYScale;

        if(SeparationVector.x != 0.0f)
        {
            VelXScale = -1.0f;
            VelYScale = 1.0f;
        }
        else
        {
            VelXScale = 1.0f;
            VelYScale = -1.0f;
        }

        if(VelocityA && !ECS->WasPushedThisFrame[Event->EntityA.Value])
        {
            PositionA->Position = AddAndWrap(PositionA->Position, SeparationVector);
            VelocityA->Velocity.x *= VelXScale;
            VelocityA->Velocity.y *= VelYScale;
            ECS->WasPushedThisFrame[Event->EntityA.Value] = true;
        }

        if(VelocityB && !ECS->WasPushedThisFrame[Event->EntityB.Value])
        {
            PositionB->Position = AddAndWrap(PositionB->Position, -SeparationVector);
            VelocityB->Velocity.x *= VelXScale;
            VelocityB->Velocity.y *= VelYScale;
            ECS->WasPushedThisFrame[Event->EntityB.Value] = true;
        }
    }
}

internal void
InitializeCompPool(memory_arena *Arena, comp_pool *Pool, u32 CompSize)
{
    Pool->Dense = PushSize(Arena, MAX_ENTITY_COUNT*CompSize);

    for(u32 Index = 0;
        Index < MAX_ENTITY_COUNT;
        ++Index)
    {
        Pool->EntityToDense[Index] = -1;
        Pool->DenseToEntity[Index].Value = -1;
    }
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

#define AddComponent(ECS, EntityID, CompType) (CompType *)AddComponent_(&ECS.CompType##_Pool, EntityID, sizeof(CompType))

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    BEGIN_TIMED_BLOCK(GameUpdateAndRender);

    DebugGameMemory = Memory;

    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!GameState->IsInitialized)
    {
        GameState->MainArena = MakeArena(GameState + 1, Memory->PermanentStorageSize - sizeof(game_state));
        GameState->BitmapInfos = BitmapInfos;
        GameState->GeneralEntropy = RandomSeries(17);

        InitializeCompPool(&GameState->MainArena, &GameState->ECS.position_comp_Pool, sizeof(position_comp));
        InitializeCompPool(&GameState->MainArena, &GameState->ECS.velocity_comp_Pool, sizeof(velocity_comp));
        InitializeCompPool(&GameState->MainArena, &GameState->ECS.hitbox_comp_Pool, sizeof(hitbox_comp));
        InitializeCompPool(&GameState->MainArena, &GameState->ECS.sprite_comp_Pool, sizeof(sprite_comp));

        u32 CellSize = 16;
        r32 CellHalfDim = (r32)(CellSize/2);
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
                r32 X = (r32)(CellX*CellSize) + CellHalfDim;
                r32 Y = (r32)(CellY*CellSize) + CellHalfDim;

                u32 Choice = RandomChoice(&GameState->GeneralEntropy, 16);
                if(Choice == 1 || Choice == 2)
                {
                    entity_id EntityID = AddEntity(&GameState->ECS);

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
                    SpriteComp->Color = Color_White;
                }
                else if(Choice == 3 || Choice == 4 || Choice == 5 || Choice == 6)
                {
                    entity_id EntityID = AddEntity(&GameState->ECS);

                    position_comp *PositionComp = AddComponent(GameState->ECS, EntityID, position_comp);
                    PositionComp->Position = V2(X, Y);

                    hitbox_comp *HitboxComp = AddComponent(GameState->ECS, EntityID, hitbox_comp);
                    HitboxComp->HalfDim = V2(8.0f, 8.0f);

                    sprite_comp *SpriteComp = AddComponent(GameState->ECS, EntityID, sprite_comp);
                    SpriteComp->BitmapID = Bitmap_Wall;
                    SpriteComp->FrameTimer = 0.0f;
                    SpriteComp->FrameDuration = 0.0f;
                    SpriteComp->FrameIndex = 0;
                    SpriteComp->Color = Color_Cyan;
                }
            }
        }

        GameState->IsInitialized = true;
    }

    memory_arena RenderArena = MakeArena(Memory->RenderList, Memory->RenderListSize);
    render_group RenderGroup;
    InitializeRenderGroup(&RenderGroup, &RenderArena, GameState->BitmapInfos);
    PushClear(&RenderGroup, Color_Black);

    MovementSystem(&GameState->ECS, Input->dt);
    CollisionDetectionSystem(&GameState->ECS);
    CollisionResponseSystem(&GameState->ECS);
    SpriteSystem(&GameState->ECS, &RenderGroup, Input->dt);

    Memory->RenderListUsed = (u32)RenderArena.Used;
    Memory->RenderListBitmapCount = RenderGroup.BitmapCount;

    END_TIMED_BLOCK(GameUpdateAndRender);
}
