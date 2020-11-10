/* ========================================================================
   $File: C:\work\handmade\code\hhlightprof.cpp $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright by Molly Rocket, Inc., All Rights Reserved. $
   ======================================================================== */

#undef HANDMADE_INTERNAL
#define HANDMADE_INTERNAL 0

#undef HANDMADE_SLOW
#define HANDMADE_SLOW 0

#include "handmade_platform.h"
#include "handmade_intrinsics.h"
#include "handmade_math.h"
#include "handmade_shared.h"
#include "handmade_memory.h"
#include "handmade_stream.h"
#include "handmade_image.h"
#include "handmade_png.h"
#include "handmade_file_formats.h"
#include "handmade_simd.h"
#include "handmade_random.h"
#include "handmade_light_atlas.h"
#include "handmade_stream.cpp"
#include "handmade_image.cpp"
#include "handmade_png.cpp"
#include "handmade_file_formats.cpp"
#include "handmade_light_atlas.cpp"
#include "handmade_renderer.h"
#include "handmade_renderer.cpp"
#include "handmade_world.h"
#include "handmade_lighting.h"
#include "handmade_config.h"

#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

#define RECORD_RAYCAST_STACK 0

#if RECORD_RAYCAST_STACK
global FILE *LeafBoxOut = 0;
global FILE *PartitionBoxOut = 0;
global u32 StackDumpSwitch = 16;
#define RECORD_LEAF_BOX(BoxIndex) if(StackDumpSwitch) {fprintf(LeafBoxOut, "%u\n", BoxIndex);}
#define RECORD_PARTITION_BOX(BoxIndex) if(StackDumpSwitch) {fprintf(PartitionBoxOut, "%u\n", BoxIndex);}
#define RECORD_RAYCAST_END(...) if(StackDumpSwitch > 0) {--StackDumpSwitch; fprintf(PartitionBoxOut, "\n");}
#define RECORD_PARTITION_PUSH(ShouldPush, Pack) if(StackDumpSwitch && ShouldPush) {fprintf(PartitionBoxOut, "  + %u->%u\n", Pack.FirstChildIndex, Pack.FirstChildIndex + Pack.ChildCount);}
#endif

#define DevMode_lighting 0

platform_api Platform = {};

internal v3
Subtract(world *World, world_position *A, world_position *B)
{
    // NOTE(casey): Stubbed for lighting system linkage
    v3 Result = {};
    return(Result);
}

internal bool32
AreInSameChunk(world *World, world_position *A, world_position *B)
{
    // NOTE(casey): Stubbed for lighting system linkage
    bool32 Result = false;
    return(Result);
}

internal void
DEBUGDumpData(char *FileName, umm DumpSize, void *DumpData)
{
    // NOTE(casey): Stubbed for lighting system linkage
}

#include "handmade_lighting.cpp"

internal void
TestRayCast(lighting_solution *Solution, light_atlas *SpecAtlas, light_atlas *DiffuseAtlas)
{
    BuildSpatialPartitionForLighting(Solution);
    
    light_atlas_texel SpecTexel = GetTileClamped(SpecAtlas, 0, 0, 0);
    
    u32 WorkCount = SpecAtlas->VoxelDim.x;
    for(u32 WorkIndex = 0;
        WorkIndex < WorkCount;
        ++WorkIndex)
    {
        lighting_work *Work = Solution->Works + WorkIndex;
        Assert(((memory_index)Work & 63) == 0);
        ZeroStruct(*Work);
        
        Work->Solution = Solution;
        Work->DiffuseLightAtlas = DiffuseAtlas;
        Work->SpecularLightAtlas = SpecAtlas;
        Work->VoxelX = WorkIndex;
        Work->SamplePointEntropy = RandomNextU32(&Solution->Series);
        Work->LightPointEntropy = RandomNextU32(&Solution->Series);
        
        ComputeLightPropagationWork(0, Work);
    }
}

internal buffer
LoadEntireFile(char *Filename, umm ExtraSize = 0)
{
    buffer Result = {};
    
    FILE *File = fopen(Filename, "rb");
    if(File)
    {
        fseek(File, 0, SEEK_END);
        Result.Count = ftell(File);
        fseek(File, 0, SEEK_SET);
        
        Result.Data = (u8 *)malloc(Result.Count + ExtraSize);
        fread(Result.Data, Result.Count, 1, File);
        
        fclose(File);
    }
    else
    {
        fprintf(stderr, "Unable to open dump %s for reading.\n", Filename);
    }
    
    return(Result);
}

internal void
ProfileRun(u32 RepeatCount)
{
    fprintf(stdout, "KD Mode: %s\n", LIGHTING_USE_OLD_KD ? "old" : "new");
    
    buffer SourceLighting = LoadEntireFile("source_lighting.dump");
    buffer SourceLightBoxes = LoadEntireFile("source_lightboxes.dump", (1 << 20));
    buffer SourceSpecAtlas = LoadEntireFile("source_specatlas.dump");
    buffer SourceDiffuseAtlas = LoadEntireFile("source_diffuseatlas.dump");
    buffer ResultSpecAtlas = LoadEntireFile("result_specatlas.dump");
    buffer ResultDiffuseAtlas = LoadEntireFile("result_diffuseatlas.dump");
    
    buffer ResultLightBoxes = LoadEntireFile("result_lightboxes.dump");
    buffer ResultBoxRefs = LoadEntireFile("result_boxrefs.dump");
    
    if(SourceLighting.Count &&
       SourceLightBoxes.Count &&
       SourceSpecAtlas.Count &&
       SourceDiffuseAtlas.Count &&
       ResultSpecAtlas.Count &&
       ResultDiffuseAtlas.Count)
    {
        lighting_solution *Solution = (lighting_solution *)SourceLighting.Data;
        Solution->SamplingSpheres = LightSamplingSphereTable;
        
        u32 AllocWorkCount = MAX_LIGHT_LOOKUP_VOXEL_DIM;
        u32 MaxWorkStackDepth = Solution->BoxCount;
        Solution->Works = (lighting_work *)malloc(AllocWorkCount*sizeof(lighting_work));
        Solution->Boxes = (lighting_box *)SourceLightBoxes.Data;
        
        Solution->KdNodeCount = 0;
        Solution->KdNodes = (kd_tree_node *)malloc(U16Max*sizeof(kd_tree_node));
        
        v3s LightAtlasVoxelDim = V3S(LIGHT_LOOKUP_VOXEL_DIM_X, LIGHT_LOOKUP_VOXEL_DIM_Y, LIGHT_LOOKUP_VOXEL_DIM_Z);
        u32 LIGHT_COLOR_LOOKUP_SQUARE_DIM = (8+2);
        v2u LightAtlasTileDim = V2U(LIGHT_COLOR_LOOKUP_SQUARE_DIM, LIGHT_COLOR_LOOKUP_SQUARE_DIM);
        light_atlas DiffuseAtlas = MakeLightAtlas(LightAtlasVoxelDim, LightAtlasTileDim);
        light_atlas SpecAtlas = MakeLightAtlas(LightAtlasVoxelDim, LightAtlasTileDim);
        
        SetLightAtlasTexels(&DiffuseAtlas, SourceDiffuseAtlas.Data);
        SetLightAtlasTexels(&SpecAtlas, SourceSpecAtlas.Data);
        
        // TODO(casey): Remove this transform once we do a new capture!
        // {
        for(u32 BoxIndex = 0;
            BoxIndex < Solution->BoxCount;
            ++BoxIndex)
        {
            lighting_box *Box = Solution->Boxes + BoxIndex;
            
            v3 OldP = Box->BoxMin;
            v3 OldR = Box->BoxMax;
            
            Box->BoxMin = OldP - OldR;
            Box->BoxMax = OldP + OldR;
        }
        // }
        
        if(RepeatCount == 1)
        {
            fprintf(stderr, "Total input boxes: %u\n", Solution->BoxCount);
        }
        
        LARGE_INTEGER StartTime, EndTime;
        QueryPerformanceCounter(&StartTime);
        u32 WorkCount = InternalLightingCore(Solution, &SpecAtlas, &DiffuseAtlas, 0, RepeatCount);
        QueryPerformanceCounter(&EndTime);
        
        LARGE_INTEGER Freq;
        QueryPerformanceFrequency(&Freq);
        
        u64 TotalCastsInitiated = 0;
        u64 TotalPartitionsTested = 0;
        u64 TotalLeavesTested = 0;
        u64 TotalPartitionLeavesUsed = 0;
        for(u32 WorkIndex = 0;
            WorkIndex < WorkCount;
            ++WorkIndex)
        {
            lighting_work *Work = Solution->Works + WorkIndex;
            TotalCastsInitiated += Work->TotalCastsInitiated;
            TotalPartitionsTested += Work->TotalPartitionsTested;
            TotalLeavesTested += Work->TotalLeavesTested;
            TotalPartitionLeavesUsed += Work->TotalPartitionLeavesUsed;
        }
        
        if(RepeatCount == 1)
        {
            if(!StringsAreEqual(ResultLightBoxes.Count, (char *)ResultLightBoxes.Data,
                                Solution->BoxCount * sizeof(*Solution->Boxes), (char *)Solution->Boxes))
            {
                fprintf(stderr, "ERROR: Resulting light boxes don't match!\n");
            }
            
            f32 *ExpectedTexels = (f32 *)ResultSpecAtlas.Data;
            f32 *GotTexels = (f32 *)GetLightAtlasTexels(&SpecAtlas);
            umm TexelCount = GetLightAtlasTexelCount(&SpecAtlas);
            
            f32 MaxError = 0;
            f32 TotalError = 0;
            umm Count = TexelCount;
            while(Count--)
            {
                f32 Expected = *ExpectedTexels++;
                f32 Got = *GotTexels++;
                
                f32 GotError = AbsoluteValue(Expected - Got);
                
                if(MaxError < GotError)
                {
                    MaxError = GotError;
                }
                
                TotalError += GotError;
            }
            
            fprintf(stderr, "Total boxes: %u\n", Solution->BoxCount);
            fprintf(stderr, "Total texels: %f\n", (f32)TexelCount);
            fprintf(stderr, "Total error: %f\n", TotalError);
            fprintf(stderr, "Total error/texel: %f\n", TotalError / (f32)TexelCount);
            fprintf(stderr, "Max error/texel: %f\n", MaxError);
        }
        else
        {
            double Diff = (double)(EndTime.QuadPart - StartTime.QuadPart);
            double TotalSeconds = Diff / (double)Freq.QuadPart;
            fprintf(stdout, "T:%.0f P:%.0f L:%.0f P/L:%.2f\n",
                    (double)TotalPartitionsTested + (double)TotalLeavesTested,
                    (double)TotalPartitionsTested, (double)TotalLeavesTested,
                    (double)TotalPartitionsTested / (double)TotalLeavesTested);
            fprintf(stdout, "P/ray:%.0f L/ray:%.0f\n",
                    (double)TotalPartitionsTested / (double)TotalCastsInitiated,
                    (double)TotalLeavesTested / (double)TotalCastsInitiated);
            fprintf(stdout, "Total Seconds Elapsed: %f\n", TotalSeconds);
            fprintf(stdout, "Expected ms/thread: %.02f\n", (1000.0*(TotalSeconds / (double)RepeatCount)) / 6.0);
        }
    }
    else
    {
        fprintf(stderr, "Unable to run raycast test due to missing data.\n");
    }
}

int
main(int ArgCount, char **Args)
{
    if(ArgCount == 1)
    {
#if RECORD_RAYCAST_STACK
        LeafBoxOut = fopen("leaf_box_traversal.txt", "w");
        PartitionBoxOut = fopen("partition_box_traversal.txt", "w");
#endif
        
        ProfileRun(1);
#if !RECORD_RAYCAST_STACK
        ProfileRun(60*2);
#endif
        
#if RECORD_RAYCAST_STACK
        fclose(LeafBoxOut);
        fclose(PartitionBoxOut);
#endif
    }
    else
    {
        fprintf(stderr, "Usage: %s\n", Args[0]);
    }
}
