/* ========================================================================
   $File: C:\work\handmade\code\handmade_edit_grid.cpp $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright by Molly Rocket, Inc., All Rights Reserved. $
   ======================================================================== */

inline world_position
ChunkPositionFromTilePosition(world_generator *Gen, s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ,
                              v3 AdditionalOffset = V3(0, 0, 0))
{
    world_position BasePos = {};
    
    v3 TileDim = Gen->TileDim;
    v3 Offset = Hadamard(TileDim, V3((real32)AbsTileX, (real32)AbsTileY, (real32)AbsTileZ));
    world_position Result = MapIntoChunkSpace(Gen->World, BasePos, AdditionalOffset + Offset);
    
    Assert(IsCanonical(Gen->World, Result.Offset_));
    
    return(Result);
}

inline world_position
ChunkPositionFromTilePosition(world_generator *Gen, gen_v3 AbsTile,
                              v3 AdditionalOffset = V3(0, 0, 0))
{
    world_position Result = ChunkPositionFromTilePosition(Gen, AbsTile.x, AbsTile.y, AbsTile.z, AdditionalOffset);;
    return(Result);
}

internal edit_tile_contents *
GetTile(edit_grid *Grid, s32 XIndex, s32 YIndex, s32 ZIndex)
{
    edit_tile_contents *Result = 0;
    gen_v3 Dim = Grid->TileCount;
    
    if((XIndex >= 0) &&
       (YIndex >= 0) &&
       (ZIndex >= 0) &&
       (XIndex < Dim.x) &&
       (YIndex < Dim.y) &&
       (ZIndex < Dim.z))
    {
        Result = Grid->Tiles + (Dim.x*Dim.y)*ZIndex + (Dim.x)*YIndex + XIndex;
    }
    
    return(Result);
}

internal edit_tile_contents *
GetTile(edit_grid *Grid, gen_v3 P)
{
    edit_tile_contents *Result = GetTile(Grid, P.x, P.y, P.z);
    return(Result);
}

internal edit_tile_contents *
GetTile(edit_tile *Tile)
{
    edit_tile_contents *Result = GetTile(Tile->Grid, Tile->RelIndex);
    return(Result);
}

internal b32x
RecursiveOpenTileSearch(edit_grid *Grid, gen_v3 TileP, gen_entity *Entity)
{
    Assert(Entity);
    
    b32x Result = false;
    
    edit_tile_contents *Tile = GetTile(Grid, TileP);
    if(Tile && Tile->Open)
    {
        Assert(Tile->Structural);
        Tile->Open = false;
        
        if(Entity->Next)
        {
            for(u32 Dir = 0;
                Dir < BoxIndex_Count;
                ++Dir)
            {
                box_surface_index DirIndex = (box_surface_index)Dir;
                u32 Mask = GetSurfaceMask(DirIndex);
                if(Entity->AllowedDirectionsForNext & Mask)
                {
                    gen_v3 dTileP = GetDirection(DirIndex);
                    if(RecursiveOpenTileSearch(Grid, TileP + dTileP, Entity->Next))
                    {
                        Entity->NextDirectionUsed = DirIndex;
                        Result = true;
                        break;
                    }
                }
            }
        }
        else
        {
            Result = true;
        }
        
        if(!Result)
        {
            Tile->Open = true;
        }
    }
    
    return(Result);
}

internal gen_room_tile_query
FindPlaceToPutEntityGroup(edit_grid *Grid, gen_entity_group *EntityGroup)
{
    gen_room_tile_query Result = {};
    
    for(s32 Z = 0;
        Z < Grid->TileCount.z;
        ++Z)
    {
        for(s32 Y = 0;
            Y < Grid->TileCount.y;
            ++Y)
        {
            for(s32 X = 0;
                X < Grid->TileCount.x;
                ++X)
            {
                gen_v3 TileP = GenV3(X, Y, Z);
                if(RecursiveOpenTileSearch(Grid, TileP, EntityGroup->FirstEntity))
                {
                    Result.Found = true;
                    Result.Volume.Min = Result.Volume.Max = TileP;
                    break;
                }
            }
        }
    }
    
    return(Result);
}

internal rectangle3 
GetRoomVolume(edit_grid *Grid)
{
    rectangle3 Result = Grid->RoomDim;
    return(Result);
}

internal world_position
GetRoomMinWorldP(edit_grid *Grid)
{
    world *World = Grid->Gen->World;
    world_position Result = MapIntoChunkSpace(World, Grid->BaseP, Grid->RoomDim.Min);
    return(Result);
}

internal world_position
GetRoomMaxWorldP(edit_grid *Grid)
{
    world *World = Grid->Gen->World;
    world_position Result = MapIntoChunkSpace(World, Grid->BaseP, Grid->RoomDim.Max);
    return(Result);
}

internal edit_grid *
BeginGridEdit(world_generator *Gen, gen_volume Volume)
{
    memory_arena *Memory = &Gen->TempMemory;
    temporary_memory TempMem = BeginTemporaryMemory(Memory);
    
    edit_grid *Grid = PushStruct(Memory, edit_grid);
    
    Grid->Gen = Gen;
    Grid->TempMem = TempMem;
    Grid->TileCount = GetDim(Volume);
    Grid->MinTile = Volume.Min;
    Grid->TileDim = Gen->TileDim;
    Grid->Series = &Gen->World->GameEntropy;
    
    v3 RoomDim =
    {
        Grid->TileCount.x * Grid->TileDim.x,
        Grid->TileCount.y * Grid->TileDim.y,
        Grid->TileCount.z * Grid->TileDim.z,
    };
    
    v3 MinRoomP = {};
    v3 MaxRoomP = MinRoomP + RoomDim;
    Grid->RoomDim = RectMinMax(MinRoomP, MaxRoomP);
    
    Grid->BaseP = ChunkPositionFromTilePosition(Gen, Grid->MinTile);
    
    rectangle3 ChangeRect = AddRadiusTo(Grid->RoomDim, 1.0f*Grid->TileDim);
    Grid->Region = BeginWorldChange(Gen->Assets, Memory, Gen->World, Grid->BaseP, ChangeRect, 0);
    
    Grid->Tiles = PushArray(Memory, GetTotalVolume(Grid->TileCount), edit_tile_contents);
    
    return(Grid);
}

internal void
EndGridEdit(edit_grid *Grid)
{
    EndWorldChange(Grid->Region);
    EndTemporaryMemory(Grid->TempMem);
}

internal gen_v3
GetAbsoluteTileIndex(edit_tile *Tile)
{
    edit_grid *Grid = Tile->Grid;
    gen_v3 Result = Grid->MinTile + Tile->RelIndex;
    return(Result);
}

internal b32x
IsOnEdge(edit_tile *Tile)
{
    gen_v3 P = Tile->RelIndex;
    edit_grid *Grid = Tile->Grid;
    b32x Result = ((P.x == 0) ||
                   (P.x == (Grid->TileCount.x - 1)) ||
                   (P.y == 0) ||
                   (P.y == (Grid->TileCount.y - 1)));
    return(Result);
}

internal rectangle3
GetTotalVolume(edit_tile *Tile)
{
    edit_grid *Grid = Tile->Grid;
    v3 MinP = {(f32)Tile->RelIndex.x, (f32)Tile->RelIndex.y, (f32)Tile->RelIndex.z};
    v3 MaxP = MinP + V3(1, 1, 1);
    
    MinP = Hadamard(Grid->TileDim, MinP);
    MaxP = Hadamard(Grid->TileDim, MaxP);
    
    rectangle3 Result = RectMinMax(MinP, MaxP);
    return(Result);
}

internal v3
GetMinZCenterP(edit_tile *Tile)
{
    v3 Result = GetMinZCenterP(GetTotalVolume(Tile));
    return(Result);
}

internal rectangle3
GetVolumeFromMinZ(edit_tile *Tile, f32 Height)
{
    rectangle3 Result = GetTotalVolume(Tile);
    Result.Max.z = (Result.Min.z + Height);
    return(Result);
}

internal edit_tile
IterateAsPlanarTiles(edit_grid *Grid)
{
    edit_tile Tile = {};
    Tile.Grid = Grid;
    return(Tile);
}

internal b32x
IsValid(edit_tile *Tile)
{
    edit_grid *Grid = Tile->Grid;
    b32x Result = IsInArrayBounds(Grid->TileCount, Tile->RelIndex);
    return(Result);
}

internal void
Advance(edit_tile *Tile)
{
    edit_grid *Grid = Tile->Grid;
    ++Tile->RelIndex.x;
    if(Tile->RelIndex.x >= Grid->TileCount.x)
    {
        Tile->RelIndex.x = 0;
        ++Tile->RelIndex.y;
    }
}
