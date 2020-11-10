/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

inline rectangle3
MakeSimpleGroundedCollision(f32 DimX, f32 DimY, f32 DimZ, f32 OffsetZ)
{
    rectangle3 Result = RectCenterDim(V3(0, 0, 0.5f*DimZ + OffsetZ),
                                      V3(DimX, DimY, DimZ));
    return(Result);
}

internal entity_visible_piece *
AddPiece(entity *Entity, asset_basic_category AssetCategory, v3 Dim, v3 Offset, v4 Color, u32 Flags)
{
    Assert(Entity->PieceCount < ArrayCount(Entity->Pieces));
    entity_visible_piece *Piece = Entity->Pieces + Entity->PieceCount++;
    Piece->AssetCategory = AssetCategory;
    Piece->Dim = Dim;
    Piece->Offset = Offset;
    Piece->Color = Color;
    Piece->Flags = Flags;
    
    return(Piece);
}

internal entity_visible_piece *
AddPiece(entity *Entity, asset_basic_category AssetCategory, f32 Height, v3 Offset, v4 Color, u32 Flags)
{
    entity_visible_piece *Result =
        AddPiece(Entity, AssetCategory, V3(0, Height, 0), Offset, Color, Flags);
    return(Result);
}

internal void
ConnectPiece(entity *Entity,
             entity_visible_piece *Parent, hha_align_point_type ParentType,
             entity_visible_piece *Child, hha_align_point_type ChildType)
{
    Assert(IsBitmap(Child));
    bitmap_piece *Bitmap = &Child->Bitmap;
    Bitmap->ParentPiece = SafeTruncateToU8(Parent - Entity->Pieces);
    Bitmap->ParentAlignType = SafeTruncateToU8(ParentType);
    Bitmap->ChildAlignType = SafeTruncateToU8(ChildType);
    
    Assert(Bitmap->ParentPiece < (Child - Entity->Pieces));
}

internal void
ConnectPieceToWorld(entity *Entity, entity_visible_piece *Child, hha_align_point_type ChildType)
{
    Assert(IsBitmap(Child));
    bitmap_piece *Bitmap = &Child->Bitmap;
    Bitmap->ParentPiece = 0;
    Bitmap->ParentAlignType = SafeTruncateToU8(HHAAlign_None);
    Bitmap->ChildAlignType = SafeTruncateToU8(ChildType);
}

internal entity_visible_piece *
AddPieceLight(entity *Entity, f32 Radius, v3 Offset, f32 Emission, v3 Color)
{
    entity_visible_piece *Result =
        AddPiece(Entity, Asset_None, V3(Radius, Radius, Radius), Offset, V4(Color, Emission), PieceType_Light);
    return(Result);
}

internal entity *
AddEntity(sim_region *Region)
{
    entity *Entity = CreateEntity(Region, AllocateEntityID(Region->World));
    
    Entity->XAxis = V2(1, 0);
    Entity->YAxis = V2(0, 1);
    
    return(Entity);
}

internal void
AddTag(entity *Entity, asset_tag_id ID, f32 Value)
{
    Assert(Entity->TagCount < ArrayCount(Entity->Tags));
    u32 TagIndex = Entity->TagCount++;
    
    Entity->Tags[TagIndex] = ID;
    Entity->TagValue[TagIndex] = Value;
}

internal void
InitHitPoints(entity *Entity, uint32 HitPointCount)
{
    Assert(HitPointCount <= ArrayCount(Entity->HitPoint));
    Entity->HitPointMax = HitPointCount;
    for(uint32 HitPointIndex = 0;
        HitPointIndex < Entity->HitPointMax;
        ++HitPointIndex)
    {
        hit_point *HitPoint = Entity->HitPoint + HitPointIndex;
        HitPoint->Flags = 0;
        HitPoint->FilledAmount = HIT_POINT_SUB_COUNT;
    }
}

internal
GEN_CREATE_ENTITY(AddInanimate)
{
    entity *Entity = AddEntity(Region);
    entity_visible_piece *Body = AddPiece(Entity, Asset_Scenery, 1.0f, V3(0, 0, 0), V4(1, 1, 1, 1));
    ConnectPieceToWorld(Entity, Body, HHAAlign_Default);
    return(Entity);
}

internal
GEN_CREATE_ENTITY(AddObstacle)
{
    entity *Entity = AddInanimate(Region);
    AddFlags(Entity, EntityFlag_Collides);
    return(Entity);
}

internal
GEN_CREATE_ENTITY(AddCat)
{
    entity *Entity = AddEntity(Region);
    AddFlags(Entity, EntityFlag_Collides);
    
    // Entity->BrainSlot = BrainSlotFor(brain_cat, Body);
    // Entity->BrainID = AddBrain(Region);
    
    entity_visible_piece *Body = AddPiece(Entity, Asset_Body, 1.0f, V3(0, 0, 0), V4(1, 1, 1, 1));
    entity_visible_piece *Head = AddPiece(Entity, Asset_Head, 1.0f, V3(0, 0, 0.1f), V4(1, 1, 1, 1));
    
    ConnectPieceToWorld(Entity, Body, HHAAlign_Default);
    ConnectPiece(Entity, Body, HHAAlign_BaseOfNeck, Head, HHAAlign_Default);
    
    return(Entity);
}

internal
GEN_CREATE_ENTITY(AddOrphan)
{
    entity *Entity = AddEntity(Region);
    AddFlags(Entity, EntityFlag_Collides);
    
    // Entity->BrainSlot = BrainSlotFor(brain_orphan, Body);
    // Entity->BrainID = AddBrain(Region);
    
    entity_visible_piece *Body = AddPiece(Entity, Asset_Body, 1.0f, V3(0, 0, 0), V4(1, 1, 1, 1));
    entity_visible_piece *Head = AddPiece(Entity, Asset_Head, 1.0f, V3(0, 0, 0.1f), V4(1, 1, 1, 1));
    
    ConnectPieceToWorld(Entity, Body, HHAAlign_Default);
    ConnectPiece(Entity, Body, HHAAlign_BaseOfNeck, Head, HHAAlign_Default);
    
    return(Entity);
}

internal
GEN_CREATE_ENTITY(AddConversation)
{
    entity *Entity = AddEntity(Region);
    v3 Dim = {0.5f, 0.5f, 0.5f};
    Entity->CollisionVolume = AddRadiusTo(RectCenterDim(V3(0, 0, 0.5f*Dim.z), Dim),
                                          V3(0.1f, 0.1f, 0.1f));
    Entity->CameraBehavior = Camera_Offset;
    Entity->CameraOffset.z = GetCameraOffsetZForCloseup();
    Entity->CameraOffset.y = 2.0f;
    
    return(Entity);
}

internal
GEN_CREATE_ENTITY(AddSnake)
{
    // TODO(casey): Figure out where this info will live
    // {
    u32 SegmentIndex = 0;
    brain_id BrainID = AddBrain(Region);
    // }
    
    entity *Entity = AddEntity(Region);
    AddFlags(Entity, EntityFlag_Collides);
    
    Entity->BrainSlot = IndexedBrainSlotFor(brain_snake, Segments, SegmentIndex);
    Entity->BrainID = BrainID;
    Entity->CollisionVolume = MakeSimpleGroundedCollision(0.75f, 0.75f, 0.75f, 0.0f);
    
    entity_visible_piece *Body = AddPiece(Entity, SegmentIndex ? Asset_Body : Asset_Head, 1.5f, V3(0, 0, 0.5f), V4(1, 1, 1, 1));
    AddPieceLight(Entity, 0.5f, V3(0, 0, 1.0f), 0.5f, V3(1, 1, 0));
    
    ConnectPieceToWorld(Entity, Body, HHAAlign_Default);
    
    InitHitPoints(Entity, 3);
    
    return(Entity);
}

internal entity *
GenEntityAtP(sim_region *Region, gen_create_entity *Creator, v3 P)
{
    entity *Result = Creator(Region);
    Result->P = P;
    return(Result);
}

internal entity *
GenEntityAtTraversable(sim_region *Region, gen_create_entity *Creator, traversable_reference StandingOn)
{
    entity *Result = Creator(Region);
    Result->P = GetSimSpaceTraversable(Region, StandingOn).P;
    Result->Occupying = StandingOn;
    
    return(Result);
}

internal void
AddMonstar(sim_region *Region, v3 P, traversable_reference StandingOn)
{
    entity *Entity = AddEntity(Region);
    AddFlags(Entity, EntityFlag_Collides);
    
    Entity->BrainSlot = BrainSlotFor(brain_monstar, Body);
    Entity->BrainID = AddBrain(Region);
    Entity->Occupying = StandingOn;
    
    InitHitPoints(Entity, 3);
    
    AddPiece(Entity, Asset_Shadow, 4.5f, V3(0, 0, 0), V4(1, 1, 1, ShadowAlpha));
    AddPiece(Entity, Asset_Body, 4.5f, V3(0, 0, 0), V4(1, 1, 1, 1));
    
    Entity->P = P;
}

internal void
AddFamiliar(sim_region *Region, v3 P, traversable_reference StandingOn)
{
    entity *Entity = AddEntity(Region);
    
    Entity->BrainSlot = BrainSlotFor(brain_familiar, Head);
    Entity->BrainID = AddBrain(Region);
    Entity->Occupying = StandingOn;
    
    AddPiece(Entity, Asset_Shadow, 2.5f, V3(0, 0, 0), V4(1, 1, 1, ShadowAlpha));
    AddPiece(Entity, Asset_Head, 2.5f, V3(0, 0, 0), V4(1, 1, 1, 1), PieceMove_BobOffset);
    
    Entity->P = P;
}
