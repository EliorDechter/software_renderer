/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#define LIGHTING_USE_FOUR_RAYS 0
#define LIGHTING_USE_OLD_KD 1

#if !defined(RECORD_LEAF_BOX)
#define RECORD_LEAF_BOX(...)
#define RECORD_PARTITION_BOX(...)
#define RECORD_RAYCAST_END(...)
#define RECORD_PARTITION_PUSH(...)
#endif

global b32x LightBoxDumpTrigger = false;

internal lighting_box *
GetBox(lighting_solution *Solution, u32 BoxIndex)
{
    Assert(BoxIndex < Solution->BoxCount);
    lighting_box *Result = Solution->Boxes + BoxIndex;
    return(Result);
}

internal u16
AddBoxStorage(lighting_solution *Solution, u16 BoxCount = 1)
{
    Assert((Solution->BoxCount + BoxCount) < MAX_LIGHT_BOX_COUNT);
    u16 Result = Solution->BoxCount;
    Solution->BoxCount += BoxCount;
    return(Result);
}

internal void
PushDebugLine(lighting_solution *Solution, v3 FromP, v3 ToP, v4 Color)
{
    if(Solution->UpdateDebugLines)
    {
        Assert(Solution->DebugLineCount < ArrayCount(Solution->DebugLines));
        debug_line *Line = Solution->DebugLines + Solution->DebugLineCount++;
        Line->FromP = FromP;
        Line->ToP = ToP;
        Line->Color = Color;
    }
}

internal void
PushDebugBox(lighting_solution *Solution, v3 BoxMin, v3 BoxMax, v4 BoxColor)
{
#if 1
    PushDebugLine(Solution,
                  V3(BoxMin.x, BoxMin.y, BoxMin.z),
                  V3(BoxMin.x, BoxMax.y, BoxMin.z),
                  BoxColor);
    PushDebugLine(Solution,
                  V3(BoxMin.x, BoxMax.y, BoxMin.z),
                  V3(BoxMin.x, BoxMax.y, BoxMax.z),
                  BoxColor);
    PushDebugLine(Solution,
                  V3(BoxMin.x, BoxMax.y, BoxMax.z),
                  V3(BoxMin.x, BoxMin.y, BoxMax.z),
                  BoxColor);
    PushDebugLine(Solution,
                  V3(BoxMin.x, BoxMin.y, BoxMax.z),
                  V3(BoxMin.x, BoxMin.y, BoxMin.z),
                  BoxColor);
    
    PushDebugLine(Solution,
                  V3(BoxMax.x, BoxMin.y, BoxMin.z),
                  V3(BoxMax.x, BoxMax.y, BoxMin.z),
                  BoxColor);
    PushDebugLine(Solution,
                  V3(BoxMax.x, BoxMax.y, BoxMin.z),
                  V3(BoxMax.x, BoxMax.y, BoxMax.z),
                  BoxColor);
    PushDebugLine(Solution,
                  V3(BoxMax.x, BoxMax.y, BoxMax.z),
                  V3(BoxMax.x, BoxMin.y, BoxMax.z),
                  BoxColor);
    PushDebugLine(Solution,
                  V3(BoxMax.x, BoxMin.y, BoxMax.z),
                  V3(BoxMax.x, BoxMin.y, BoxMin.z),
                  BoxColor);
    
    PushDebugLine(Solution,
                  V3(BoxMin.x, BoxMin.y, BoxMin.z),
                  V3(BoxMax.x, BoxMin.y, BoxMin.z),
                  BoxColor);
    PushDebugLine(Solution,
                  V3(BoxMin.x, BoxMax.y, BoxMin.z),
                  V3(BoxMax.x, BoxMax.y, BoxMin.z),
                  BoxColor);
    PushDebugLine(Solution,
                  V3(BoxMin.x, BoxMax.y, BoxMax.z),
                  V3(BoxMax.x, BoxMax.y, BoxMax.z),
                  BoxColor);
    PushDebugLine(Solution,
                  V3(BoxMin.x, BoxMin.y, BoxMax.z),
                  V3(BoxMax.x, BoxMin.y, BoxMax.z),
                  BoxColor);
#endif
}

internal lighting_box *
GetLightLeafForP(lighting_solution *Solution, v3 P, b32x IsTestCase)
{
    // NOTE(casey): This does not actually guarantee you the closest light source!
    
    lighting_box *Result = GetBox(Solution, Solution->RootLightBoxIndex);
    while(Result->Pack.ChildCount)
    {
        u32 FirstChildIndex = Result->Pack.FirstChildIndex;
        u32 OnePastLastChildIndex = (u32)(FirstChildIndex + Result->Pack.ChildCount);
        
        f32 ClosestDSq = F32Max;
        for(u32 SourceIndex = FirstChildIndex;
            SourceIndex < OnePastLastChildIndex;
            ++SourceIndex)
        {
            lighting_box *Box = GetBox(Solution, SourceIndex);
            rectangle3 Extents = RectMinMax(Box->BoxMin, Box->BoxMax);
            v3 ClosestP = GetClosestPointInBox(Extents, P);
            if(IsTestCase)
            {
                PushDebugLine(Solution, P, ClosestP, V4(1, 1, 1, .5f));
                PushDebugBox(Solution, Extents.Min, Extents.Max, V4(1, 1, 1, .5f));
            }
            f32 ThisDSq = LengthSq(P - ClosestP);
            if(ClosestDSq > ThisDSq)
            {
                ClosestDSq = ThisDSq;
                Result = Box;
            }
        }
        
        if(IsTestCase)
        {
            rectangle3 Extents = RectMinMax(Result->BoxMin, Result->BoxMax);
            PushDebugBox(Solution, Extents.Min, Extents.Max, V4(1, 1, 0, 1));
        }
    }
    
    return(Result);
}

internal v3
GetVoxelCenterP(lighting_solution *Solution, v3s Index)
{
    v3 Result = Solution->VoxMinCornerHot + Hadamard(Solution->VoxCellDim, V3(Index) + V3(0.5f, 0.5f, 0.5f));
    return(Result);
}

internal v3
ComputeVoxelIrradianceAt(lighting_solution *Solution, light_atlas *DiffuseAtlas, v3 SampleP, v3 SampleN, v3 Outgoing)
{
    // TODO(casey): Eventually this should probably be done wide, because WIDENESSSSSSS
    
    v3 FCoord = Hadamard(Solution->InvVoxCellDim, (SampleP- Solution->VoxMinCornerHot)) - V3(0.5f, 0.5f, 0.5f);
    v3s VoxelIndex = FloorToV3S(FCoord);
    v3 BCoord = V3(VoxelIndex);
    v3 UVW = FCoord - BCoord;
    
    v3 R = SampleN;
    v2u T = GetOctahedralOffset(DiffuseAtlas->OctDimCoefficient, R);
    u32 Tx = T.x;
    u32 Ty = T.y;
    
    light_atlas_texel Tiles[2][2][2] =
    {
        GetTileClamped(DiffuseAtlas, VoxelIndex.x    , VoxelIndex.y    , VoxelIndex.z, Tx, Ty),
        GetTileClamped(DiffuseAtlas, VoxelIndex.x + 1, VoxelIndex.y    , VoxelIndex.z, Tx, Ty),
        GetTileClamped(DiffuseAtlas, VoxelIndex.x    , VoxelIndex.y + 1, VoxelIndex.z, Tx, Ty),
        GetTileClamped(DiffuseAtlas, VoxelIndex.x + 1, VoxelIndex.y + 1, VoxelIndex.z, Tx, Ty),
        GetTileClamped(DiffuseAtlas, VoxelIndex.x    , VoxelIndex.y    , VoxelIndex.z + 1, Tx, Ty),
        GetTileClamped(DiffuseAtlas, VoxelIndex.x + 1, VoxelIndex.y    , VoxelIndex.z + 1, Tx, Ty),
        GetTileClamped(DiffuseAtlas, VoxelIndex.x    , VoxelIndex.y + 1, VoxelIndex.z + 1, Tx, Ty),
        GetTileClamped(DiffuseAtlas, VoxelIndex.x + 1, VoxelIndex.y + 1, VoxelIndex.z + 1, Tx, Ty),
    };
    
    v3 Result = Lerp(Lerp(Lerp(*Tiles[0][0][0].Value, UVW.x, *Tiles[0][0][1].Value), UVW.y,
                          Lerp(*Tiles[0][1][0].Value, UVW.x, *Tiles[0][1][1].Value)), UVW.z,
                     Lerp(Lerp(*Tiles[1][0][0].Value, UVW.x, *Tiles[1][0][1].Value), UVW.y,
                          Lerp(*Tiles[1][1][0].Value, UVW.x, *Tiles[1][1][1].Value)));
    
    return(Result);
}

internal void
UpdateVoxel(light_atlas *Atlas, light_atlas_texel Tile, v2u TOffset, f32 tUpdateBlend, v3 LightC, f32 LightD)
{
    light_atlas_texel Texel = OffsetFromTexel(Atlas, Tile, TOffset);
    *Texel.Value = Lerp(*Texel.Value, tUpdateBlend, LightC);
}

internal b32x
LooksFishy(f32 A)
{
    b32x Result = !((A >= -10000.0f) &&
                    (A <=  10000.0f));
    return(Result);
}

internal b32x
LooksFishy(v3 A)
{
    b32x Result = (LooksFishy(A.x) ||
                   LooksFishy(A.y) ||
                   LooksFishy(A.z));
    
    return(Result);
}

inline void
GridRayCast(lighting_work *Work, light_atlas *DiffuseAtlas,
            v3 RayOriginSingle, v3 RayDSingle, 
            u16 InitialGridIndex, s16 *WalkTable, 
            v3 *TransferPPS)
{
    lighting_solution *Solution = Work->Solution;
    f32 tUpdateBlend = Solution->tUpdateBlend;
    
    Work->TotalCastsInitiated += 1;
    
    f32_4x tRay = F32_4x(F32Max);
    
    f32_4x One = F32_4x(1.0f);
    v3_4x RayD = V3_4x(RayDSingle);
    v3_4x RayOrigin = V3_4x(RayOriginSingle);
    
    v3_4x InvRayD = V3_4x(1.0f, 1.0f, 1.0f, 1.0f) / RayD;
    
    f32_4x HitTMinX = {};
    f32_4x HitTMinY = {};
    v3_4x HitRefColor = {};
    f32_4x HitEmission = {};
    
    u16 GridIndex = InitialGridIndex;
    for(;;)
    {
        lighting_spatial_grid_node Node = Solution->SpatialGridNodes[GridIndex];
        if(Node.StartIndex == SPATIAL_GRID_NODE_TERMINATOR)
        {
            break;
        }
        else
        {
            // TODO(casey): We need to also try a version that is single-leaf spatial
            // structures that we then interleave in a pre-walk, because that would not
            // suffer from the underfull leaf case like this one does.
            for(u16 LeafIndexIndex = Node.StartIndex;
                LeafIndexIndex < Node.OnePastLastIndex;
                ++LeafIndexIndex)
            {
                lighting_spatial_grid_leaf Leaf = Solution->SpatialGridLeaves[LeafIndexIndex];
                
                v3_4x BoxMin = Leaf.BoxMin;
                v3_4x BoxMax = Leaf.BoxMax;
                
                v3_4x tBoxMin = Hadamard((BoxMin - RayOrigin), InvRayD);
                v3_4x tBoxMax = Hadamard((BoxMax - RayOrigin), InvRayD);
                
                v3_4x tMin3 = Min(tBoxMin, tBoxMax);
                v3_4x tMax3 = Max(tBoxMin, tBoxMax);
                
                f32_4x tMin = Max(tMin3.x, Max(tMin3.y, tMin3.z));
                f32_4x tMax = Min(tMax3.x, Min(tMax3.y, tMax3.z));
                
                //f32_4x MaxPass = (tMax >= ZeroF32_4x());
                //f32_4x tInside = (MaxPass & (tMin <= ZeroF32_4x()));
                
                // TODO(casey): Do we actually need (tMin < tMax) here?
                f32_4x tValid = (tMin > ZeroF32_4x()) & (tMin < tMax);
                f32_4x Mask = tValid & (tMin < tRay);
                
                tRay = Select(tRay, Mask, tMin);
                HitTMinX = Select(HitTMinX, Mask, tMin3.x);
                HitTMinY = Select(HitTMinY, Mask, tMin3.y);
                HitRefColor = Select(HitRefColor, Mask, Leaf.RefColor);
                HitEmission = Select(HitEmission, Mask, Leaf.IsEmission);
            }
            
            GridIndex += *WalkTable++;
        }
    }
    
#if 0
    __m128 tRayTo16 = {};
    __m128 tRayScaled = _mm_mul_ps(tRay.P, tRayTo16);
    __m128i tRayInt = _mm_cvtps_epi32(tRayScaled);
    __m128i HComp = _mm_packs_epi32(tRayInt, tRayInt);
#else
    alignas(16) u8 HCompShuffler[16] = {2, 3, 6, 7, 10, 11, 14, 15, 2, 3, 2, 3, 2, 3, 2, 3};
    __m128i HComp = _mm_shuffle_epi8(_mm_castps_si128(tRay.P), *(__m128i *)HCompShuffler);
#endif
    
    __m128i MinTest = _mm_minpos_epu16(HComp);
    int ShuffleIndex = _mm_extract_epi16(MinTest, 2);
    Assert(ShuffleIndex < 4);
    alignas(16) u8 ShuffleTable[4][16] = 
    {
        { 0,  1,  2,  3,   0,  1,  2,  3,   0,  1,  2,  3,   0,  1,  2,  3},
        { 4,  5,  6,  7,   4,  5,  6,  7,   4,  5,  6,  7,   4,  5,  6,  7},
        { 8,  9, 10, 11,   8,  9, 10, 11,   8,  9, 10, 11,   8,  9, 10, 11},
        {12, 13, 14, 15,  12, 13, 14, 15,  12, 13, 14, 15,  12, 13, 14, 15},
    };
    __m128i Shuffler = *(__m128i *)ShuffleTable[ShuffleIndex];
    
    tRay = PShufB(tRay, Shuffler);
    f32 tRaySingle = Extract0(tRay);
    
    v3 EmissionDirection = -RayDSingle;
    if(tRaySingle != F32Max)
    {
        v3_4x ProbeSampleP = RayOrigin + tRay*RayD;
        
        f32_4x SignX = InverseSignBitFrom(RayD.x);
        f32_4x SignY = InverseSignBitFrom(RayD.y);
        f32_4x SignZ = InverseSignBitFrom(RayD.z);
        
        f32_4x XMask = (PShufB(HitTMinX, Shuffler) == tRay);
        f32_4x YMask = AndNot(PShufB(HitTMinY, Shuffler) == tRay, XMask);
        f32_4x ZMask = (XMask | YMask);
        
        f32_4x NormalX = SignX | (One & XMask);
        f32_4x NormalY = SignY | (One & YMask);
        f32_4x NormalZ = SignZ | AndNot(One, ZMask);
        
        // TODO(casey): Probably leave all these in lanes, and convert
        // ComputeVoxelIrradianceAt to use lanes instead
        v3 ProbeSampleNSingle = 
        {
            Extract0(NormalX), 
            Extract0(NormalY),
            Extract0(NormalZ),
        };
        v3 ProbeSamplePSingle = 
        {
            Extract0(ProbeSampleP.x), 
            Extract0(ProbeSampleP.y),
            Extract0(ProbeSampleP.z),
        };
        v3 ReflectColorFoo = ComputeVoxelIrradianceAt(Solution, DiffuseAtlas, ProbeSamplePSingle,
                                                      ProbeSampleNSingle, EmissionDirection);
        f32 ReflectFalloff = Clamp01(Inner(ProbeSampleNSingle, EmissionDirection));
        ReflectColorFoo *= ReflectFalloff;
        
        f32_4x ReflectLevel = F32_4x(ReflectColorFoo.r, ReflectColorFoo.g, ReflectColorFoo.b, 0);
        f32_4x EmissionLevel = PShufB(HitEmission, Shuffler);
        f32_4x TransmissionLevel = Max(ReflectLevel, EmissionLevel);
        
        TransferPPS->r = Extract0(TransmissionLevel)*Extract0(PShufB(HitRefColor.r, Shuffler));
        TransferPPS->g = Extract1(TransmissionLevel)*Extract0(PShufB(HitRefColor.g, Shuffler));
        TransferPPS->b = Extract2(TransmissionLevel)*Extract0(PShufB(HitRefColor.b, Shuffler));
    }
    
    RECORD_RAYCAST_END();
}

struct kd_stack_pack
{
    u32 Bits;
    f32 D;
};

inline void
RayCast(lighting_work *Work, v3_4x RayOrigin, v3_4x RayD,
        light_atlas *SpecAtlas,
        light_atlas *DiffuseAtlas,
        light_atlas_texel SpecTexel,
        light_sample_direction *SampleDirections,
        u16 InitialGridIndex,
        b32x Debugging = false)
{
    lighting_solution *Solution = Work->Solution;
    f32 tUpdateBlend = Solution->tUpdateBlend;
    
    // TODO(casey): Make this procedural in some way, since we don't want to add moonlight in
    // to areas that aren't outside.
    v3 MoonColor = 0.1f*V3(0.10f, 0.80f, 1.00f);
    v3 MoonDir = NOZ(V3(0.25f, -0.25f, 1.0f));
    
    Work->TotalCastsInitiated += 4;
    
    u32 Depth = 0;
    
    f32_4x tRay = F32_4x(F32Max);
    
    f32_4x One = F32_4x(1.0f);
#if 0
    f32_4x NegativeOne = F32_4x(-1.0f);
#endif
    
    f32_4x tCloseEnough = F32_4x(15.0f);
    v3_4x InvRayD = V3_4x(1.0f, 1.0f, 1.0f, 1.0f) / RayD;
    
    v3_4x HitBoxMin = {};
    v3_4x HitBoxMax = {};
    v3_4x HitRefColor = {};
    f32_4x HitEmission = {};
    
#if 0
    if(DebugThis)
    {
        v3 SampleD = GetComponent(RayDirection, SubRay);
        b32 Hit = tHit.U32[SubRay];
        PushDebugLine(Solution, LightProbeP,
                      Hit ? GetComponent(Ray.P, SubRay) : LightProbeP + 0.25f*SampleD,
                      Hit ? V4(0, 1, 1, 1) : V4(1, 1, 0, 1));
    }
#endif
    
#if LIGHTING_USE_FOUR_RAYS
    // TODO(casey): Try with a circular buffer?
#if LIGHTING_USE_OLD_KD
    
    u32 BoxStack[64];
    lighting_box *StartingBox = GetBox(Solution, Solution->RootBoxIndex);
    BoxStack[Depth++] = *(u32 *)&StartingBox->Pack;
    
    while(Depth > 0)
    {
        lighting_box_pack Entry = *(lighting_box_pack *)&BoxStack[--Depth];
        
        for(u32 SourceIndex = Entry.FirstChildIndex;
            SourceIndex < (u32)(Entry.FirstChildIndex + Entry.ChildCount);
            ++SourceIndex)
        {
            RECORD_PARTITION_BOX(SourceIndex);
            
            lighting_box *Box = GetBox(Solution, SourceIndex);
            ++Work->TotalPartitionsTested;
            
            b32x AnyCloseEnough;
            b32x AnyInside;
            b32x ShouldPush;
            {
                v3_4x BoxMin = V3_4x(Box->BoxMin);
                v3_4x BoxMax = V3_4x(Box->BoxMax);
                
                v3_4x tBoxMin = Hadamard((BoxMin - RayOrigin), InvRayD);
                v3_4x tBoxMax = Hadamard((BoxMax - RayOrigin), InvRayD);
                
                v3_4x tMin3 = Min(tBoxMin, tBoxMax);
                v3_4x tMax3 = Max(tBoxMin, tBoxMax);
                
                f32_4x tMin = Max(tMin3.x, Max(tMin3.y, tMin3.z));
                f32_4x tMax = Min(tMax3.x, Min(tMax3.y, tMax3.z));
                
                f32_4x MaxPass = (tMax >= ZeroF32_4x());
                f32_4x tInside = (MaxPass & (tMin <= ZeroF32_4x()));
                f32_4x tValid = (tMin > ZeroF32_4x()) & (tMin < tMax); // TODO(casey): Do we actually need (tMin < tMax) here?
                f32_4x Mask = tValid & (tMin < tRay);
                f32_4x CloseEnough = Mask & (tMin < tCloseEnough);
                
                AnyCloseEnough = AnyTrue(CloseEnough);
                AnyInside = AnyTrue(tInside);
                ShouldPush = (AnyInside || AnyCloseEnough);
            }
            
            // NOTE(casey): Current 'fast' path
            
            if(ShouldPush && (Box->Pack.IsLight_IsLeafContainer & LightBox_IsLeafContainer))
            {
                for(u32 LeafIndex = Box->Pack.FirstChildIndex;
                    LeafIndex < (u32)(Box->Pack.FirstChildIndex + Box->Pack.ChildCount);
                    ++LeafIndex)
                {
                    RECORD_LEAF_BOX(SourceIndex);
                    
                    lighting_box *Leaf = GetBox(Solution, LeafIndex);
                    ++Work->TotalLeavesTested;
                    
                    v3_4x BoxMin = V3_4x(Leaf->BoxMin);
                    v3_4x BoxMax = V3_4x(Leaf->BoxMax);
                    
                    v3_4x tBoxMin = Hadamard((BoxMin - RayOrigin), InvRayD);
                    v3_4x tBoxMax = Hadamard((BoxMax - RayOrigin), InvRayD);
                    
                    v3_4x tMin3 = Min(tBoxMin, tBoxMax);
                    v3_4x tMax3 = Max(tBoxMin, tBoxMax);
                    
                    f32_4x tMin = Max(tMin3.x, Max(tMin3.y, tMin3.z));
                    f32_4x tMax = Min(tMax3.x, Min(tMax3.y, tMax3.z));
                    
                    f32_4x MaxPass = (tMax >= ZeroF32_4x());
                    
                    f32_4x tInside = (MaxPass & (tMin <= ZeroF32_4x()));
                    f32_4x tValid = (tMin > ZeroF32_4x()) & (tMin < tMax); // TODO(casey): Do we actually need (tMin < tMax) here?
                    f32_4x Mask = tValid & (tMin < tRay);
                    f32_4x CloseEnough = Mask & (tMin < tCloseEnough);
                    
                    f32 IsEmission = (Leaf->Pack.IsLight_IsLeafContainer & LightBox_IsLight) ? 1.0f : 0.0f;
                    tRay = Select(tRay, Mask, tMin);
                    HitBoxMin = Select(HitBoxMin, Mask, BoxMin);
                    HitBoxMax = Select(HitBoxMax, Mask, BoxMax);
                    HitRefColor = Select(HitRefColor, Mask, V3_4x(Leaf->RefC));
                    HitEmission = Select(HitEmission, Mask, F32_4x(IsEmission));
                }
            }
            else
            {
                BoxStack[Depth] = ShouldPush ? *(u32 *)&Box->Pack : BoxStack[Depth];
                Depth = ShouldPush ? (Depth + 1) : Depth;
            }
            
            RECORD_PARTITION_PUSH(ShouldPush, Box->Pack);
        }
    }
#else
    
#define PACK_KD_STACK(PlaneD, PackedSide) (((u64)(*(u32 *)&(PlaneD)) << 32) | (u64)(PackedSide))
#define UNPACK(Pack) ( (kd_stack_pack *)&(Pack) )
#define UNPACK_PLANE_D(Pack) UNPACK(Pack)->D
#define UNPACK_LEAF_COUNT(Pack) ((u8) ((UNPACK(Pack)->Bits) >> 16) & 0xff)
#define UNPACK_NODE_INDEX(Pack) ((u16) (UNPACK(Pack)->Bits) & 0xffff)
#define UNPACK_PLANE_TEST_PATTERN(Pack) ((u8)((UNPACK(Pack)->Bits >> 24) & 0xf))
#define UNPACK_KD_INDEX(Pack) ((u8) ((UNPACK(Pack)->Bits) >> 28))
#define UNPACK_NEXT_KD_INDEX(Pack) ((u8) ((UNPACK(Pack)->Bits) >> 30))
    
    u64 KdStack[64];
    
    u8 LeafCount = UNPACK_LEAF_COUNT(Solution->RootKdValue);
    u16 CurStackValue = UNPACK_NODE_INDEX(Solution->RootKdValue);
    u8 KdIndex = UNPACK_NEXT_KD_INDEX(Solution->RootKdValue);
    do
    {
        if(LeafCount)
        {
            u32 LeafIndex = CurStackValue;
            u32 OnePastLastLeafIndex = LeafIndex + LeafCount;
            do
            {
                RECORD_LEAF_BOX(SourceIndex);
                
                lighting_box *Leaf = GetBox(Solution, LeafIndex);
                ++Work->TotalLeavesTested;
                
                v3_4x BoxMin = V3_4x(Leaf->BoxMin);
                v3_4x BoxMax = V3_4x(Leaf->BoxMax);
                
                v3_4x tBoxMin = Hadamard((BoxMin - RayOrigin), InvRayD);
                v3_4x tBoxMax = Hadamard((BoxMax - RayOrigin), InvRayD);
                
                v3_4x tMin3 = Min(tBoxMin, tBoxMax);
                v3_4x tMax3 = Max(tBoxMin, tBoxMax);
                
                f32_4x tMin = Max(tMin3.x, Max(tMin3.y, tMin3.z));
                f32_4x tMax = Min(tMax3.x, Min(tMax3.y, tMax3.z));
                
                f32_4x MaxPass = (tMax >= ZeroF32_4x());
                
                f32_4x tInside = (MaxPass & (tMin <= ZeroF32_4x()));
                f32_4x tValid = (tMin > ZeroF32_4x()) & (tMin < tMax); // TODO(casey): Do we actually need (tMin < tMax) here?
                f32_4x Mask = tValid & (tMin < tRay);
                f32_4x CloseEnough = Mask & (tMin < tCloseEnough);
                
                f32 IsEmission = (Leaf->Pack.IsLight_IsLeafContainer & LightBox_IsLight) ? 1.0f : 0.0f;
                tRay = Select(tRay, Mask, tMin);
                HitBoxMin = Select(HitBoxMin, Mask, BoxMin);
                HitBoxMax = Select(HitBoxMax, Mask, BoxMax);
                HitRefColor = Select(HitRefColor, Mask, V3_4x(Leaf->RefC));
                HitEmission = Select(HitEmission, Mask, F32_4x(IsEmission));
            } while(++LeafIndex < OnePastLastLeafIndex);
            
            CurStackValue = 0;
            while(Depth > 0)
            {
                u64 PackedValue = KdStack[--Depth];
                u16 TestNodeIndex = UNPACK_NODE_INDEX(PackedValue);
                u32 PlaneTestPattern = UNPACK_PLANE_TEST_PATTERN(PackedValue);
                f32 PlaneD = UNPACK_PLANE_D(PackedValue);
                u8 OldKdIndex = UNPACK_KD_INDEX(PackedValue);
                
                f32_4x StartD = RayOrigin.E[OldKdIndex];
                f32_4x DirD = RayD.E[OldKdIndex];
                f32_4x EndD = StartD + tRay*DirD - F32_4x(PlaneD);
                
                u32 EndSideBit = _mm_movemask_ps(EndD.P);
                if(EndSideBit != PlaneTestPattern)
                {
                    LeafCount = UNPACK_LEAF_COUNT(PackedValue);
                    KdIndex = UNPACK_NEXT_KD_INDEX(PackedValue);
                    CurStackValue = TestNodeIndex;
                    break;
                }
            }
        }
        else
        {
            ++Work->TotalPartitionsTested;
            
            kd_tree_node *Kd = Solution->KdNodes + CurStackValue;
            f32_4x StartSide = RayOrigin.E[KdIndex] - F32_4x(Kd->PlaneD);
            u32 StartSideBit = (_mm_movemask_ps(StartSide.P) & 0x1);
            
            KdStack[Depth++] = PACK_KD_STACK(Kd->PlaneD, Kd->Sides[StartSideBit ^ 0x1]);
            
            u64 PackedValue = Kd->Sides[StartSideBit];
            LeafCount = UNPACK_LEAF_COUNT(PackedValue);
            CurStackValue = UNPACK_NODE_INDEX(PackedValue);
            KdIndex = UNPACK_NEXT_KD_INDEX(PackedValue);
        } 
    } while(CurStackValue);
    
#undef UNPACK
#undef UNPACK_PLANE_D
#undef UNPACK_LEAF_COUNT
#undef UNPACK_NODE_INDEX
#undef UNPACK_PLANE_TEST_PATTERN
#undef UNPACK_KD_INDEX
#undef UNPACK_NEXT_KD_INDEX
    
#endif
    
    v3_4x HitBoxRadius = 0.5f*(HitBoxMax - HitBoxMin);
    v3_4x HitBoxCenter = 0.5f*(HitBoxMax + HitBoxMin);
    
    v3_4x HitP = RayOrigin + tRay*RayD;
    v3_4x HitBoxInvRadius = ApproxOneOver(HitBoxRadius);
    v3_4x UnscaledD = HitP - HitBoxCenter;
    v3_4x ScaledD = Hadamard(HitBoxInvRadius, UnscaledD);
    
    // TODO(casey): We can probably get rid of this normal computation entirely
    // because we know we just need the attenuation factor and the Tx/Ty, so we
    // can just compute those directly and potentially save work.
    
    // TODO(casey): If we don't end up deleting this case, we probably
    // need to do one AND instruction here to make this use the absolute value for
    // comparisons?
    
    // TODO(casey): Is there a way to do this with less compares?
    // TODO(casey): Verify that this produces correctly only-one-1 normals for all equivalences
    f32_4x XMask = (ScaledD.x <= ScaledD.y) & (ScaledD.x <= ScaledD.z);
    f32_4x YMask = (ScaledD.y < ScaledD.x) & (ScaledD.y <= ScaledD.z);
    f32_4x ZMask = (ScaledD.z < ScaledD.x) & (ScaledD.z < ScaledD.y);
    
    v3_4x HitNormal;
    HitNormal.x = XMask & (One | SignBitFrom(ScaledD.x));
    HitNormal.y = YMask & (One | SignBitFrom(ScaledD.y));
    HitNormal.z = ZMask & (One | SignBitFrom(ScaledD.z));
    
    v3 TransferPPS[4] = {};
    for(u32 SubRay = 0;
        SubRay < 4;
        ++SubRay)
    {
        f32_4x tHit = (F32_4x(F32Max) != tRay);
        v3 EmissionDirection = -GetComponent(RayD, SubRay);
        if(tHit.U32[SubRay])
        {
            v3 ProbeSampleP = GetComponent(HitP, SubRay);
            v3 ProbeSampleN = GetComponent(HitNormal, SubRay);
            v3 SampleRefColor = GetComponent(HitRefColor, SubRay);
            f32 Emission = HitEmission.E[SubRay];
            f32 RefFalloff = (1.0f - Emission)*Clamp01(Inner(ProbeSampleN, EmissionDirection));
            v3 LightSample = ComputeVoxelIrradianceAt(Solution, DiffuseAtlas, ProbeSampleP, ProbeSampleN, EmissionDirection);
            TransferPPS[SubRay] = RefFalloff*Hadamard(LightSample, SampleRefColor) + Emission*SampleRefColor;
        }
        else
        {
            // TODO(casey): We probably want this to come from the light
            // probe eventually?
            TransferPPS[SubRay] = Clamp01(Inner(MoonDir, -EmissionDirection))*MoonColor;
        }
    }
    
#else
    v3 RayOriginSingle = GetComponent(RayOrigin, 0);
    v3 TransferPPS[4];
    for(u32 RayIndex = 0;
        RayIndex < 4;
        ++RayIndex)
    {
        v3 ThisRayD = SampleDirections[RayIndex].RayD;
        TransferPPS[RayIndex] = Clamp01(Inner(MoonDir, -ThisRayD))*MoonColor;
        
        s16 *WalkTable = LightSamplingWalkTable + SampleDirections[RayIndex].WalkTableOffset;
        GridRayCast(Work, DiffuseAtlas, RayOriginSingle, ThisRayD,
                    InitialGridIndex, WalkTable,
                    &TransferPPS[RayIndex]);
    }
#endif
    
    f32_4x S0 = LoadF32_4X((f32_4x *)SpecTexel.Value + 0);
    f32_4x S1 = LoadF32_4X((f32_4x *)SpecTexel.Value + 1);
    f32_4x S2 = LoadF32_4X((f32_4x *)SpecTexel.Value + 2);
    
    f32_4x T0 = LoadF32_4X((f32_4x *)TransferPPS + 0);
    f32_4x T1 = LoadF32_4X((f32_4x *)TransferPPS + 1);
    f32_4x T2 = LoadF32_4X((f32_4x *)TransferPPS + 2);
    
    f32_4x Blend = F32_4x(tUpdateBlend);
    f32_4x InvBlend = F32_4x(1.0f - tUpdateBlend);
    
    S0 = InvBlend*S0 + Blend*T0;
    S1 = InvBlend*S1 + Blend*T1;
    S2 = InvBlend*S2 + Blend*T2;
    
    StoreF32_4X(S0, (f32_4x *)SpecTexel.Value + 0);
    StoreF32_4X(S1, (f32_4x *)SpecTexel.Value + 1);
    StoreF32_4X(S2, (f32_4x *)SpecTexel.Value + 2);
    
    RECORD_RAYCAST_END();
}

internal void
FullCast(lighting_work *Work, light_atlas_texel Tile, v3 VoxCenterP, u32 EntropyIndex,
         u16 InitialGridIndex)
{
    lighting_solution *Solution = Work->Solution;
    light_atlas *SpecAtlas = Work->SpecularLightAtlas;
    light_atlas *DiffuseAtlas = Work->DiffuseLightAtlas;
    
    // TODO(casey): We should probably ask for places other than
    // the pure center of the voxel, but hey.
    v3 LightProbeP = VoxCenterP;
    v3_4x RayOrigin = V3_4x(LightProbeP, LightProbeP, LightProbeP, LightProbeP);
    
#if 0
    b32x DebugThis = (X == ) &&
        (Y == )
        (Z == );
    if(DebugThis)
    {
        PushDebugLine(Solution, LightProbeP, LightProbeP + V3(1, 0, 0), V4(1, 0, 0, 1));
        PushDebugLine(Solution, LightProbeP, LightProbeP + V3(0, 1, 0), V4(0, 1, 0, 1));
        PushDebugLine(Solution, LightProbeP, LightProbeP + V3(0, 0, 1), V4(0, 0, 1, 1));
    }
#endif
    
    u32 DirSampleIndex = EntropyIndex & LIGHT_SAMPLING_SPHERE_MASK;
    Assert(DirSampleIndex < LIGHT_SAMPLING_SPHERE_COUNT);
    light_sampling_sphere *SamplingSphere = Solution->SamplingSpheres + DirSampleIndex;
    light_sampling_sphere_2 *LightSampleDirections = LightSamplingSphereTable2 + DirSampleIndex;
    
    u32 RayBundleIndex = 0;
    for(u32 Sy = 0;
        Sy < (SpecAtlas->TileDim.y - 2);
        ++Sy)
    {
        light_atlas_texel SpecTexelA = OffsetFromTexel(SpecAtlas, Tile, 1, Sy+1);
        light_atlas_texel SpecTexelB = OffsetFromTexel(SpecAtlas, Tile, 5, Sy+1);
        
        v3_4x SampleDirA = SamplingSphere->SampleDirection[RayBundleIndex + 0];
        v3_4x SampleDirB = SamplingSphere->SampleDirection[RayBundleIndex + 1];
        
        RayCast(Work, RayOrigin, SampleDirA, SpecAtlas, DiffuseAtlas, SpecTexelA, 
                LightSampleDirections->SampleDirection + 8*RayBundleIndex + 0, InitialGridIndex);
        RayCast(Work, RayOrigin, SampleDirB, SpecAtlas, DiffuseAtlas, SpecTexelB,
                LightSampleDirections->SampleDirection + 8*RayBundleIndex + 4, InitialGridIndex);
        
        RayBundleIndex += 2;
        Assert(RayBundleIndex <= LIGHT_SAMPLING_RAY_BUNDLES_PER_SPHERE);
    }
}

internal void
TestSphere(lighting_solution *Solution, light_atlas *SpecAtlas, light_atlas_texel Tile, v3 BoxCenterP)
{
    v3 LightP = Solution->DebugLightP;
    f32 LightFalloffRate = 0.1f;
    f32 LightI = 0.1f*MAX_LIGHT_INTENSITY;
    v3 LightColor = {1, 1, 1};
    
    v2 OxyCoefficient = SpecAtlas->OxyCoefficient;
    
    for(u32 Ty = 1;
        Ty < (SpecAtlas->TileDim.y - 1);
        ++Ty)
    {
        for(u32 Tx = 1;
            Tx < (SpecAtlas->TileDim.x - 1);
            ++Tx)
        {
            v3 LightC = {};
            f32 LightD = 0.0f;
            
            v3 SampleDir = DirectionFromTxTy(OxyCoefficient, Tx, Ty);
            
#if 1
            {
                v3 LightN = (LightP - BoxCenterP);
                f32 LightDist = Length(LightN);
                LightN = NOZ(LightN);
                
                f32 LightA = LightI - LightFalloffRate*LightDist;
                LightA = Clamp01(LightA);
                
                LightC = LightColor*LightA*Clamp01(Inner(LightN, SampleDir));
            }
#else
            LightC = V3((SampleDir.x > 0) ? 0.1f : 0.01f,
                        (SampleDir.y > 0) ? 0.1f : 0.01f,
                        (SampleDir.z > 0) ? 0.1f : 0.01f);
#endif
            *OffsetFromTexel(SpecAtlas, Tile, V2U(Tx, Ty)).Value = LightC;
        }
    }
}

internal void
FillLightAtlasBorder(light_atlas *Atlas, u32 X, u32 Y, u32 Z)
{
    u32 N;
    
    v3 *DiffuseTexels = (v3 *)GetLightAtlasTexels(Atlas);
    
    N = Atlas->TileDim.x - 1;
    u32 Row0Start = LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 0, 0);
    u32 Row1Start = LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 0, 1);
    u32 RowNStart = LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 0, N);
    u32 RowN1Start = LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 0, N - 1);
    for(u32 Tx = 1;
        Tx < N;
        ++Tx)
    {
        v3 C0 = DiffuseTexels[Row1Start + N - Tx];
        v3 C1 = DiffuseTexels[RowN1Start + N - Tx];
        
#if EDGE_COLORING
        C0 = V3(1, 0, 0);
        C1 = V3(0, 1, 0);
#endif
        
        DiffuseTexels[Row0Start + Tx] = C0;
        DiffuseTexels[RowNStart + Tx] = C1;
    }
    
    N = Atlas->TileDim.y - 1;
    for(u32 Ty = 1;
        Ty < N;
        ++Ty)
    {
        v3 C0 = DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 1, N - Ty)];
        v3 C1 = DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, N - 1, N - Ty)];
        
#if EDGE_COLORING
        C0 = V3(1, 1, 0);
        C1 = V3(0, 1, 1);
#endif
        
        DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 0, Ty)] = C0;
        DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, N, Ty)] = C1;
    }
    
    DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 0, 0)] =
        DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, N - 1, N - 1)];
    DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, N, 0)] =
        DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 1, N - 1)];
    DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 0, N)] =
        DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, N - 1, 1)];
    DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, N, N)] =
        DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 1, 1)];
    
#if 0
    DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 3, 3)] = V3(1, 0, 0);
    DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 4, 3)] = V3(0, 1, 0);
    DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 5, 3)] = V3(0, 0, 1);
    DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 3, 4)] = V3(1, 1, 0);
    DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 4, 4)] = V3(0, 0, 0);
    DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 5, 4)] = V3(1, 0, 1);
    DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 3, 5)] = V3(0.5f, 1, 0);
    DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 4, 5)] = V3(1, 0.5f, 0);
    DiffuseTexels[LIGHT_ATLAS_OFFSET(Atlas, X, Y, Z, 5, 5)] = V3(0, 0.5f, 1);
#endif
}

internal u16
GridIndexFrom(v3s VoxelDim, u32 X, u32 Y, u32 Z)
{
    u16 Result = SafeTruncateToU16(VoxelDim.x*(VoxelDim.y*VoxelDim.z + Y) + X);
    return(Result);
}

internal PLATFORM_WORK_QUEUE_CALLBACK(ComputeLightPropagationWork)
{
    TIMED_FUNCTION();
    
    lighting_work *Work = (lighting_work *)Data;
    lighting_solution *Solution = Work->Solution;
    light_atlas *DiffuseAtlas = Work->DiffuseLightAtlas;
    light_atlas *SpecAtlas = Work->SpecularLightAtlas;
    
    u32 SamplePointEntropy = Work->SamplePointEntropy;
    u32 LightPointEntropy = Work->LightPointEntropy;
    u32 EntropyIndex = SamplePointEntropy;
    
#define EDGE_COLORING 0
    
    u32 X = Work->VoxelX;
    v3s VoxelDim = SpecAtlas->VoxelDim;
    u32 XOdd = (X&1) ^ Solution->FrameOdd;
    
    for(s32 Z = 0;
        Z < VoxelDim.z;
        ++Z)
    {
        for(s32 Y = 0;
            Y< VoxelDim.y;
            ++Y)
        {
            u16 InitialGridIndex = GridIndexFrom(VoxelDim, X, Y, Z);
            b32x ShouldCompute = true; // (XOdd ^ ((Y&1) ^ (Z&1)));
            if(ShouldCompute)
            {
                light_atlas_texel SpecTile = GetTileClamped(SpecAtlas, X, Y, Z);
                
                v3 VoxCenterP = {(f32)X + 0.5f, (f32)Y + 0.5f, (f32)Z + 0.5f};
                v3 BoxCenterP = Solution->VoxMinCornerHot + Hadamard(VoxCenterP, Solution->VoxCellDim);
                
#if 0
                if(IsInDebugVoxDim(X) &&
                   IsInDebugVoxDim(Y) &&
                   IsInDebugVoxDim(Z))
                {
                    v3 BoxR = 0.1f*V3(1, 1, 1);
                    PushDebugBox(Solution, BoxCenterP - BoxR, BoxCenterP + BoxR, V4(1.0f, 1.0f, 1.0f, 1.0f));
                }
#endif
                
#define TEST_SPHERE 0
#if TEST_SPHERE
                TestSphere(Solution, SpecAtlas, SpecTile, BoxCenterP);
#else
                FullCast(Work, SpecTile, BoxCenterP, EntropyIndex, InitialGridIndex);
#endif
                light_atlas_texel DiffuseTile = GetTileClamped(DiffuseAtlas, X, Y, Z);
                
                // 8
#if 1
                for(u32 Ty = 1;
                    Ty < (DiffuseAtlas->TileDim.y - 1);
                    ++Ty)
                {
                    light_atlas_texel DiffuseTexel = OffsetFromTexel(DiffuseAtlas, DiffuseTile, 1, Ty);
                    
                    v3_4x *DestC = (v3_4x *)DiffuseTexel.Value;
                    
                    // 2
                    for(u32 Tx4 = 0;
                        Tx4 < ((DiffuseAtlas->TileDim.x - 2) / 4);
                        ++Tx4)
                    {
                        v3_4x LightC = {};
                        diffuse_weight_map *Weights = &Solution->DiffuseWeightMap[Ty-1][Tx4];
                        
                        // 8
                        for(u32 Sy = 0;
                            Sy < (SpecAtlas->TileDim.y - 2);
                            ++Sy)
                        {
                            light_atlas_texel SpecTexel = OffsetFromTexel(SpecAtlas, SpecTile, 1, Sy+1);
                            
                            f32_4x S0 = LoadF32_4X((f32_4x *)SpecTexel.Value + 0);
                            f32_4x S1 = LoadF32_4X((f32_4x *)SpecTexel.Value + 1);
                            f32_4x S2 = LoadF32_4X((f32_4x *)SpecTexel.Value + 2);
                            
                            f32_4x S3 = LoadF32_4X((f32_4x *)SpecTexel.Value + 3);
                            f32_4x S4 = LoadF32_4X((f32_4x *)SpecTexel.Value + 4);
                            f32_4x S5 = LoadF32_4X((f32_4x *)SpecTexel.Value + 5);
                            
                            LightC.r += Weights->E[Sy][0] * Broadcast4x(S0, 0);
                            LightC.g += Weights->E[Sy][0] * Broadcast4x(S0, 1);
                            LightC.b += Weights->E[Sy][0] * Broadcast4x(S0, 2);
                            
                            LightC.r += Weights->E[Sy][1] * Broadcast4x(S0, 3);
                            LightC.g += Weights->E[Sy][1] * Broadcast4x(S1, 0);
                            LightC.b += Weights->E[Sy][1] * Broadcast4x(S1, 1);
                            
                            LightC.r += Weights->E[Sy][2] * Broadcast4x(S1, 2);
                            LightC.g += Weights->E[Sy][2] * Broadcast4x(S1, 3);
                            LightC.b += Weights->E[Sy][2] * Broadcast4x(S2, 0);
                            
                            LightC.r += Weights->E[Sy][3] * Broadcast4x(S2, 1);
                            LightC.g += Weights->E[Sy][3] * Broadcast4x(S2, 2);
                            LightC.b += Weights->E[Sy][3] * Broadcast4x(S2, 3);
                            
                            LightC.r += Weights->E[Sy][4] * Broadcast4x(S3, 0);
                            LightC.g += Weights->E[Sy][4] * Broadcast4x(S3, 1);
                            LightC.b += Weights->E[Sy][4] * Broadcast4x(S3, 2);
                            
                            LightC.r += Weights->E[Sy][5] * Broadcast4x(S3, 3);
                            LightC.g += Weights->E[Sy][5] * Broadcast4x(S4, 0);
                            LightC.b += Weights->E[Sy][5] * Broadcast4x(S4, 1);
                            
                            LightC.r += Weights->E[Sy][6] * Broadcast4x(S4, 2);
                            LightC.g += Weights->E[Sy][6] * Broadcast4x(S4, 3);
                            LightC.b += Weights->E[Sy][6] * Broadcast4x(S5, 0);
                            
                            LightC.r += Weights->E[Sy][7] * Broadcast4x(S5, 1);
                            LightC.g += Weights->E[Sy][7] * Broadcast4x(S5, 2);
                            LightC.b += Weights->E[Sy][7] * Broadcast4x(S5, 3);
                        }
                        
                        LightC = Transpose(LightC);
                        
                        _mm_storeu_ps((f32 *)&DestC[Tx4].x, LightC.x.P);
                        _mm_storeu_ps((f32 *)&DestC[Tx4].y, LightC.y.P);
                        _mm_storeu_ps((f32 *)&DestC[Tx4].z, LightC.z.P);
                    }
                }
#else
#endif
                
                FillLightAtlasBorder(DiffuseAtlas, X, Y, Z);
                FillLightAtlasBorder(SpecAtlas, X, Y, Z);
                
                // TODO(casey): Check this as a source of entropy
                EntropyIndex += 234987;
            }
        }
    }
}

internal void
SplitBox(lighting_solution *Solution, lighting_box *ParentBox,
         u16 SourceCount, u16 *Source, u16 *Dest,
         u32 DimIndex)
{
    b32x ActuallySplit = false;
    
    if(SourceCount > 4)
    {
        for(u32x Attempt = 0;
            Attempt < 3;
            ++Attempt)
        {
            //
            // NOTE(casey): One-plane case (k-d-tree-like)
            //
            v3 ClassDir = {};
            ClassDir.E[DimIndex] = 1.0f;
            f32 ClassDist = Inner(ClassDir, 0.5f*(ParentBox->BoxMin + ParentBox->BoxMax));
            
            v3 RefCA = {};
            v3 RefCB = {};
            u16 CountA = 0;
            u16 CountB = 0;
            rectangle3 BoundsA = InvertedInfinityRectangle3();
            rectangle3 BoundsB = InvertedInfinityRectangle3();
            for(u32 SourceIndex = 0;
                SourceIndex < SourceCount;
                ++SourceIndex)
            {
                u16 BoxRef = Source[SourceIndex];
                lighting_box *Box = Solution->Boxes + BoxRef;
                
                if(Inner(ClassDir, 0.5f*(Box->BoxMin + Box->BoxMax)) < ClassDist)
                {
                    Dest[CountA] = BoxRef;
                    BoundsA = Union(BoundsA, RectMinMax(Box->BoxMin, Box->BoxMax));
                    RefCA += Box->RefC;
                    ++CountA;
                }
                else
                {
                    Dest[(SourceCount - 1) - CountB] = BoxRef;
                    BoundsB = Union(BoundsB, RectMinMax(Box->BoxMin, Box->BoxMax));
                    RefCB += Box->RefC;
                    ++CountB;
                }
            }
            
            u32 NextDimIndex = (DimIndex + 1) % 3;
            
            if((CountA > 0) && (CountB > 0))
            {
                u16 ChildBoxAAt = AddBoxStorage(Solution);
                u16 ChildBoxBAt = AddBoxStorage(Solution);
                Assert((ChildBoxAAt + 1) == ChildBoxBAt);
                
                ParentBox->Pack.ChildCount = 2;
                ParentBox->Pack.FirstChildIndex = ChildBoxAAt;
                
                lighting_box *ChildBoxA = Solution->Boxes + ChildBoxAAt;
                ChildBoxA->BoxMin = BoundsA.Min;
                ChildBoxA->BoxMax = BoundsA.Max;
                ChildBoxA->RefC = (1.0f / (f32)CountA)*RefCA;
                SplitBox(Solution, ChildBoxA,
                         CountA, Dest, Source,
                         NextDimIndex);
                
                lighting_box *ChildBoxB = Solution->Boxes + ChildBoxBAt;
                ChildBoxB->BoxMin = BoundsB.Min;
                ChildBoxB->BoxMax = BoundsB.Max;
                ChildBoxB->RefC = (1.0f / (f32)CountB)*RefCB;
                SplitBox(Solution, ChildBoxB,
                         CountB, Dest + CountA, Source,
                         NextDimIndex);
                
                ActuallySplit = true;
                break;
            }
            
            DimIndex = NextDimIndex;
        }
    }
    
    if(ActuallySplit)
    {
        ParentBox->Pack.IsLight_IsLeafContainer &= ~LightBox_IsLeafContainer;
    }
    else
    {
        ParentBox->Pack.IsLight_IsLeafContainer |= LightBox_IsLeafContainer;
        Assert(SourceCount < U8Max);
        ParentBox->Pack.ChildCount = (u8)SourceCount;
        ParentBox->Pack.FirstChildIndex = AddBoxStorage(Solution, SourceCount);
        for(u32 SourceIndex = 0;
            SourceIndex < SourceCount;
            ++SourceIndex)
        {
            u16 BoxRef = Source[SourceIndex];
            Solution->Boxes[ParentBox->Pack.FirstChildIndex + SourceIndex] = Solution->Boxes[BoxRef];
        }
    }
}

internal u32
SplitKd(lighting_solution *Solution, 
        u16 SourceCount, u16 *Source, u16 *Dest,
        u32 DimIndex, rectangle3 TotalBounds)
{
    v3 TotalBoundsCenter = GetCenter(TotalBounds);
    f32 PlaneD = 0;
    u8 KdIndex = 3;
    if(SourceCount > 4)
    {
        u16 LowestMinCount = U16Max;
        for(u32x Attempt = 0;
            Attempt < 3;
            ++Attempt)
        {
            // TODO(casey): This is a pretty stupid Kd splitter, we should look
            // for better fast metrics for picking the split plane!
            
            //
            // NOTE(casey): One-plane case (k-d-tree-like)
            //
            f32 ClassDist = TotalBoundsCenter.E[DimIndex];
            
            u16 CountA = 0;
            u16 CountB = 0;
            for(u32 SourceIndex = 0;
                SourceIndex < SourceCount;
                ++SourceIndex)
            {
                u16 BoxRef = Source[SourceIndex];
                lighting_box *Box = Solution->Boxes + BoxRef;
                
                if(Box->BoxMax.E[DimIndex] >= ClassDist)
                {
                    ++CountA;
                }
                
                if(Box->BoxMin.E[DimIndex] < ClassDist)
                {
                    ++CountB;
                }
            }
            
            u16 MinCount = Maximum(CountA, CountB);
            if(LowestMinCount > MinCount)
            {
                LowestMinCount = MinCount;
                KdIndex = SafeTruncateToU8(DimIndex);
                PlaneD = ClassDist;
            }
            
            DimIndex = (DimIndex + 1) % 3;
        }
    }
    
    u16 NodeIndex = 0;
    u8 LeafCount = 0;
    
    if(KdIndex != 3)
    {
        Assert(KdIndex >= 0);
        Assert(KdIndex <= 2);
        
        if(Solution->KdNodeCount < U16Max)
        {
            u16 PositiveCount = 0;
            u16 NegativeCount = 0;
            rectangle3 PositiveBounds = InvertedInfinityRectangle3();
            rectangle3 NegativeBounds = InvertedInfinityRectangle3();
            for(u32 SourceIndex = 0;
                SourceIndex < SourceCount;
                ++SourceIndex)
            {
                u16 BoxRef = Source[SourceIndex];
                lighting_box *Box = Solution->Boxes + BoxRef;
                
                rectangle3 BoxBounds = RectMinMax(Box->BoxMin, Box->BoxMax);
                if(Box->BoxMax.E[KdIndex] >= PlaneD)
                {
                    Dest[PositiveCount] = BoxRef;
                    PositiveBounds = Union(PositiveBounds, BoxBounds);
                    ++PositiveCount;
                }
                
                if(Box->BoxMin.E[KdIndex] < PlaneD)
                {
                    Dest[(SourceCount - 1) - NegativeCount] = BoxRef;
                    NegativeBounds = Union(NegativeBounds, BoxBounds);
                    ++NegativeCount;
                }
            }
            
            if((PositiveCount < SourceCount) &&
               (NegativeCount < SourceCount))
            {
                NodeIndex = Solution->KdNodeCount++;
                kd_tree_node *Node = Solution->KdNodes + NodeIndex;
                Node->PlaneD = PlaneD;
                
                u32 NextDimIndex = (DimIndex + 1) % 3;
                
                // NOTE(casey): Positive side
                u32 Sides0 = SplitKd(Solution,
                                     PositiveCount, Dest, Source,
                                     NextDimIndex, PositiveBounds);
                
                // NOTE(casey): Negative side
                u32 Sides1 = SplitKd(Solution,
                                     NegativeCount, Dest + PositiveCount, Source,
                                     NextDimIndex, NegativeBounds);
                
                u32 KdIndexPack = (KdIndex << 28);
                u32 PositivePlaneTestPattern = (0x0 << 24);
                u32 NegativePlaneTestPattern = (0xf << 24);
                Node->Sides[0] = Sides0 | KdIndexPack | PositivePlaneTestPattern;
                Node->Sides[1] = Sides1 | KdIndexPack | NegativePlaneTestPattern;
            }
            else
            {
                KdIndex = 3;
            }
        }
    }
    
    if(KdIndex == 3)
    {
        NodeIndex = AddBoxStorage(Solution, SourceCount);
        LeafCount = SafeTruncateToU8(SourceCount);
        for(u32 SourceIndex = 0;
            SourceIndex < SourceCount;
            ++SourceIndex)
        {
            u16 BoxRef = Source[SourceIndex];
            Solution->Boxes[NodeIndex + SourceIndex] = Solution->Boxes[BoxRef];
        }
    }
    
    u32 PackValue = (NodeIndex | (LeafCount << 16) | (KdIndex << 30));
    
    return(PackValue);
}

internal void
CalculateRefC(lighting_solution *Solution, lighting_box *RootBox)
{
    v3 RefC = {};
    
    if(RootBox->Pack.ChildCount)
    {
        for(u32 SourceIndex = RootBox->Pack.FirstChildIndex;
            SourceIndex < (u32)(RootBox->Pack.FirstChildIndex + RootBox->Pack.ChildCount);
            ++SourceIndex)
        {
            lighting_box *Box = GetBox(Solution, SourceIndex);
            RefC += Box->RefC;
        }
        
        RefC *= (1.0f / (f32)RootBox->Pack.ChildCount);
    }
    
    RootBox->RefC = RefC;
}

internal u32
ComputeWalkTable(v3s VoxelDim, v3 CellDim, u32 MaxDestCount, u16 *Dest)
{
    u32 DestIndex = 0;
    
#if 0
    v3 StartingLocation = 0.5f*CellDim;
    for(u32 RayDirIndex = 0;
        RayDirIndex < RayDirCount;
        ++RayDirIndex)
    {
        u32 Span[3] = {};
        v3 RayD = ???;
        .WalkTableOffset = DestIndex;
        s32 Step[3];
        
        s16 DimStep = 1;
        for(u32 DimIndex = 0;
            DimIndex < 3;
            ++DimIndex)
        {
            if(RayD.E[DimIndex] >= 0)
            {
                Negate[DimIndex] = DimStep;
            }
            else
            {
                RayD.E[DimIndex] *= -1.0f;
                Negate[DimIndex] = -DimStep;
            }
            
            DimStep *= VoxelDim.E[DimIndex];
        }
        
        v3 At = StartingLocation;
        for(;;)
        {
            f32 tBest = F32Max;
            u32 BestDim = 0;
            
            for(u32 DimIndex = 0;
                DimIndex < 3;
                ++DimIndex)
            {
                f32 tThis = (CellDim.E[DimIndex] - At.E[DimIndex]) / RayD.E[DimIndex];
                if(tBest > tThis)
                {
                    tBest = tThis;
                    BestDim = DimIndex;
                }
            }
            
            At += tBest*RayD;
            Assert(DestIndex < MaxDestCount);
            Dest[DestIndex++] = Step[BestDim];
            
            ++Span[BestDim];
            if(Span[BestDim] > VoxelDim.E[BestDim])
            {
                break;
            }
        }
    }
#endif
}

internal void
BuildSpatialPartitionForLighting(lighting_solution *Solution)
{
    TIMED_FUNCTION();
    
    u16 ActualBoxCount = Solution->BoxCount;
    u16 ActualLightCount = 0;
    
    rectangle3 OccluderBounds = InvertedInfinityRectangle3();
    rectangle3 LightBounds = InvertedInfinityRectangle3();
    for(u16 BoxIndex = 0;
        BoxIndex < ActualBoxCount;
        ++BoxIndex)
    {
        lighting_box *Box = Solution->Boxes + BoxIndex;
        Solution->ScratchA[BoxIndex] = BoxIndex;
        OccluderBounds = Union(OccluderBounds, RectMinMax(Box->BoxMin, Box->BoxMax));
    }
    
#if LIGHTING_USE_OLD_KD
    Solution->RootBoxIndex = AddBoxStorage(Solution);
    lighting_box *RootBox = GetBox(Solution, Solution->RootBoxIndex);
    RootBox->BoxMin = OccluderBounds.Min;
    RootBox->BoxMax = OccluderBounds.Max;
    
    Solution->RootLightBoxIndex = AddBoxStorage(Solution);
    lighting_box *RootLightBox = GetBox(Solution, Solution->RootLightBoxIndex);
    RootLightBox->BoxMin = LightBounds.Min;
    RootLightBox->BoxMax = LightBounds.Max;
    
    SplitBox(Solution, RootBox, ActualBoxCount, Solution->ScratchA, Solution->ScratchB, 0);
    SplitBox(Solution, RootLightBox, ActualLightCount, Solution->ScratchC, Solution->ScratchB, 0);
    
    CalculateRefC(Solution, RootBox);
    CalculateRefC(Solution, RootLightBox);
#else
    Solution->RootKdValue = SplitKd(Solution, ActualBoxCount, Solution->ScratchA, Solution->ScratchB, 0, OccluderBounds);
#endif
    
    
    // TODO(casey): Grid build goes here.
#if 0
    u32 TotalLeafCount = 0;
    for(u16 BoxIndex = 0;
        BoxIndex < ActualBoxCount;
        ++BoxIndex)
    {
        lighting_box *Box = Solution->Boxes + BoxIndex;
        Solution->ScratchA[BoxIndex] = BoxIndex;
        v3u MinNodeIndex = GetVoxelIndexForP(Box->BoxMin);
        v3u MaxNodeIndex = GetVoxelIndexForP(Box->BoxMax);
        for(u32 Z = MinNodeIndex.z;
            Z <= MaxNodeIndex.z;
            ++Z)
        {
            for(u32 Y = MinNodeIndex.y;
                Y<= MaxNodeIndex.y;
                ++Y)
            {
                for(u32 X = MinNodeIndex.x;
                    X <= MaxNodeIndex.x;
                    ++X)
                {
                    lighting_node_blah_blah *Node = V3U(X, Y, Z);
                    Chain(Node, Box);
                    ++TotalLeafCount;
                }
            }
        }
    }
    
    u32 LeafIndex = 0;
    for(u32 NodeIndex = 0;
        NodeIndex < TotalNodeCount;
        ++NodeIndex)
    {
        lighting_node_blah_blah *Node = V3U(X, Y, Z);
        u16 Chain = Node->FirstInChain;
        while(Chain != ChainEnd)
        {
            lighting_box *Box = Solution->Boxes + Chain;
            
            v3 BoxMin, BoxMax, RefC;
            lighting_box_pack Pack;
            
            u32 PrimaryIndex = LeafIndex / 4;
            u32 LaneIndex = LeafIndex % 4;
            
            BoxMin[PrimaryIndex].x.E[LaneIndex] = Leaf.BoxMin.x;
            BoxMin[PrimaryIndex].y.E[LaneIndex] = Leaf.BoxMin.y;
            BoxMin[PrimaryIndex].z.E[LaneIndex] = Leaf.BoxMin.z;
            
            BoxMax[PrimaryIndex].x.E[LaneIndex] = Leaf.BoxMax.x;
            BoxMax[PrimaryIndex].y.E[LaneIndex] = Leaf.BoxMax.y;
            BoxMax[PrimaryIndex].z.E[LaneIndex] = Leaf.BoxMax.z;
            
            RefColor[PrimaryIndex].x.E[LaneIndex] = Leaf.RefC.x;
            RefColor[PrimaryIndex].y.E[LaneIndex] = Leaf.RefC.y;
            RefColor[PrimaryIndex].z.E[LaneIndex] = Leaf.RefC.z;
            
            Emission[PrimaryIndex].E[LaneIndex] = 
                (Leaf.Pack.IsLight_IsLeafContainer & LightBox_IsLight) ? 1.0f : 0.0f;
            
            Chain = Box->NextInChain;
        }
    }
    
    while(LeafIndex % 4)
    {
        // TODO(casey): What's the best "can't hit me" box for our actual
        // box/ray test
        BoxMin[PrimaryIndex].x.E[LaneIndex] = 10001.0f;
        BoxMin[PrimaryIndex].y.E[LaneIndex] = 10001.0f;
        BoxMin[PrimaryIndex].z.E[LaneIndex] = 10001.0f;
        
        BoxMax[PrimaryIndex].x.E[LaneIndex] = 10000.0f;
        BoxMax[PrimaryIndex].y.E[LaneIndex] = 10000.0f;
        BoxMax[PrimaryIndex].z.E[LaneIndex] = 10000.0f;
        
        RefColor[PrimaryIndex].x.E[LaneIndex] = 0.0f;
        RefColor[PrimaryIndex].y.E[LaneIndex] = 0.0f;
        RefColor[PrimaryIndex].z.E[LaneIndex] = 0.0f;
        
        Emission[PrimaryIndex].E[LaneIndex] = 0.0f;
        
        ++LeafIndex;
    }
#endif
}

internal u64
IrradiancePack(v3 LightI)
{
    LightI += V3(LIGHT_FLOOR_VALUE, LIGHT_FLOOR_VALUE, LIGHT_FLOOR_VALUE);
    if(LightI.r < 0) {LightI.r = 0.0f;}
    if(LightI.g < 0) {LightI.g = 0.0f;}
    if(LightI.b < 0) {LightI.b = 0.0f;}
    
    u64 Result = 0;
    f32 MaxV = Maximum(LightI.x, Maximum(LightI.y, LightI.z));
    if(MaxV > 0)
    {
        f32 InvMaxV = 1.0f / MaxV;
        f32 A = Clamp01(MaxV / (f32)MAX_LIGHT_INTENSITY);
        Result = BGRAPack4x16(65535.0f*V4(InvMaxV*LightI.x, InvMaxV*LightI.y, InvMaxV*LightI.z, A));
    }
    
    return(Result);
}

internal v3
BinormalToNormal(v3 N)
{
    v3 Result = 0.5f*N + V3(0.5f, 0.5f, 0.5f);
    return(Result);
}

internal void
BuildDiffuseLightMaps(lighting_solution *Solution, light_atlas *DiffuseAtlas, light_atlas *SpecAtlas)
{
    for(u32 Ty = 1;
        Ty < (DiffuseAtlas->TileDim.y - 1);
        ++Ty)
    {
        for(u32 Tx = 1;
            Tx < (DiffuseAtlas->TileDim.x - 1);
            ++Tx)
        {
            v3 Outgoing = DirectionFromTxTy(DiffuseAtlas->OxyCoefficient, Tx, Ty);
            
            for(u32 Sy = 0;
                Sy < (SpecAtlas->TileDim.y - 2);
                ++Sy)
            {
                for(u32 Sx = 0;
                    Sx < (SpecAtlas->TileDim.x - 2);
                    ++Sx)
                {
                    v3 Incoming = DirectionFromTxTy(SpecAtlas->OxyCoefficient, Sx + 1, Sy + 1);
                    f32 W = Clamp01(Inner(Outgoing, Incoming));
                    
                    // TODO(casey): Empirically, we need some removal of energy
                    // here, which seems like a bug.  We need to find out if
                    // it is or not!
                    // W *= 0.975f;
                    W *= 0.75f;
                    
                    diffuse_weight_map *Weights = &Solution->DiffuseWeightMap[Ty - 1][(Tx - 1)/4];
                    Weights->E[Sy][Sx].E[(Tx - 1) % 4] = W;
                }
            }
        }
    }
}

internal void
InitLighting(lighting_solution *Solution, memory_arena *Arena, f32 FundamentalUnit)
{
    Solution->Series = RandomSeed(1234);
    Solution->SamplingSpheres = LightSamplingSphereTable;
    // TODO(casey): We could probably just get rid of this if we want to,
    // actually - the per-frame arena could just push the lighting_work on at that time.
    Solution->Works = PushArray(Arena, MAX_LIGHT_LOOKUP_VOXEL_DIM, lighting_work, Align(64, false));
    Solution->VoxCellDim = FundamentalUnit*V3(1, 1, 1);
    Solution->InvVoxCellDim = 1.0f / Solution->VoxCellDim;
    
    Solution->UpdateDebugLines = true;
}

internal void
BeginLightingComputation(lighting_solution *Solution, render_group *Group, world *World,
                         world_position SimOriginP, world_position OriginP,
                         b32x LightingEnabled)
{
    Solution->LightingEnabled = LightingEnabled;
    if(Solution->LightingEnabled)
    {
        v3s VoxCameraOffset = RoundToV3S(Hadamard(Solution->InvVoxCellDim, OriginP.Offset_));
        VoxCameraOffset.y += 2;
        v3 CameraOffset = Hadamard(Solution->VoxCellDim, V3(VoxCameraOffset));
        OriginP.Offset_ = V3(0, 0, 0);
        
        v3s dVoxel = VoxCameraOffset - Solution->VoxCameraOffset;
        Solution->VoxCameraOffset = VoxCameraOffset;
        
        b32 CenterMoved = !AreInSameChunk(World, &Solution->LastOriginP, &OriginP);
        if(CenterMoved)
        {
            v3s VoxelCellsPerChunk =
            {
                RoundReal32ToInt32(World->ChunkDimInMeters.x / Solution->VoxCellDim.x),
                RoundReal32ToInt32(World->ChunkDimInMeters.y / Solution->VoxCellDim.y),
                RoundReal32ToInt32(World->ChunkDimInMeters.z / Solution->VoxCellDim.z),
            };
            
            v3s dChunk =
            {
                OriginP.ChunkX - Solution->LastOriginP.ChunkX,
                OriginP.ChunkY - Solution->LastOriginP.ChunkY,
                OriginP.ChunkZ - Solution->LastOriginP.ChunkZ,
            };
            
            dVoxel = dVoxel + Hadamard(VoxelCellsPerChunk, dChunk);
        }
        Solution->LastOriginP = OriginP;
        
        if(dVoxel.x || dVoxel.y || dVoxel.z)
        {
            game_render_commands *Commands = Group->Commands;
            BlockCopyAtlas(&Commands->DiffuseLightAtlas, dVoxel);
            BlockCopyAtlas(&Commands->SpecularLightAtlas, dVoxel);
        }
        
        v3s HotCellCount = Group->Commands->DiffuseLightAtlas.VoxelDim;
        
        v3 HotDim = Hadamard(V3(HotCellCount), Solution->VoxCellDim);
        v3 InvHotDim = 1.0f / HotDim;
        
        Solution->VoxMinCornerHot = Subtract(World, &OriginP, &SimOriginP) + CameraOffset - 0.5f*HotDim;
        rectangle3 HotVoxelRect = RectMinDim(Solution->VoxMinCornerHot, HotDim);
        
#if 0
        PushVolumeOutline(Group, HotVoxelRect, V4(0, 1, 1, 1));
#endif
        
        SetLightBounds(Group, HotVoxelRect.Min, InvHotDim);
        EnableLighting(Group, HotVoxelRect);
    }
}

internal u32
InternalLightingCore(lighting_solution *Solution, light_atlas *SpecAtlas, light_atlas *DiffuseAtlas, platform_work_queue *LightingQueue, u32 DebugRepeatCount = 1)
{
    // TODO(casey): Shouldn't I be able to not use debug/ here?  Maybe we need to
    // make the way this works be more intuitive?
    if(LightBoxDumpTrigger)
    {
        DEBUGDumpData("debug/source_lighting.dump", sizeof(*Solution), Solution);
        DEBUGDumpData("debug/source_lightboxes.dump", Solution->BoxCount*sizeof(*Solution->Boxes), Solution->Boxes);
        DEBUGDumpData("debug/source_specatlas.dump", GetLightAtlasSize(SpecAtlas), GetLightAtlasTexels(SpecAtlas));
        DEBUGDumpData("debug/source_diffuseatlas.dump", GetLightAtlasSize(DiffuseAtlas), GetLightAtlasTexels(DiffuseAtlas));
    }
    
    BuildSpatialPartitionForLighting(Solution);
    
    if(LightBoxDumpTrigger)
    {
        DEBUGDumpData("debug/result_lightboxes.dump", Solution->BoxCount*sizeof(*Solution->Boxes), Solution->Boxes);
    }
    
    u32 WorkCount = SpecAtlas->VoxelDim.x;
    while(DebugRepeatCount--)
    {
        for(u32 WorkIndex = 0;
            WorkIndex < WorkCount;
            ++WorkIndex)
        {
            lighting_work *Work = Solution->Works + WorkIndex;
            Assert(((memory_index)Work & 63) == 0);
            
            Work->Solution = Solution;
            
            Work->DiffuseLightAtlas = DiffuseAtlas;
            Work->SpecularLightAtlas = SpecAtlas;
            Work->VoxelX = WorkIndex;
            Work->SamplePointEntropy = RandomNextU32(&Solution->Series);
            Work->LightPointEntropy = RandomNextU32(&Solution->Series);
            
            Work->TotalCastsInitiated = 0;
            Work->TotalPartitionsTested = 0;
            Work->TotalPartitionLeavesUsed = 0;
            Work->TotalLeavesTested = 0;
            
            if(LightingQueue)
            {
                Platform.AddEntry(LightingQueue, ComputeLightPropagationWork, Work);
            }
            else
            {
                ComputeLightPropagationWork(LightingQueue, Work);
            }
        }
    }
    
    if(LightingQueue)
    {
        Platform.CompleteAllWork(LightingQueue);
    }
    
    if(LightBoxDumpTrigger)
    {
        DEBUGDumpData("debug/result_specatlas.dump", GetLightAtlasSize(SpecAtlas), GetLightAtlasTexels(SpecAtlas));
        DEBUGDumpData("debug/result_diffuseatlas.dump", GetLightAtlasSize(DiffuseAtlas), GetLightAtlasTexels(DiffuseAtlas));
        
        LightBoxDumpTrigger = false;
    }
    
    return(WorkCount);
}

internal void
EndLightingComputation(lighting_solution *Solution, render_group *Group, platform_work_queue *LightingQueue)
{
    TIMED_FUNCTION();
    
    if(Solution->LightingEnabled)
    {
        render_setup *Setup = &Group->LastSetup;
        game_render_commands *Commands = Group->Commands;
        light_atlas *DiffuseAtlas = &Commands->DiffuseLightAtlas;
        light_atlas *SpecAtlas = &Commands->SpecularLightAtlas;
        
        // TODO(casey): Figure out where to cache this
        BuildDiffuseLightMaps(Solution, DiffuseAtlas, SpecAtlas);
        
        if(Solution->UpdateDebugLines)
        {
            Solution->DebugLineCount = 0;
        }
        Solution->BoxCount = SafeTruncateToU16(Group->Commands->LightOccluderCount);
        u32 OriginalBoxCount = Solution->BoxCount;
        Solution->Boxes = Group->Commands->LightOccluders;
        // Solution->PointCount = Group->LightPointIndex;
        
        u32 EntropyFrameCount = 256;
        
        // NOTE(casey): From the linear blend coefficient tIrradiancePreservation
        // between the two successive frames of irradiance data, we "derive" two
        // coefficients, one for the multiplication at the end that renormalizes,
        // and one for the new ray contributions.  This is to avoid pre-multiplying
        // the existing probes in a first pass.
        Solution->tUpdateBlend = 8.0 / 60.0f;
        
        // NOTE(casey): When doing long-term accumulation for testing, we turn
        // off the linear blend and instead just set the coefficients for
        // long-term averaging.
        if(Solution->Accumulating)
        {
            if(Solution->AccumulationCount < LIGHT_TEST_ACCUMULATION_COUNT)
            {
                Solution->tUpdateBlend = 1.0f / LIGHT_TEST_ACCUMULATION_COUNT;
            }
            else
            {
                Solution->tUpdateBlend = 0.0f;
            }
        }
        
        u32 WorkCount = InternalLightingCore(Solution, SpecAtlas, DiffuseAtlas, LightingQueue);
        
        u32 TotalCastsInitiated = 0;
        u32 TotalPartitionsTested = 0;
        u32 TotalLeavesTested = 0;
        u32 TotalPartitionLeavesUsed = 0;
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
        
        f32 PartitionsPerCast = (f32)((f64)TotalPartitionsTested / (f64)TotalCastsInitiated);
        f32 LeavesPerCast = (f32)((f64)TotalLeavesTested / (f64)TotalCastsInitiated);
        f32 PartitionsPerLeaf = (f32)((f64)TotalPartitionsTested / (f64)TotalLeavesTested);
        
        u32 RaysPerProbe = 4*ArrayCount(Solution->SamplingSpheres[0].SampleDirection);
        
        {DEBUG_DATA_BLOCK("Lighting");
            DEBUG_UI_HUD(DevMode_lighting);
            DEBUG_B32(Global_Lighting_ShowProbes);
            
            DEBUG_VALUE(Solution->BoxCount);
            
            DEBUG_VALUE(TotalCastsInitiated);
            DEBUG_VALUE(TotalPartitionsTested);
            DEBUG_VALUE(TotalPartitionLeavesUsed);
            DEBUG_VALUE(TotalLeavesTested);
            DEBUG_VALUE(PartitionsPerCast);
            DEBUG_VALUE(LeavesPerCast);
            DEBUG_VALUE(PartitionsPerLeaf);
            
            DEBUG_VALUE(RaysPerProbe);
            
            DEBUG_B32(LightBoxDumpTrigger);
        }
        
        renderer_texture WhiteTexture = Group->WhiteTexture;
        
#if 0
        for(u32 SphereIndex = 0;
            SphereIndex < LIGHT_SAMPLING_SPHERE_COUNT;
            ++SphereIndex)
        {
            for(u32 BundleIndex = 0;
                BundleIndex < LIGHT_SAMPLING_RAY_BUNDLES_PER_SPHERE;
                ++BundleIndex)
            {
                for(u32 RayIndex = 0;
                    RayIndex < 4;
                    ++RayIndex)
                {
                    v3 P = GetComponent(Solution->SamplingSpheres[SphereIndex].SampleDirection[BundleIndex], RayIndex);
                    P.z += 3.0f;
                    v3 Radius = {0.01f, 0.01f, 0.01f};
                    PushCube(Group, WhiteTexture, P, Radius, GetDebugColor4(4*BundleIndex + RayIndex));
                }
            }
        }
#endif
        
        GetCurrentQuads(Group, Solution->DebugLineCount, WhiteTexture);
        for(u32 DebugLineIndex = 0;
            DebugLineIndex < Solution->DebugLineCount;
            ++DebugLineIndex)
        {
            debug_line *Line = Solution->DebugLines + DebugLineIndex;
            PushLineSegment(Group, WhiteTexture, Line->FromP, Line->Color, Line->ToP, Line->Color, 0.01f);
        }
        
        Solution->FrameOdd = (Solution->FrameOdd ^ 1);
    }
}
