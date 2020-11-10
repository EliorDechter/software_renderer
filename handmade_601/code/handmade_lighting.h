/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#include "handmade_sampling_spheres.inl"

struct lighting_point
{
    v3 P;
    v3 RefC;
    v3 N;
    v3 XAxis;
    v3 YAxis;
    u32 PackIndex;
};

struct debug_line
{
    v3 FromP;
    v3 ToP;
    v4 Color;
};

struct lighting_work
{
    struct lighting_solution *Solution;
    light_atlas *DiffuseLightAtlas;
    light_atlas *SpecularLightAtlas;
    void *Reserved;
    
    u32 VoxelX;
    u32 SamplePointEntropy;
    u32 LightPointEntropy;
    
    u32 TotalCastsInitiated; // NOTE(casey): Number of attempts to raycast from a point
    u32 TotalPartitionsTested; // NOTE(casey): Number of partition boxes checked
    u32 TotalPartitionLeavesUsed; // NOTE(casey): Number of partition boxes used as leaves
    u32 TotalLeavesTested; // NOTE(casey): Number of leaf boxes checked
    
    u8 SmallPad[4];
};
CTAssert(sizeof(lighting_work) == 64);

struct light_node
{
    v3 P;
    v3 Radius;
    v3 Emission;
    u32 Child[2];
};

struct diffuse_weight_map
{
    f32_4x E[16][16];
};

struct kd_tree_node // 4*3 = 12 bytes
{
    u32 Sides[2];
    f32 PlaneD;
};

#define SPATIAL_GRID_NODE_TERMINATOR 0xffff
struct lighting_spatial_grid_node
{
    u16 StartIndex;
    u16 OnePastLastIndex;
};

struct lighting_spatial_grid_leaf
{
    v3_4x BoxMin;
    v3_4x BoxMax;
    v3_4x RefColor;
    f32_4x IsEmission;
};

struct lighting_solution
{
    u16 BoxCount;
    lighting_box *Boxes;
    
    u16 ScratchA[MAX_LIGHT_BOX_COUNT];
    u16 ScratchB[MAX_LIGHT_BOX_COUNT];
    u16 ScratchC[MAX_LIGHT_BOX_COUNT];
    
    // TODO(casey): Remove these once we recapture!
    // {
    union
    {
        struct
        {
            u32 RootKdValue;
            u16 KdNodeCount;
            kd_tree_node *KdNodes;
        };
        
        struct
        {
            u16 BoxRefCount_;
            u16 BoxTable_[MAX_LIGHT_BOX_COUNT];
        };
        
        struct
        {
            lighting_spatial_grid_node *SpatialGridNodes;
            lighting_spatial_grid_leaf *SpatialGridLeaves;
        };
    };
    // }
    u16 RootBoxIndex;
    u16 RootLightBoxIndex;
    
    random_series Series;
    
    u32x DebugBoxDrawDepth;
    
    u32 MaxWorkCount;
    lighting_work *Works;
    
    b32x UpdateDebugLines;
    u32x DebugProbeIndex;
    u32x DebugLineCount;
    debug_line DebugLines[65536];
    
    f32 tUpdateBlend;
    
    b32 LightingEnabled;
    b32 Accumulating;
    u32 AccumulationCount;
    
    u32 FrameOdd;
    
    diffuse_weight_map DiffuseWeightMap[16][16];
    light_sampling_sphere *SamplingSpheres;
    
    v3s VoxCameraOffset;
    v3 VoxMinCornerHot;
    v3 VoxCellDim;
    v3 InvVoxCellDim;
    
    v3 DebugLightP;
    
    world_position LastOriginP;
};
