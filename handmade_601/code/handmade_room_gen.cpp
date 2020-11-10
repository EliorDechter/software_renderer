/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

internal f32
GetCameraOffsetZForCloseup(void)
{
    f32 Result = 6.0f;
    return(Result);
}

internal f32
GetCameraOffsetZForDim(gen_v3 Dim, u32 *CameraBehavior)
{
    s32 XCount = Dim.x;
    s32 YCount = Dim.y;
    
    f32 XDist = 13.0f;
    if(XCount == 12)
    {
        XDist = 14.0f;
    }
    else if(XCount == 13)
    {
        XDist = 15.0f;
    }
    else if(XCount == 14)
    {
        XDist = 16.0f;
        *CameraBehavior |= Camera_ViewPlayerX;
    }
    else if(XCount >= 15)
    {
        XDist = 17.0f;
        *CameraBehavior |= Camera_ViewPlayerX;
    }
    
    f32 YDist = 13.0f;
    if(YCount == 10)
    {
        YDist = 15.0f;
    }
    else if(YCount == 11)
    {
        YDist = 17.0f;
    }
    else if(YCount == 12)
    {
        YDist = 19.0f;
        *CameraBehavior |= Camera_ViewPlayerY;
    }
    else if(YCount >= 13)
    {
        YDist = 21.0f;
        *CameraBehavior |= Camera_ViewPlayerY;
    }
    
    f32 Result = Maximum(XDist, YDist);
    
    return(Result);
}

internal void
AddTreeTags(world_generator *Gen, entity *Tree)
{
    AddTag(Tree, Tag_Variant, RandomUnilateral(Gen->Entropy));
    AddTag(Tree, Tag_DarkEnergy, 0.0f);//RandomUnilateral(Gen->Entropy));
    AddTag(Tree, Tag_Winter, RandomUnilateral(Gen->Entropy));
    AddTag(Tree, Tag_Fall, 0.0f);//RandomUnilateral(Gen->Entropy));
    AddTag(Tree, Tag_Damaged, 0.0f);//RandomUnilateral(Gen->Entropy));
}

internal void
GenerateRoom(world_generator *Gen, gen_room *Room)
{
    gen_room_spec *Spec = Room->Spec;
    
    edit_grid *Grid = BeginGridEdit(Gen, Room->Vol);
    for(edit_tile Tile = IterateAsPlanarTiles(Grid);
        IsValid(&Tile);
        Advance(&Tile))
    {
        gen_v3 AbsIndex = GetAbsoluteTileIndex(&Tile);
        v3 P = GetMinZCenterP(&Tile);
        
        edit_tile_contents *Contents = GetTile(&Tile);
        Assert(Contents);
        
        b32x OnEdge = IsOnEdge(&Tile);
        b32 OnBoundary = OnEdge;
        f32 tStair = 0.0f;
        b32 OnConnection = false;
        b32 Stairwell = false;
        if(Spec->Outdoors)
        {
            OnBoundary = false;
        }
        
        for(gen_room_connection *RoomCon = Room->FirstConnection;
            RoomCon;
            RoomCon = RoomCon->Next)
        {
            gen_connection *Con = RoomCon->Connection;
            if(IsInVolume(&Con->Vol, AbsIndex))
            {
                if((RoomCon->PlacedDirection == BoxIndex_Up) ||
                   (RoomCon->PlacedDirection == BoxIndex_Down))
                {
                    Stairwell = true;
                    tStair = (f32)(AbsIndex.y - Con->Vol.Min.y + 1) /
                        (f32)(Con->Vol.Max.y - Con->Vol.Min.y + 2);
                }
                OnConnection = true;
            }
        }
        
        entity *Entity = AddEntity(Grid->Region);
        
        v4 Color = sRGBLinearize(0.31f, 0.49f, 0.32f, 1.0f);
        if(OnConnection)
        {
            Color = sRGBLinearize(0.21f, 0.29f, 0.42f, 1.0f);
        }
        
        f32 WallHeight = 0.5f;
        
        b32x PlaceTree = (Spec->Outdoors && !OnConnection && OnEdge);
        b32 OnLamp = (!Spec->Outdoors && ((Tile.RelIndex.x == (Grid->TileCount.x - 2)) &&
                                          (Tile.RelIndex.y == 1)));
        b32x RandomizeTop = false;
        b32x Traversable = false;
        b32x OnWall = (OnBoundary && !OnConnection);
        if(OnWall)
        {
            WallHeight = 2.0f;
            Color = sRGBLinearize(0.5f, 0.2f, 0.2f, 1.0f);
            AddTag(Entity, Tag_Wall, 1.0f);
            AddTag(Entity, Tag_Wood, 1.0f);
        }
        else
        {
            AddTag(Entity, Tag_Floor, 1.0f);
            if(Spec->Outdoors)
            {
                AddTag(Entity, Tag_Grass, 1.0f);
                Entity->GroundCoverSpecs[0].Type = CoverType_ThickGrass;
                Entity->GroundCoverSpecs[0].Density = 64;
            }
            else
            {
                AddTag(Entity, Spec->StoneFloor ? Tag_Stone : Tag_Wood, 1.0f);
            }
            RandomizeTop = true;
            Traversable = true;
        }
        
        rectangle3 Vol = MakeRelative(GetVolumeFromMinZ(&Tile, WallHeight), P);
        
        if(Traversable)
        {
            Entity->TraversableCount = 1;
            Entity->Traversables[0].P = GetMaxZCenterP(Vol);
            Entity->Traversables[0].Occupier = 0;
        }
        
        if(!Spec->Outdoors)
        {
            AddTag(Entity, Tag_Manmade, 1.0f);
        }
        
        if(Stairwell)
        {
            P.z -= tStair*Grid->TileDim.z;
        }
        v3 BasisP = P;
        
        P.x += 0.0f;
        P.y += 0.0f;
        //P.z += 0.5f*RandomUnilateral(Grid->Series);
        
        Color = sRGBLinearize(0.8f, 0.8f, 0.8f, 1.0f);
        entity_visible_piece *Piece =
            AddPiece(Entity, Asset_Block, GetRadius(Vol), GetCenter(Vol), Color, PieceType_Cube);
        
        if(RandomizeTop)
        {
            Piece->CubeUVLayout = EncodeCubeUVLayout(0, 0, 0, 0, 0, 0,
                                                     RandomChoice(Gen->Entropy, 4),
                                                     RandomChoice(Gen->Entropy, 4));
        }
        
        Entity->P = P;
        Entity->CollisionVolume = Vol;
        Contents->Structural = Entity;
        Contents->Open = (!Stairwell && !OnConnection &&
                          (Entity->TraversableCount == 1));
        
        if(Contents->Open)
        {
            traversable_reference Ref = {};
            Ref.Entity = Contents->Structural->ID;
            v3 GroundP = GetSimSpaceTraversable(Grid->Region, Ref).P;
            if(PlaceTree)
            {
                entity *PlacedEntity = GenEntityAtTraversable(Grid->Region, AddObstacle, Ref);
                AddTreeTags(Gen, PlacedEntity);
            }
            else if(OnLamp)
            {
                entity *PlacedEntity = GenEntityAtTraversable(Grid->Region, AddObstacle, Ref);
                AddTag(PlacedEntity, Tag_Lamp, 1.0f);
                v3 LampLight = V3(RandomBetween(Gen->Entropy, 0.4f, 0.7f),
                                  RandomBetween(Gen->Entropy, 0.4f, 0.7f),
                                  0.5f);
                AddPieceLight(Entity, 0.5f, V3(0, 0, 2.5f), 1.0f, LampLight);
            }
        }
    }
    
    for(gen_entity_group *EntityGroup = Room->FirstEntityGroup;
        EntityGroup;
        EntityGroup = EntityGroup->Next)
    {
        gen_room_tile_query Query = FindPlaceToPutEntityGroup(Grid, EntityGroup);
        Assert(Query.Found);
        
        gen_v3 TileP = Query.Volume.Min;
        
        for(gen_entity *PendingEntity = EntityGroup->FirstEntity;
            PendingEntity;
            PendingEntity = PendingEntity->Next)
        {
            edit_tile_contents *Contents = GetTile(Grid, TileP);
            Assert(Contents);
            
            traversable_reference Ref = {};
            if(Contents->Structural)
            {
                Ref.Entity = Contents->Structural->ID;
            }
            else
            {
                InvalidCodePath;
            }
            
            entity *PlacedEntity = GenEntityAtTraversable(Grid->Region, PendingEntity->Creator, Ref);
            for(u32 TagIndex = 0;
                TagIndex < PendingEntity->TagCount;
                ++TagIndex)
            {
                gen_entity_tag *Tag = PendingEntity->Tags + TagIndex;
                AddTag(PlacedEntity, Tag->ID, Tag->Value);
            }
            
            TileP = TileP + GetDirection(PendingEntity->NextDirectionUsed);
        }
    }
    
    entity *CamRoom = AddEntity(Grid->Region);
    CamRoom->P = GetCenter(GetRoomVolume(Grid));
    CamRoom->CollisionVolume = MakeRelative(GetRoomVolume(Grid), CamRoom->P);
    CamRoom->BrainSlot = SpecialBrainSlot(Type_brain_room);
    CamRoom->CameraOffset.z = GetCameraOffsetZForDim(Grid->TileCount, &CamRoom->CameraBehavior);
    
    world_room *WorldRoom = AddWorldRoom(Gen->World, GetRoomMinWorldP(Grid), GetRoomMaxWorldP(Grid));
    
    EndGridEdit(Grid);
    
    if(Spec->Apron)
    {
        gen_apron *Apron = GenApron(Gen, Spec->Apron);
        Apron->Vol = AddRadiusTo(&Room->Vol, GenV3(8, 8, 0));
    }
}

internal void
GenerateApron(world_generator *Gen,
              gen_apron *Apron)
{
    gen_apron_spec *Spec = Apron->Spec;
    
    edit_grid *Grid = BeginGridEdit(Gen, Apron->Vol);
    for(edit_tile Tile = IterateAsPlanarTiles(Grid);
        IsValid(&Tile);
        Advance(&Tile))
    {
        f32 Eps = 0.001f;
        if(!OverlappingEntitiesExist(Grid->Region, AddRadiusTo(GetTotalVolume(&Tile), V3(-Eps, -Eps, -Eps))))
        {
            v3 P = GetMinZCenterP(&Tile);
            f32 Height = RandomBetween(Grid->Series, 0.5f, 1.0f);
            rectangle3 Vol = MakeRelative(GetVolumeFromMinZ(&Tile, Height), P);
            
            entity *Entity = AddEntity(Grid->Region);
            Entity->P = P;
            Entity->CollisionVolume = Vol;
            
            v4 Color = sRGBLinearize(0.5f, 0.5f, 0.5f, 1.0f);
            
            entity_visible_piece *Piece = AddPiece(Entity, Asset_Block, GetRadius(Vol), GetCenter(Vol), Color, PieceType_Cube);
            AddTag(Entity, Tag_Floor, 1.0f);
            AddTag(Entity, Tag_Grass, 1.0f);
            
            Entity->GroundCoverSpecs[0].Type = CoverType_ThickGrass;
            Entity->GroundCoverSpecs[0].Density = 64;
            
            v3 GroundP = P + GetMaxZCenterP(Vol);
            if(RandomChoice(Grid->Series, 3))
            {
                entity *Tree = GenEntityAtP(Grid->Region, AddInanimate, GroundP);
                AddTreeTags(Gen, Tree);
            }
        }
    }
    EndGridEdit(Grid);
}
