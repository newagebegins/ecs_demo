#include "game.h"

#include "render_group.cpp"

internal void
SpriteSystem(ecs *ECS, render_group *RenderGroup, r32 dt)
{
    for(u32 Entity = 0;
        Entity < ECS->EntityCount;
        ++Entity)
    {
        if(ECS->ComponentMasks[Entity] & ComponentMask_Sprite)
        {
            sprite *Sprite = ECS->Sprites + Entity;
            bitmap_info *Info = RenderGroup->BitmapInfos + Sprite->BitmapID;

            Sprite->FrameTimer += dt;
            if(Sprite->FrameTimer >= Sprite->FrameDuration)
            {
                Sprite->FrameTimer -= Sprite->FrameDuration;
                Sprite->FrameIndex = (Sprite->FrameIndex + 1) % Info->FrameCount;
            }

            PushBitmap(RenderGroup, Sprite->BitmapID,
                       (s32)(ECS->Positions[Entity].x - (r32)Info->FrameWidth*0.5f),
                       (s32)(ECS->Positions[Entity].y - (r32)Info->FrameHeight*0.5f),
                       Sprite->FrameIndex, false, Sprite->Color);
        }
    }
}

inline v2
AddAndWrap(v2 A, v2 B)
{
    v2 Result = {AddModN(A.x, B.x, BACKBUFFER_WIDTH),
                 AddModN(A.y, B.y, BACKBUFFER_HEIGHT)};
    return(Result);
}

#define PIXELS_PER_METER (16.0f / 1.5f)
#define METERS_PER_PIXEL (1.0f / PIXELS_PER_METER)
#define GRAVITY_METRIC 9.8f
#define GRAVITY_PIXEL (GRAVITY_METRIC * PIXELS_PER_METER)

internal void
MovementSystem(ecs *ECS, r32 dt)
{
    for(u32 Entity = 0;
        Entity < ECS->EntityCount;
        ++Entity)
    {
        if(ECS->ComponentMasks[Entity] & ComponentMask_RigidBody)
        {
            rigid_body *Body = ECS->RigidBodies + Entity;

            v2 Friction = -Normalize(Body->Velocity) * Body->FrictionCoeff * GRAVITY_PIXEL;
            Body->Acceleration += Friction;

            Body->Velocity += dt*Body->Acceleration;
            ECS->Positions[Entity] = AddAndWrap(ECS->Positions[Entity], dt*Body->Velocity);

            Body->Acceleration = {};
        }
    }
}

internal void
AddCollisionEvent(ecs *ECS, u32 EntityA, u32 EntityB, v2 SeparationVector)
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

    for(u32 Entity = 0;
        Entity < ECS->EntityCount;
        ++Entity)
    {
        s32 CellX = (s32)(ECS->Positions[Entity].x / (r32)CELL_SIZE);
        s32 CellY = (s32)(ECS->Positions[Entity].y / (r32)CELL_SIZE);

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

    for(u32 EntityA = 0;
        EntityA < ECS->EntityCount;
        ++EntityA)
    {
        s32 CellX = (s32)(ECS->Positions[EntityA].x / (r32)CELL_SIZE);
        s32 CellY = (s32)(ECS->Positions[EntityA].y / (r32)CELL_SIZE);

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

                for(u32 CellEntityIndex = 0;
                    CellEntityIndex < Cell->EntityCount;
                    ++CellEntityIndex)
                {
                    u32 EntityB = Cell->Entities[CellEntityIndex];

                    if(EntityA < EntityB)
                    {
                        ++CheckCount;

                        r32 DiffX = ECS->Positions[EntityA].x - ECS->Positions[EntityB].x;
                        r32 DiffY = ECS->Positions[EntityA].y - ECS->Positions[EntityB].y;

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

                        r32 OverlapX = (ECS->HalfDims[EntityA].x + ECS->HalfDims[EntityB].x) - DistanceX;
                        r32 OverlapY = (ECS->HalfDims[EntityA].y + ECS->HalfDims[EntityB].y) - DistanceY;

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
    for(u32 EventIndex = 0;
        EventIndex < ECS->CollisionEventsCount;
        ++EventIndex)
    {
        collision_event *Event = ECS->CollisionEvents + EventIndex;

        rigid_body *BodyA = ECS->RigidBodies + Event->EntityA;
        rigid_body *BodyB = ECS->RigidBodies + Event->EntityB;

        r32 TotalInvMass = BodyA->InvMass + BodyB->InvMass;
        Assert(TotalInvMass != 0.0f);

        r32 SpringStiffness = 500.0f;
        v2 SpringForce = Event->SeparationVector * SpringStiffness;

        BodyA->Acceleration += SpringForce * (BodyA->InvMass / TotalInvMass);
        BodyB->Acceleration -= SpringForce * (BodyB->InvMass / TotalInvMass);
    }
}

inline u32
AddEntity(ecs *ECS)
{
    u32 ID;
    if(ECS->EntityCount < MAX_ENTITY_COUNT)
    {
        ID = ECS->EntityCount++;
    }
    else
    {
        ID = 0;
        Assert(!"No room for a new entity");
    }
    return(ID);
}

internal void
BomberSystem(ecs *ECS, r32 dt)
{
    for(u32 Entity = 0;
        Entity < ECS->EntityCount;
        ++Entity)
    {
        if(ECS->ComponentMasks[Entity] & ComponentMask_Bomber)
        {
            if(RandomChoice(ECS->RandomSeries, 2000) == 7)
            {
                u32 BombID = AddEntity(ECS);

                ECS->ComponentMasks[BombID] = (ComponentMask_Position|
                                               ComponentMask_RigidBody|
                                               ComponentMask_HalfDim|
                                               ComponentMask_Sprite);

                ECS->Positions[BombID] = ECS->Positions[Entity];
                ECS->RigidBodies[BombID].Velocity = 10.0f*RandomDirection(ECS->RandomSeries);
                ECS->RigidBodies[BombID].InvMass = 1.0f / 10.0f;
                ECS->RigidBodies[BombID].FrictionCoeff = 0.8f;

                ECS->HalfDims[BombID] = V2(6.0f, 6.0f);

                sprite *Sprite = ECS->Sprites + BombID;
                Sprite->BitmapID = Bitmap_Bomb;
                Sprite->FrameTimer = 0.0f;
                Sprite->FrameDuration = 0.0f;
                Sprite->FrameIndex = 0;
                Sprite->Color = Color_Red;
            }
        }
    }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    BEGIN_TIMED_BLOCK(GameUpdateAndRender);

    DebugGameMemory = Memory;

    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    game_state *GameState = (game_state *)Memory->PermanentStorage;

    ecs *ECS = &GameState->ECS;

    if(!GameState->IsInitialized)
    {
        GameState->MainArena = MakeArena(GameState + 1, Memory->PermanentStorageSize - sizeof(game_state));
        GameState->BitmapInfos = BitmapInfos;
        GameState->GeneralEntropy = RandomSeries(17);

        GameState->ECS.RandomSeries = &GameState->GeneralEntropy;

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
                    u32 Entity = AddEntity(ECS);

                    ECS->ComponentMasks[Entity] = (ComponentMask_Position|
                                                   ComponentMask_RigidBody|
                                                   ComponentMask_HalfDim|
                                                   ComponentMask_Sprite|
                                                   ComponentMask_Bomber);

                    ECS->Positions[Entity] = V2(X, Y);
                    ECS->RigidBodies[Entity].Velocity = 20.0f*RandomDirection(ECS->RandomSeries);
                    ECS->RigidBodies[Entity].InvMass = 1.0f / 50.0f;

                    ECS->HalfDims[Entity] = V2(5.0f, 6.0f);

                    sprite *Sprite = ECS->Sprites + Entity;
                    Sprite->BitmapID = Bitmap_Guy;
                    Sprite->FrameTimer = 0.0f;
                    Sprite->FrameDuration = 0.2f;
                    Sprite->FrameIndex = 0;
                    Sprite->Color = Color_White;
                }
                else if(Choice == 3 || Choice == 4 || Choice == 5 || Choice == 6)
                {
                    u32 Entity = AddEntity(ECS);

                    ECS->ComponentMasks[Entity] = (ComponentMask_Position|
                                                   ComponentMask_HalfDim|
                                                   ComponentMask_Sprite);

                    ECS->Positions[Entity] = V2(X, Y);
                    ECS->HalfDims[Entity] = V2(8.0f, 8.0f);

                    sprite *Sprite = ECS->Sprites + Entity;
                    Sprite->BitmapID = Bitmap_Wall;
                    Sprite->FrameTimer = 0.0f;
                    Sprite->FrameDuration = 0.0f;
                    Sprite->FrameIndex = 0;
                    Sprite->Color = Color_Cyan;
                }
            }
        }

        GameState->IsInitialized = true;
    }

    memory_arena RenderArena = MakeArena(Memory->RenderList, Memory->RenderListSize);
    render_group RenderGroup;
    InitializeRenderGroup(&RenderGroup, &RenderArena, GameState->BitmapInfos);
    PushClear(&RenderGroup, Color_Black);

    MovementSystem(ECS, Input->dt);
    BomberSystem(ECS, Input->dt);
    CollisionDetectionSystem(ECS);
    CollisionResponseSystem(ECS);
    SpriteSystem(ECS, &RenderGroup, Input->dt);

    Memory->RenderListUsed = (u32)RenderArena.Used;
    Memory->RenderListBitmapCount = RenderGroup.BitmapCount;

    END_TIMED_BLOCK(GameUpdateAndRender);
}
