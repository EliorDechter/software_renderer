/* ========================================================================
   $File: C:\work\handmade\code\hhsphere.cpp $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright by Molly Rocket, Inc., All Rights Reserved. $
   ======================================================================== */

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

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

internal void
GeneratePoissonSamples(random_series *Series, u32 DestCount, v3 *Dest)
{
    // TODO(casey): We could put a much more efficient poisson noise
    // generator in here, and probably should at some point...
    
    // TODO(casey): Right now we just hard code MinDistSq, but that
    // wouldn't work if you wanted to adjust the DestCount to something
    // else, so we probably need something that determines what the
    // correct "packing ratio" is...
    
    f32 MinDistSq = Square(0.17f);
    u32 PointCount = 0;
    while(PointCount < DestCount)
    {
        v3 P = V3(RandomBilateral(Series),
                  RandomBilateral(Series),
                  RandomBilateral(Series));
        f32 PSq = LengthSq(P);
        if(PSq > 0.01)
        {
            P *= (1.0f / SquareRoot(PSq));
            
            b32 Valid = true;
            for(u32 TestIndex = 0;
                TestIndex < PointCount;
                ++TestIndex)
            {
                if(LengthSq(Dest[TestIndex] - P) < MinDistSq)
                {
                    Valid = false;
                    break;
                }
            }
            
            if(Valid)
            {
                Dest[PointCount++] = P;
            }
        }
    }
}

internal f32
TestFunc(v3 Dir)
{
    f32 Result = 3.0f*Dir.x + 2.0f*Dir.y - 0.5f*Dir.z*Dir.z;
    return(Result);
}

struct cube_store
{
    f32 E[6];
};
struct sphere_store
{
    v3_4x *SampleDirection;
};

internal void
InterleaveDirections(u32 LightSamplingSphereCount, u32 RayBundlesPerSphere,
                     v3 *Directions, sphere_store *Spheres)
{
    v3 *DirFrom = Directions;
    for(u32 SphereIndex = 0;
        SphereIndex < LightSamplingSphereCount;
        ++SphereIndex)
    {
        sphere_store *Sphere = Spheres + SphereIndex;
        for(u32 BundleIndex = 0;
            BundleIndex < RayBundlesPerSphere;
            ++BundleIndex)
        {
            Sphere->SampleDirection[BundleIndex] = V3_4x(DirFrom[0], DirFrom[1], DirFrom[2], DirFrom[3]);
            DirFrom += 4;
        }
    }
}

internal v3 *
GeneratePoissonDistribution(u32 TotalDirectionCount)
{
    random_series Series = {1234};
    
    v3 *Directions = (v3 *)malloc(TotalDirectionCount*sizeof(v3));
    v3 *Displacements = (v3 *)malloc(TotalDirectionCount*sizeof(v3));
    b32 *Used = (b32 *)malloc(TotalDirectionCount*sizeof(b32));
    
    // NOTE(casey): Place points onto the sphere poorly (white noise cube projected to sphere surface)
    for(u32 DirIndex = 0;
        DirIndex < TotalDirectionCount;
        )
    {
        v3 Dir =
        {
            RandomBilateral(&Series),
            RandomBilateral(&Series),
            RandomBilateral(&Series),
        };
        
        Dir = NOZ(Dir);
        if(LengthSq(Dir) > 0.1f)
        {
            Used[DirIndex] = false;
            Directions[DirIndex] = Dir;
            ++DirIndex;
        }
    }
    
    f32 LastMinMaxSeparation = F32Max;
    for(;;)
    {
        f32 MinClosestPointDistance = F32Max;
        f32 MaxClosestPointDistance = F32Min;
        
        for(u32 DirIndex = 0;
            DirIndex < TotalDirectionCount;
            ++DirIndex)
        {
            v3 Disp = {};
            v3 Dir = Directions[DirIndex];
            f32 ClosestPointDistance = F32Max;
            
            // TODO(casey): You could make this only do half the
            // iterations by only considering pairs at a time, but I'm not going to bother.
            for(u32 RepIndex = 0;
                RepIndex < TotalDirectionCount;
                ++RepIndex)
            {
                if(DirIndex != RepIndex)
                {
                    v3 Rep = Directions[RepIndex];
                    
                    f32 Falloff = Inner(Dir, Rep);
                    if(Falloff > 0)
                    {
                        v3 ForceLine = (Dir - Rep);
                        f32 ForceLineLength = Length(ForceLine);
                        
                        ForceLine = Cross(ForceLine, Dir);
                        ForceLine = Cross(Dir, ForceLine);
                        
                        ForceLine = NOZ(ForceLine);
                        Disp += Falloff*ForceLine;
                        
                        if(ClosestPointDistance > ForceLineLength)
                        {
                            ClosestPointDistance = ForceLineLength;
                        }
                    }
                }
            }
            
            Displacements[DirIndex] = Disp;
            MinClosestPointDistance = Minimum(MinClosestPointDistance, ClosestPointDistance);
            MaxClosestPointDistance = Maximum(MaxClosestPointDistance, ClosestPointDistance);
        }
        
        // TODO(casey): Determine what the real convergence criteria is.
        f32 MinMaxSeparation = MaxClosestPointDistance - MinClosestPointDistance;
        if(MinMaxSeparation < 0.025f)
        {
            break;
        }
        
        printf("\rGenerating directions... (%f - %f = %f +%f)            ",
               MaxClosestPointDistance, MinClosestPointDistance, MinMaxSeparation,
               LastMinMaxSeparation - MinMaxSeparation);
        LastMinMaxSeparation = MinMaxSeparation;
        
        f32 MaxDispPerStep = 0.01f*MaxClosestPointDistance;
        for(u32 DirIndex = 0;
            DirIndex < TotalDirectionCount;
            ++DirIndex)
        {
            v3 Displacement = Displacements[DirIndex];
            v3 *Direction = Directions + DirIndex;
            
            v3 Dir = *Direction;
            
            Dir += MaxDispPerStep*Displacement;
            Dir = NOZ(Dir);
            
            *Direction = Dir;
        }
    }
    
    printf("\n");
    
    return(Directions);
}

internal void
GeneratePoissonLightingPattern(u32 LightSamplingSphereCount,
                               u32 RayBundlesPerSphere,
                               sphere_store *Spheres)
{
    u32 TotalDirectionCount = (LightSamplingSphereCount * RayBundlesPerSphere * 4);
    v3 *Directions = GeneratePoissonDistribution(TotalDirectionCount);
    InterleaveDirections(LightSamplingSphereCount, RayBundlesPerSphere, Directions, Spheres);
}

internal void
GenerateOctahedralLightingPattern(u32 LightSamplingSphereCount,
                                  u32 RayBundlesPerSphere,
                                  sphere_store *Spheres)
{
    light_atlas Atlas = MakeLightAtlas(V3S(1, 1, 1), V2U(10, 10));
    
    u32 TotalDirectionCount = (LightSamplingSphereCount * RayBundlesPerSphere * 4);
    v3 *InputDirections = GeneratePoissonDistribution(TotalDirectionCount);
    v3 *OutputDirections = (v3 *)malloc(TotalDirectionCount*sizeof(v3));
    
    u32 Dx = 8;
    u32 Dy = 8;
    
    v3 *OutDir = OutputDirections;
    for(u32 SphereIndex = 0;
        SphereIndex < LightSamplingSphereCount;
        ++SphereIndex)
    {
        sphere_store *Sphere = Spheres + SphereIndex;
        
        u32 Tx = 0;
        u32 Ty = 0;
        for(u32 BundleIndex = 0;
            BundleIndex < (4*RayBundlesPerSphere);
            ++BundleIndex)
        {
            f32 Used = 10.0f;
            u32 Mx = Tx + 1;
            u32 My = Ty + 1;
            
            // TODO(casey): Perturb this
            v3 Pick = DirectionFromTxTy(Atlas.OxyCoefficient, Mx, My);
            for(u32 DirIndex = 0;
                DirIndex < TotalDirectionCount;
                ++DirIndex)
            {
                v3 *Check = InputDirections + DirIndex;
                if(Check->x != Used)
                {
                    v2u Oxy = GetOctahedralOffset(Atlas.OctDimCoefficient, *Check);
                    if((Oxy.x == Mx) &&
                       (Oxy.y == My))
                    {
                        Pick = *Check;
                        Check->x = Used;
                        break;
                    }
                }
            }
            
            *OutDir++ = Pick;
            
            ++Tx;
            if(Tx >= Dx)
            {
                Tx = 0;
                ++Ty;
                if(Ty >= Dy)
                {
                    Ty = 0;
                }
            }
        }
    }
    
    InterleaveDirections(LightSamplingSphereCount, RayBundlesPerSphere, OutputDirections, Spheres);
}

internal void
OutputSphereINL(u32 LightSamplingSphereCount,
                u32 RayBundlesPerSphere,
                sphere_store *Spheres,
                FILE *File)
{
    fprintf(File, "#define LIGHT_SAMPLING_SPHERE_COUNT %u\n", LightSamplingSphereCount);
    fprintf(File, "#define LIGHT_SAMPLING_SPHERE_MASK %u\n", LightSamplingSphereCount - 1);
    fprintf(File, "#define LIGHT_SAMPLING_RAY_BUNDLES_PER_SPHERE %u\n", RayBundlesPerSphere);
    fprintf(File, "#define LIGHT_SAMPLING_TOTAL_RAYS_PER_SPHERE (4*LIGHT_SAMPLING_RAY_BUNDLES_PER_SPHERE)\n");
    fprintf(File, "\n");
    fprintf(File, "struct light_sampling_sphere\n");
    fprintf(File, "{\n");
    fprintf(File, "    v3_4x SampleDirection[LIGHT_SAMPLING_RAY_BUNDLES_PER_SPHERE];\n");
    u32 Alignment = ((4*RayBundlesPerSphere*6) & 3);
    if(Alignment != 0)
    {
        fprintf(File, "    f32 Padding[%u];\n", 4 - Alignment);
    }
    fprintf(File, "};\n");
    fprintf(File, "\n");
    
    fprintf(File, "alignas(16) global f32 LightSamplingSphereFloatTable[] =\n");
    fprintf(File, "{\n");
    for(u32 SphereIndex = 0;
        SphereIndex < LightSamplingSphereCount;
        ++SphereIndex)
    {
        sphere_store *Sphere = Spheres + SphereIndex;
        
        for(u32 RayBundleIndex = 0;
            RayBundleIndex < RayBundlesPerSphere;
            ++RayBundleIndex)
        {
            v3_4x Bundle = Sphere->SampleDirection[RayBundleIndex];
            fprintf(File, "    %ff, %ff, %ff, %ff,  %ff, %ff, %ff, %ff,  %ff, %ff, %ff, %ff,\n",
                    Bundle.E[0].E[0], Bundle.E[0].E[1], Bundle.E[0].E[2], Bundle.E[0].E[3],
                    Bundle.E[1].E[0], Bundle.E[1].E[1], Bundle.E[1].E[2], Bundle.E[1].E[3],
                    Bundle.E[2].E[0], Bundle.E[2].E[1], Bundle.E[2].E[2], Bundle.E[2].E[3]);
        }
    }
    fprintf(File, "};\n");
    fprintf(File, "alignas(16) global light_sampling_sphere *LightSamplingSphereTable = (light_sampling_sphere *)LightSamplingSphereFloatTable;\n");
};

int
main(int ArgCount, char **Args)
{
    if(ArgCount == 4)
    {
        u32 LightSamplingSphereCount = atoi(Args[1]);
        u32 RayBundlesPerSphere = atoi(Args[2]);
        char *DestFileName = Args[3];
        
        sphere_store *Spheres = (sphere_store *)malloc(LightSamplingSphereCount*sizeof(sphere_store));
        for(u32 SphereIndex = 0;
            SphereIndex < LightSamplingSphereCount;
            ++SphereIndex)
        {
            sphere_store *Sphere = Spheres + SphereIndex;
            Sphere->SampleDirection = (v3_4x *)malloc(RayBundlesPerSphere*sizeof(v3_4x));
        }
        GenerateOctahedralLightingPattern(LightSamplingSphereCount, RayBundlesPerSphere, Spheres);
        
        FILE *File = fopen(DestFileName, "w");
        if(File)
        {
            OutputSphereINL(LightSamplingSphereCount, RayBundlesPerSphere, Spheres, File);
        }
        else
        {
            fprintf(stderr, "Unable to open INL %s for writing.\n", DestFileName);
        }
    }
    else
    {
        fprintf(stderr, "Usage: %s <sphere count> <ray bundle count> <destination .inl>\n",
                Args[0]);
    }
}
