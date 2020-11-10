/* ========================================================================
   $File: C:\work\handmade\code\handmade_edit_grid.h $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright by Molly Rocket, Inc., All Rights Reserved. $
   ======================================================================== */

struct world_generator;

struct gen_room_tile_query
{
    b32 Found;
    gen_volume Volume;
};

struct edit_tile_contents
{
    b32 Open;
    entity *Structural;
};

struct edit_grid
{
    temporary_memory TempMem;
    gen_v3 TileCount;
    gen_v3 MinTile;
    v3 TileDim;
    world_position BaseP;
    
    rectangle3 RoomDim;
    
    world_generator *Gen;
    random_series *Series;
    sim_region *Region;
    edit_tile_contents *Tiles;
};

struct edit_tile
{
    edit_grid *Grid;
    gen_v3 RelIndex;
};

