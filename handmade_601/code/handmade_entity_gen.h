/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#define GEN_CREATE_ENTITY(name) entity *name(sim_region *Region)
typedef GEN_CREATE_ENTITY(gen_create_entity);
struct gen_entity_tag
{
    asset_tag_id ID;
    f32 Value;
};
enum gen_entity_flag
{
    GenEntity_IsNotOccupier = 0x1,
};
struct gen_entity
{
    gen_entity *Next;
    gen_create_entity *Creator;
    
    gen_entity_tag Tags[14];
    u32 TagCount;
    
    enum32(gen_entity_flag) GenFlags;
    flag32(box_surface_mask) AllowedDirectionsForNext;
    box_surface_index NextDirectionUsed;
};

struct gen_entity_group
{
    gen_entity_group *Next;
    gen_entity *FirstEntity;
};

internal void ConnectPiece(entity *Entity,
                           entity_visible_piece *Parent, hha_align_point_type ParentType,
                           entity_visible_piece *Child, hha_align_point_type ChildType);
internal void ConnectPieceToWorld(entity *Entity, entity_visible_piece *Child, hha_align_point_type ChildType);
internal void AddTag(entity *Entity, asset_tag_id ID, f32 Value);
