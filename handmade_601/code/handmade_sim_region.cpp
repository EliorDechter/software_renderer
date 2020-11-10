/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

inline brain_id
AddBrain(sim_region *Region)
{
    brain_id Result = AddBrain(Region->World);
    return(Result);
}

inline entity_id
AllocateEntityID(sim_region *Region)
{
    entity_id Result = AllocateEntityID(Region->World);
    return(Result);
}

inline entity_traversable_point *
GetTraversable(entity *Entity, u32 Index)
{
    entity_traversable_point *Result = 0;
    if(Entity)
    {
        Assert(Index < Entity->TraversableCount);
        Result = Entity->Traversables + Index;
    }
    return(Result);
}

inline entity_traversable_point *
GetTraversable(sim_region *SimRegion, traversable_reference Reference)
{
    entity_traversable_point *Result = GetTraversable(GetEntityByID(SimRegion, Reference.Entity), Reference.Index);
    return(Result);
}

inline entity_traversable_point
GetSimSpaceTraversable(entity *Entity, u32 Index)
{
    entity_traversable_point Result = {};
    if(Entity)
    {
        Result.P = Entity->P;
        
        entity_traversable_point *Point = GetTraversable(Entity, Index);
        if(Point)
        {
            // TODO(casey): This wants to be rotated eventually!
            Result.P += Point->P;
            Result.Occupier = Point->Occupier;
        }
    }
    
    return(Result);
}

inline entity_traversable_point
GetSimSpaceTraversable(sim_region *SimRegion, traversable_reference Reference)
{
    entity_traversable_point Result =
        GetSimSpaceTraversable(GetEntityByID(SimRegion, Reference.Entity), Reference.Index);
    return(Result);
}

internal void
MarkBit(u64 *Array, umm Index)
{
    umm OccIndex = Index / 64;
    umm BitIndex = Index % 64;
    Array[OccIndex] |= ((u64)1 << BitIndex);
}

internal b32x
IsEmpty(u64 *Array, umm Index)
{
    umm OccIndex = Index / 64;
    umm BitIndex = Index % 64;
    b32x Result = !(Array[OccIndex] & ((u64)1 << BitIndex));
    return(Result);
}

internal void
MarkOccupied(sim_region *SimRegion, entity_hash *Entry)
{
    umm Index = Entry - SimRegion->EntityHash;
    MarkBit(SimRegion->EntityHashOccupancy, Index);
}

internal void
MarkOccupied(sim_region *SimRegion, brain_hash *Entry)
{
    umm Index = Entry - SimRegion->BrainHash;
    MarkBit(SimRegion->BrainHashOccupancy, Index);
}

internal entity_hash *
GetHashFromID(sim_region *SimRegion, entity_id StorageIndex)
{
    Assert(StorageIndex.Value);
    
    entity_hash *Result = 0;
    
    uint32 HashValue = StorageIndex.Value;
    for(uint32 Offset = 0;
        Offset < ArrayCount(SimRegion->EntityHash);
        ++Offset)
    {
        uint32 HashMask = (ArrayCount(SimRegion->EntityHash) - 1);
        uint32 HashIndex = ((HashValue + Offset) & HashMask);
        entity_hash *Entry = SimRegion->EntityHash + HashIndex;
        if(IsEmpty(SimRegion->EntityHashOccupancy, HashIndex))
        {
            Result = Entry;
            Result->Ptr = 0;
            break;
        }
        else if(Entry->Ptr->ID.Value == StorageIndex.Value)
        {
            Result = Entry;
            break;
        }
    }
    
    Assert(Result);
    return(Result);
}

internal brain_hash *
GetHashFromID(sim_region *SimRegion, brain_id StorageIndex)
{
    Assert(StorageIndex.Value);
    
    brain_hash *Result = 0;
    
    uint32 HashValue = StorageIndex.Value;
    for(uint32 Offset = 0;
        Offset < ArrayCount(SimRegion->BrainHash);
        ++Offset)
    {
        uint32 HashMask = (ArrayCount(SimRegion->BrainHash) - 1);
        uint32 HashIndex = ((HashValue + Offset) & HashMask);
        brain_hash *Entry = SimRegion->BrainHash + HashIndex;
        if(IsEmpty(SimRegion->BrainHashOccupancy, HashIndex))
        {
            Result = Entry;
            Result->Ptr = 0;
            break;
        }
        else if(Entry->Ptr->ID.Value == StorageIndex.Value)
        {
            Result = Entry;
            break;
        }
    }
    
    Assert(Result);
    return(Result);
}

inline entity *
GetEntityByID(sim_region *SimRegion, entity_id ID)
{
    entity *Result = 0;
    if(ID.Value)
    {
        entity_hash *Entry = GetHashFromID(SimRegion, ID);
        Result = Entry ? Entry->Ptr : 0;
    }
    
    return(Result);
}

inline b32x
EntityOverlapsRectangle(v3 P, rectangle3 EntityVolume, rectangle3 Rect)
{
    b32x Result = RectanglesIntersect(Offset(EntityVolume, P), Rect);
    
    return(Result);
}

inline b32x
EntityOverlapsEntity(entity *A, entity *B)
{
    b32x Result = RectanglesIntersect(Offset(A->CollisionVolume, A->P),
                                      Offset(B->CollisionVolume, B->P));
    
    return(Result);
}

internal brain *
GetOrAddBrain(sim_region *SimRegion, brain_id ID, brain_type Type)
{
    brain *Result = 0;
    
    brain_hash *Hash = GetHashFromID(SimRegion, ID);
    Result = Hash->Ptr;
    
    if(!Result)
    {
        Assert(SimRegion->BrainCount < SimRegion->MaxBrainCount);
        Assert(IsEmpty(SimRegion->BrainHashOccupancy, Hash - SimRegion->BrainHash));
        Result = SimRegion->Brains + SimRegion->BrainCount++;
        ZeroStruct(*Result);
        Result->ID = ID;
        Result->Type = Type;
        
        Hash->Ptr = Result;
        
        MarkOccupied(SimRegion, Hash);
    }
    
    return(Result);
}

inline void
AddEntityToHash(sim_region *Region, entity *Entity)
{
    entity_hash *Entry = GetHashFromID(Region, Entity->ID);
    Assert(IsEmpty(Region->EntityHashOccupancy, Entry - Region->EntityHash));
    Entry->Ptr = Entity;
    MarkOccupied(Region, Entry);
}

inline v3
MapIntoSimSpace(sim_region *SimRegion, world_position WorldPos)
{
    v3 Result = Subtract(SimRegion->World, &WorldPos, &SimRegion->Origin);
    return(Result);
}

internal void
RegisterEntity(sim_region *SimRegion, entity *Entity)
{
    if(EntityOverlapsRectangle(Entity->P, Entity->CollisionVolume, SimRegion->Bounds))
    {
        AddEntityToHash(SimRegion, Entity);
        
        if(EntityOverlapsRectangle(Entity->P, Entity->CollisionVolume, SimRegion->UpdatableBounds))
        {
            Entity->Flags |= EntityFlag_Active;
        }
        else
        {
            Entity->Flags &= ~EntityFlag_Active;
        }
        
        if(Entity->BrainID.Value)
        {
            brain *Brain = GetOrAddBrain(SimRegion, Entity->BrainID, (brain_type)Entity->BrainSlot.Type);
            u8 *Ptr = (u8 *)&Brain->Array;
            Ptr += sizeof(entity *)*Entity->BrainSlot.Index;
            Assert(Ptr <= ((u8 *)Brain + sizeof(brain) - sizeof(entity *)));
            *((entity **)Ptr) = Entity;
        }
    }
}

internal sim_region *
BeginWorldChange(game_assets *Assets, memory_arena *SimArena, world *World, world_position Origin, rectangle3 Bounds, real32 dt)
{
    TIMED_FUNCTION();
    
    // TODO(casey): If entities were stored in the world, we wouldn't need the game state here!
    
    BEGIN_BLOCK("SimArenaAlloc");
    sim_region *SimRegion = PushStruct(SimArena, sim_region, NoClear());
    END_BLOCK();
    
    BEGIN_BLOCK("SimArenaClear");
    ZeroStruct(SimRegion->EntityHashOccupancy);
    ZeroStruct(SimRegion->BrainHashOccupancy);
    END_BLOCK();
    
    SimRegion->World = World;
    
    SimRegion->Origin = Origin;
    SimRegion->Bounds = Bounds;
    SimRegion->UpdatableBounds = SimRegion->Bounds;
    
    SimRegion->MaxBrainCount = 512;
    SimRegion->BrainCount = 0;
    SimRegion->Brains = PushArray(SimArena, SimRegion->MaxBrainCount, brain, NoClear());
    
    world_position MinChunkP = MapIntoChunkSpace(World, SimRegion->Origin, GetMinCorner(SimRegion->Bounds));
    world_position MaxChunkP = MapIntoChunkSpace(World, SimRegion->Origin, GetMaxCorner(SimRegion->Bounds));
    
    DEBUG_VALUE(SimRegion->Origin.ChunkX);
    DEBUG_VALUE(SimRegion->Origin.ChunkY);
    DEBUG_VALUE(SimRegion->Origin.ChunkZ);
    DEBUG_VALUE(SimRegion->Origin.Offset_);
    
    EnsureRegionIsUnpacked(Assets, World, MinChunkP, MaxChunkP, SimRegion);
    
    return(SimRegion);
}

internal void
EndWorldChange(sim_region *Region)
{
    world *World = Region->World;
    
    // TODO(casey): Maybe use the new expected camera position here instead?
    world_position MinChunkP = MapIntoChunkSpace(World, Region->Origin, GetMinCorner(Region->Bounds));
    world_position MaxChunkP = MapIntoChunkSpace(World, Region->Origin, GetMaxCorner(Region->Bounds));
    RepackEntitiesAsNecessary(World, MinChunkP, MaxChunkP);
}

inline entity *
CreateEntity(sim_region *Region, entity_id ID)
{
    entity *Result = AcquireUnpackedEntitySlot(Region->World);
    ZeroStruct(*Result);
    
    Result->ID = ID;
    AddEntityToHash(Region, Result);
    
    return(Result);
}

inline void
DeleteEntity(sim_region *Region, entity *Entity)
{
    if(Entity)
    {
        Entity->Flags |= EntityFlag_Deleted;
    }
}

internal b32x
IsRoom(entity *Entity)
{
    b32x Result = (Entity->BrainSlot.Type == Type_brain_room);
    return(Result);
}

internal void
UpdateCameraForEntityMovement(game_camera *Camera, sim_region *Region, world *World,
                              entity *Entity, f32 dt, v3 CameraZ)
{
    Assert(Entity->ID.Value == Camera->FollowingEntityIndex.Value);
    
    // TODO(casey): Probably don't want to loop over all entities - maintain a
    // separate list of room entities during unpack?
    entity *InRoom = 0;
    entity *SpecialCamera = 0;
    for(entity_iterator Iter = IterateAllEntities(Region);
        Iter.Entity;
        Advance(&Iter))
    {
        entity *TestEntity = Iter.Entity;
        
        if(IsRoom(TestEntity))
        {
            if(EntityOverlapsEntity(Entity, TestEntity))
            {
                InRoom = TestEntity;
            }
        }
        
        if(TestEntity->CameraBehavior)
        {
            b32x Pass = EntityOverlapsEntity(Entity, TestEntity);
            if(TestEntity->CameraBehavior & Camera_GeneralVelocityConstraint)
            {
                Pass = Pass && IsInRange(TestEntity->CameraMinVelocity,
                                         Length(Entity->dP),
                                         TestEntity->CameraMaxVelocity);
            }
            
            if(TestEntity->CameraBehavior & Camera_DirectionalVelocityConstraint)
            {
                Pass = Pass && IsInRange(TestEntity->CameraMinVelocity,
                                         Inner(TestEntity->CameraVelocityDirection, Entity->dP),
                                         TestEntity->CameraMaxVelocity);
            }
            
            if(Pass)
            {
                if((TestEntity != InRoom) ||
                   (!SpecialCamera))
                {
                    SpecialCamera = TestEntity;
                }
            }
        }
    }
    
    if(SpecialCamera)
    {
        if(IsEqual(Camera->InSpecial, SpecialCamera->ID))
        {
            Camera->tInSpecial += dt;
        }
        else
        {
            Camera->InSpecial = SpecialCamera->ID;
            Camera->tInSpecial = 0.0f;
        }
    }
    else
    {
        Clear(&Camera->InSpecial);
    }
    
    v3 P = Subtract(World, &Camera->P, &Region->Origin);
    v3 TargetP = Subtract(World, &Camera->TargetP, &Region->Origin);
    v3 NewTargetP = TargetP;
    
    f32 EntityFocusZ = 8.0f;
    f32 RoomFocusZ = EntityFocusZ;
    if(InRoom)
    {
        rectangle3 RoomVolume = Offset(InRoom->CollisionVolume, InRoom->P);
        
        v3 SimulationCenter = V3(GetCenter(RoomVolume).xy, RoomVolume.Min.z);
        Camera->SimulationCenter = MapIntoChunkSpace(World, Region->Origin, SimulationCenter);
        
        NewTargetP = SimulationCenter;
        f32 TargetOffsetZ = InRoom->CameraOffset.z;
        
        if(SpecialCamera && (Camera->tInSpecial > SpecialCamera->CameraMinTime))
        {
            if(SpecialCamera->CameraBehavior & Camera_Inspect)
            {
                NewTargetP = SpecialCamera->P;
            }
            
            if(SpecialCamera->CameraBehavior & Camera_ViewPlayerX)
            {
                NewTargetP.x = Entity->P.x;
            }
            
            if(SpecialCamera->CameraBehavior & Camera_ViewPlayerY)
            {
                NewTargetP.y = Entity->P.y;
            }
            
            if(SpecialCamera->CameraBehavior & Camera_Offset)
            {
                NewTargetP = SpecialCamera->P;
                NewTargetP.xy += SpecialCamera->CameraOffset.xy;
                
                TargetOffsetZ = SpecialCamera->CameraOffset.z;
                
                Camera->MovementMask = V3(1, 1, 1);
            }
        }
        
        v3 CameraArm = TargetOffsetZ*CameraZ;
        NewTargetP += CameraArm;
        
        RoomFocusZ = CameraArm.z;
        EntityFocusZ = (NewTargetP - Entity->GroundP).z;
    }
    
    Camera->TargetExpectedFocusMinZ = Minimum(RoomFocusZ, EntityFocusZ);
    Camera->TargetExpectedFocusMaxZ = Maximum(RoomFocusZ, EntityFocusZ);
    
    f32 A = 10.0f;
    f32 B = 5.25f;
    
    TargetP = NewTargetP;
    
    v3 Threshold = {4.0f, 2.0f, 1.0f};
    
    v3 TotalDeltaP = TargetP - P;
    
    if(AbsoluteValue(TotalDeltaP.E[0]) > Threshold.E[0]) {Camera->MovementMask.E[0] = 1.0f;}
    if(AbsoluteValue(TotalDeltaP.E[1]) > Threshold.E[1]) {Camera->MovementMask.E[1] = 1.0f;}
    if(AbsoluteValue(TotalDeltaP.E[2]) > Threshold.E[2]) {Camera->MovementMask.E[2] = 1.0f;}
    
    {DEBUG_DATA_BLOCK("Renderer/Pre");
        DEBUG_VALUE(P);
        DEBUG_VALUE(TargetP);
        DEBUG_VALUE(TotalDeltaP);
        DEBUG_VALUE(Camera->MovementMask);
    }
    
    TotalDeltaP = Hadamard(TotalDeltaP, Camera->MovementMask);
    
    v3 dPTarget = V3(0, 0, 0);
    v3 TotalDeltadP = (dPTarget - Camera->dP);
    TotalDeltadP = Hadamard(TotalDeltadP, Camera->MovementMask);
    
    v3 ddP = A*TotalDeltaP + B*TotalDeltadP;
    v3 dP = dt*ddP + Camera->dP;
    
    v3 DeltaP = 0.5f*dt*dt*ddP + dt*Camera->dP;
    if((LengthSq(DeltaP) < LengthSq(TotalDeltaP)) ||
       (LengthSq(Hadamard(dPTarget - dP, Camera->MovementMask)) < Square(0.001f)))
    {
        P += DeltaP;
    }
    else
    {
        P += TotalDeltaP;
        dP = dPTarget;
        ddP = V3(0, 0, 0);
        Camera->MovementMask = V3(0, 0, 0);
    }
    
    {DEBUG_DATA_BLOCK("Renderer/Post");
        DEBUG_VALUE(TotalDeltaP);
        DEBUG_VALUE(DeltaP);
    }
    
    if(AbsoluteValue(TotalDeltaP.z) > 0.01f)
    {
        f32 DeltaPercent = DeltaP.z / TotalDeltaP.z;
        
        f32 MinZSpan = (Camera->TargetExpectedFocusMinZ - Camera->ExpectedFocusMinZ);
        f32 MaxZSpan = (Camera->TargetExpectedFocusMaxZ - Camera->ExpectedFocusMaxZ);
        Camera->ExpectedFocusMinZ += DeltaPercent*MinZSpan;
        Camera->ExpectedFocusMaxZ += DeltaPercent*MaxZSpan;
    }
    else
    {
        Camera->ExpectedFocusMinZ = Camera->TargetExpectedFocusMinZ;
        Camera->ExpectedFocusMaxZ = Camera->TargetExpectedFocusMaxZ;
    }
    
    Camera->P = MapIntoChunkSpace(World, Region->Origin, P);
    Camera->dP = dP;
    Camera->TargetP = MapIntoChunkSpace(World, Region->Origin, TargetP);
}

struct test_wall
{
    real32 X;
    real32 RelX;
    real32 RelY;
    real32 DeltaX;
    real32 DeltaY;
    real32 MinY;
    real32 MaxY;
    v3 Normal;
};
internal bool32
TestWall(real32 WallX, real32 RelX, real32 RelY, real32 PlayerDeltaX, real32 PlayerDeltaY,
         real32 *tMin, real32 MinY, real32 MaxY)
{
    bool32 Hit = false;
    
    real32 tEpsilon = 0.001f;
    if(PlayerDeltaX != 0.0f)
    {
        real32 tResult = (WallX - RelX) / PlayerDeltaX;
        real32 Y = RelY + tResult*PlayerDeltaY;
        if((tResult >= 0.0f) && (*tMin > tResult))
        {
            if((Y >= MinY) && (Y <= MaxY))
            {
                *tMin = Maximum(0.0f, tResult - tEpsilon);
                Hit = true;
            }
        }
    }
    
    return(Hit);
}

internal bool32
CanCollide(entity *A, entity *B)
{
    b32 Result = false;
    
    if(A != B)
    {
        if(A->ID.Value > B->ID.Value)
        {
            entity *Temp = A;
            A = B;
            B = Temp;
        }
        
        if(IsSet(A, EntityFlag_Collides) && IsSet(B, EntityFlag_Collides))
        {
            if(HasArea(A->CollisionVolume) && HasArea(B->CollisionVolume))
            {
                Result = true;
            }
        }
    }
    
    return(Result);
}

internal bool32
HandleCollision(entity *A, entity *B)
{
    bool32 StopsOnCollision = true;
    
#if 0
    if(A->Type > B->Type)
    {
        entity *Temp = A;
        A = B;
        B = Temp;
    }
#endif
    
    return(StopsOnCollision);
}

internal bool32
CanOverlap(entity *Mover, entity *Region)
{
    bool32 Result = false;
    
    if(Mover != Region)
    {
#if 0
        if(Region->Type == EntityType_Stairwell)
        {
            Result = true;
        }
#endif
    }
    
    return(Result);
}

internal bool32
SpeculativeCollide(entity *Mover, entity *Region, v3 TestP)
{
    bool32 Result = true;
    
#if 0
    if(Region->Type == EntityType_Stairwell)
    {
        // TODO(casey): Needs work :)
        real32 StepHeight = 0.1f;
#if 0
        Result = ((AbsoluteValue(GetEntityGroundPoint(Mover).z - Ground) > StepHeight) ||
                  ((Bary.y > 0.1f) && (Bary.y < 0.9f)));
#endif
        v3 MoverGroundPoint = GetEntityGroundPoint(Mover, TestP);
        real32 Ground = GetStairGround(Region, MoverGroundPoint);
        Result = (AbsoluteValue(MoverGroundPoint.z - Ground) > StepHeight);
    }
#endif
    
    return(Result);
}

inline b32
IsOccupied(sim_region *SimRegion, traversable_reference Ref)
{
    b32 Result = true;
    
    entity_traversable_point *Dest = GetTraversable(SimRegion, Ref);
    if(Dest)
    {
        Result = (Dest->Occupier != 0);
    }
    
    return(Result);
}

internal b32
TransactionalOccupy(sim_region *SimRegion, entity *Entity, traversable_reference *DestRef, traversable_reference DesiredRef)
{
    b32 Result = false;
    
    entity_traversable_point *Desired = GetTraversable(SimRegion, DesiredRef);
    if(Desired && !Desired->Occupier)
    {
        entity_traversable_point *Dest = GetTraversable(SimRegion, *DestRef);
        if(Dest)
        {
            Dest->Occupier = 0;
        }
        *DestRef = DesiredRef;
        Desired->Occupier = Entity;
        Result = true;
    }
    
    return(Result);
}

internal void
MoveEntity(sim_region *SimRegion, entity *Entity, real32 dt, v3 ddP)
{
    world *World = SimRegion->World;
    
    v3 PlayerDelta = (0.5f*ddP*Square(dt) + Entity->dP*dt);
    Entity->dP = ddP*dt + Entity->dP;
    // TODO(casey): Upgrade physical motion routines to handle capping the
    // maximum velocity?
    //Assert(LengthSq(Entity->dP) <= Square(SimRegion->MaxEntityVelocity));
    
    real32 DistanceRemaining = Entity->DistanceLimit;
    if(DistanceRemaining == 0.0f)
    {
        // TODO(casey): Do we want to formalize this number?
        DistanceRemaining = 10000.0f;
    }
    
    for(uint32 Iteration = 0;
        Iteration < 4;
        ++Iteration)
    {
        real32 tMin = 1.0f;
        real32 tMax = 1.0f;
        
        real32 PlayerDeltaLength = Length(PlayerDelta);
        // TODO(casey): What do we want to do for epsilons here?
        // Think this through for the final collision code
        if(PlayerDeltaLength > 0.0f)
        {
            if(PlayerDeltaLength > DistanceRemaining)
            {
                tMin = (DistanceRemaining / PlayerDeltaLength);
            }
            
            v3 WallNormalMin = {};
            v3 WallNormalMax = {};
            entity *HitEntityMin = 0;
            entity *HitEntityMax = 0;
            
            v3 DesiredPosition = Entity->P + PlayerDelta;
            
            // NOTE(casey): This is just an optimization to avoid enterring the
            // loop in the case where the test entity is non-spatial!
            
            // TODO(casey): Spatial partition here!
            for(entity_iterator Iter = IterateAllEntities(SimRegion);
                Iter.Entity;
                Advance(&Iter))
            {
                entity *TestEntity = Iter.Entity;
                
                // TODO(casey): Robustness!
                real32 OverlapEpsilon = 0.001f;
                
                if(CanCollide(Entity, TestEntity))
                {
                    v3 VolumeDim = GetDim(Entity->CollisionVolume);
                    v3 VolumeP = GetCenter(Entity->CollisionVolume);
                    
                    v3 TestVolumeDim = GetDim(TestEntity->CollisionVolume);
                    v3 TestVolumeP = GetCenter(TestEntity->CollisionVolume);
                    
                    v3 MinkowskiDiameter = {TestVolumeDim.x + VolumeDim.x,
                        TestVolumeDim.y + VolumeDim.y,
                        TestVolumeDim.z + VolumeDim.z};
                    
                    v3 MinCorner = -0.5f*MinkowskiDiameter;
                    v3 MaxCorner = 0.5f*MinkowskiDiameter;
                    
                    v3 Rel = ((Entity->P + VolumeP) -
                              (TestEntity->P + TestVolumeP));
                    
                    // TODO(casey): Do we want an open inclusion at the MaxCorner?
                    if((Rel.z >= MinCorner.z) && (Rel.z < MaxCorner.z))
                    {
                        test_wall Walls[] =
                        {
                            {MinCorner.x, Rel.x, Rel.y, PlayerDelta.x, PlayerDelta.y, MinCorner.y, MaxCorner.y, V3(-1, 0, 0)},
                            {MaxCorner.x, Rel.x, Rel.y, PlayerDelta.x, PlayerDelta.y, MinCorner.y, MaxCorner.y, V3(1, 0, 0)},
                            {MinCorner.y, Rel.y, Rel.x, PlayerDelta.y, PlayerDelta.x, MinCorner.x, MaxCorner.x, V3(0, -1, 0)},
                            {MaxCorner.y, Rel.y, Rel.x, PlayerDelta.y, PlayerDelta.x, MinCorner.x, MaxCorner.x, V3(0, 1, 0)},
                        };
                        
                        real32 tMinTest = tMin;
                        bool32 HitThis = false;
                        
                        v3 TestWallNormal = {};
                        for(uint32 WallIndex = 0;
                            WallIndex < ArrayCount(Walls);
                            ++WallIndex)
                        {
                            test_wall *Wall = Walls + WallIndex;
                            
                            real32 tEpsilon = 0.001f;
                            if(Wall->DeltaX != 0.0f)
                            {
                                real32 tResult = (Wall->X - Wall->RelX) / Wall->DeltaX;
                                real32 Y = Wall->RelY + tResult*Wall->DeltaY;
                                if((tResult >= 0.0f) && (tMinTest > tResult))
                                {
                                    if((Y >= Wall->MinY) && (Y <= Wall->MaxY))
                                    {
                                        tMinTest = Maximum(0.0f, tResult - tEpsilon);
                                        TestWallNormal = Wall->Normal;
                                        HitThis = true;
                                    }
                                }
                            }
                        }
                        
                        // TODO(casey): We need a concept of stepping onto vs. stepping
                        // off of here so that we can prevent you from _leaving_
                        // stairs instead of just preventing you from getting onto them.
                        if(HitThis)
                        {
                            v3 TestP = Entity->P + tMinTest*PlayerDelta;
                            if(SpeculativeCollide(Entity, TestEntity, TestP))
                            {
                                tMin = tMinTest;
                                WallNormalMin = TestWallNormal;
                                HitEntityMin = TestEntity;
                            }
                        }
                    }
                }
            }
            
            v3 WallNormal;
            entity *HitEntity;
            real32 tStop;
            if(tMin < tMax)
            {
                tStop = tMin;
                HitEntity = HitEntityMin;
                WallNormal = WallNormalMin;
            }
            else
            {
                tStop = tMax;
                HitEntity = HitEntityMax;
                WallNormal = WallNormalMax;
            }
            
            Entity->P += tStop*PlayerDelta;
            DistanceRemaining -= tStop*PlayerDeltaLength;
            if(HitEntity)
            {
                PlayerDelta = DesiredPosition - Entity->P;
                
                bool32 StopsOnCollision = HandleCollision(Entity, HitEntity);
                if(StopsOnCollision)
                {
                    PlayerDelta = PlayerDelta - 1*Inner(PlayerDelta, WallNormal)*WallNormal;
                    Entity->dP = Entity->dP - 1*Inner(Entity->dP, WallNormal)*WallNormal;
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
    
    if(Entity->DistanceLimit != 0.0f)
    {
        Entity->DistanceLimit = DistanceRemaining;
    }
    
#if 0
    // TODO(casey): Change to using the acceleration vector
    if((Entity->dP.x == 0.0f) && (Entity->dP.y == 0.0f))
    {
        // NOTE(casey): Leave FacingDirection whatever it was
    }
    else
    {
        Entity->FacingDirection = ATan2(Entity->dP.y, Entity->dP.x);
    }
#endif
}

internal b32x
OverlappingEntitiesExist(sim_region *SimRegion, rectangle3 Bounds)
{
    b32x Result = false;
    
    for(entity_iterator Iter = IterateAllEntities(SimRegion);
        Iter.Entity;
        Advance(&Iter))
    {
        entity *TestEntity = Iter.Entity;
        if(EntityOverlapsRectangle(TestEntity->P, TestEntity->CollisionVolume, Bounds))
        {
            Result = true;
            break;
        }
    }
    
    return(Result);
}

enum traversable_search_flag
{
    TraversableSearch_Unoccupied = 0x1,
    TraversableSearch_ClippedZ = 0x2,
};
// TODO(casey): Why doesn't this just return a traversable_reference
internal b32
GetClosestTraversable(sim_region *SimRegion, v3 FromP, traversable_reference *Result,
                      u32 Flags = 0)
{
    TIMED_FUNCTION();
    
    b32 Found = false;
    
    // TODO(casey): Make spatial queries easy for things!
    r32 ClosestDistanceSq = Square(1000.0f);
    
    for(entity_iterator Iter = IterateAllEntities(SimRegion);
        Iter.Entity;
        Advance(&Iter))
    {
        entity *TestEntity = Iter.Entity;
        for(u32 PIndex = 0;
            PIndex < TestEntity->TraversableCount;
            ++PIndex)
        {
            entity_traversable_point P =
                GetSimSpaceTraversable(TestEntity, PIndex);
            if(!(Flags & TraversableSearch_Unoccupied) || (P.Occupier == 0))
            {
                v3 ToPoint = P.P - FromP;
                if(!(Flags & TraversableSearch_ClippedZ) ||
                   ((ToPoint.z >= -2.0f) &&
                    (ToPoint.z <= 2.0f)))
                {
                    if(Flags & TraversableSearch_ClippedZ)
                    {
                        ToPoint.z = 0.0f;
                    }
                    
                    real32 TestDSq = LengthSq(ToPoint);
                    if(ClosestDistanceSq > TestDSq)
                    {
                        // P.P;
                        Result->Entity = TestEntity->ID;
                        Result->Index = PIndex;
                        ClosestDistanceSq = TestDSq;
                        Found = true;
                    }
                }
            }
        }
    }
    
    if(!Found)
    {
        Result->Entity.Value = 0;
        Result->Index = 0;
    }
    
    return(Found);
}

internal b32
GetClosestTraversableAlongRay(sim_region *SimRegion, v3 FromP, v3 Dir,
                              traversable_reference Skip, traversable_reference *Result,
                              u32 Flags = 0)
{
    TIMED_FUNCTION();
    
    b32 Found = false;
    
    for(u32 ProbeIndex = 0;
        ProbeIndex < 5;
        ++ProbeIndex)
    {
        v3 SampleP = FromP + 0.5f*(r32)ProbeIndex*Dir;
        if(GetClosestTraversable(SimRegion, SampleP, Result, Flags))
        {
            if(!IsEqual(Skip, *Result))
            {
                Found = true;
                break;
            }
        }
    }
    
    return(Found);
}

struct closest_entity
{
    entity *Entity;
    v3 Delta;
    r32 DistanceSq;
};
internal closest_entity
GetClosestEntityWithBrain(sim_region *SimRegion, v3 P, brain_type Type, r32 MaxRadius = 20.0f)
{
    closest_entity Result = {};
    Result.DistanceSq = Square(MaxRadius);
    
    for(entity_iterator Iter = IterateAllEntities(SimRegion);
        Iter.Entity;
        Advance(&Iter))
    {
        entity *TestEntity = Iter.Entity;
        if(IsType(TestEntity->BrainSlot, Type))
        {
            v3 TestDelta = TestEntity->P - P;
            real32 TestDSq = LengthSq(TestDelta);
            if(Result.DistanceSq > TestDSq)
            {
                Result.Entity = TestEntity;
                Result.DistanceSq = TestDSq;
                Result.Delta = TestDelta;
            }
        }
    }
    
    return(Result);
}

internal void
FindNextEntity(entity_iterator *Iter)
{
    Iter->Entity = 0;
    
    while(Iter->HashIndex < ArrayCount(Iter->SimRegion->EntityHash))
    {
        if(!IsEmpty(Iter->SimRegion->EntityHashOccupancy, Iter->HashIndex))
        {
            Iter->Entity = Iter->SimRegion->EntityHash[Iter->HashIndex].Ptr;
            Assert(Iter->Entity);
            break;
        }
        ++Iter->HashIndex;
    }
}

internal entity_iterator 
IterateAllEntities(sim_region *SimRegion)
{
    entity_iterator Iter;
    Iter.SimRegion = SimRegion;
    Iter.HashIndex = 0;
    FindNextEntity(&Iter);
    
    return(Iter);
}

internal void
Advance(entity_iterator *Iter)
{
    Assert(Iter->Entity);
    ++Iter->HashIndex;
    FindNextEntity(Iter);
}
