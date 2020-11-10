/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#include "handmade_platform.h"
#include "handmade_config.h"
#include "handmade_intrinsics.h"
#include "handmade_math.h"
#include "handmade_shared.h"
#include "handmade_light_atlas.h"
#include "handmade_simd.h"
#include "handmade_memory.h"
#include "handmade_stream.h"
#include "handmade_random.h"
#include "handmade_file_formats.h"
#include "handmade_file_formats_v0.h"
#include "handmade_cutscene.h"
#include "handmade_tokenizer.h"

enum dev_mode
{
    DevMode_none,
    
    DevMode_first_editor,
    DevMode_camera = DevMode_first_editor,
    DevMode_editing_assets,
    DevMode_editing_hha,
    DevMode_last_editor = DevMode_editing_hha,
    
    DevMode_profiling,
    DevMode_rendering,
    DevMode_lighting,
    DevMode_memory,
    
    DevMode_dump,
    
    DevMode_count,
};

#define DLIST_INSERT(Sentinel, Element)         \
(Element)->Next = (Sentinel)->Next;         \
(Element)->Prev = (Sentinel);               \
(Element)->Next->Prev = (Element);          \
(Element)->Prev->Next = (Element);
#define DLIST_REMOVE(Element)         \
if((Element)->Next) \
{ \
    (Element)->Next->Prev = (Element)->Prev;         \
    (Element)->Prev->Next = (Element)->Next;         \
    (Element)->Next = (Element)->Prev = 0; \
}
#define DLIST_INSERT_AS_LAST(Sentinel, Element)         \
(Element)->Next = (Sentinel);               \
(Element)->Prev = (Sentinel)->Prev;         \
(Element)->Next->Prev = (Element);          \
(Element)->Prev->Next = (Element);

#define DLIST_INIT(Sentinel) \
(Sentinel)->Next = (Sentinel); \
(Sentinel)->Prev = (Sentinel)

#define DLIST_IS_EMPTY(Sentinel) \
((Sentinel)->Next == (Sentinel))

#define FREELIST_ALLOCATE(Result, FreeListPointer, AllocationCode)             \
(Result) = (FreeListPointer); \
if(Result) {FreeListPointer = (Result)->NextFree;} else {Result = AllocationCode;}
#define FREELIST_DEALLOCATE(Pointer, FreeListPointer) \
if(Pointer) {(Pointer)->NextFree = (FreeListPointer); (FreeListPointer) = (Pointer);}

#include "handmade_box.h"
#include "handmade_world.h"
#include "handmade_brain.h"
#include "handmade_renderer.h"
#include "handmade_renderer_geometry.h"
#include "handmade_camera.h"
#include "handmade_lighting.h"
#include "handmade_entity.h"
#include "handmade_sim_region.h"
#include "handmade_world_mode.h"
#include "handmade_gen_math.h"
#include "handmade_edit_grid.h"
#include "handmade_entity_gen.h"
#include "handmade_world_gen.h"
#include "handmade_room_gen.h"
#include "handmade_particles.h"
#include "handmade_dev_ui.h"
#include "handmade_in_game_editor.h"

struct task_with_memory
{
    b32 BeingUsed;
    b32 DependsOnGameMode;
    memory_arena Arena;
    
    temporary_memory MemoryFlush;
};

#include "handmade_image.h"
#include "handmade_riff.h"
#include "handmade_png.h"
#include "handmade_wav.h"
#include "handmade_import.h"
#include "handmade_asset.h"
#include "handmade_audio.h"

struct controlled_hero
{
    brain_id BrainID;
    v2 ddP;
};

struct hero_bitmap_ids
{
    bitmap_id Head;
    bitmap_id Cape;
    bitmap_id Torso;
};

enum game_mode
{
    GameMode_None,
    
    GameMode_TitleScreen,
    GameMode_CutScene,
    GameMode_World,
};

struct game_state
{
    memory_arena TotalArena;
    
    memory_arena ModeArena;
    memory_arena AudioArena; // TODO(casey): Move this into the audio system proper!
    
    memory_arena *FrameArena; // TODO(casey): Cleared once per frame
    temporary_memory FrameArenaTemp;
    
    controlled_hero ControlledHeroes[MAX_CONTROLLER_COUNT];
    
    task_with_memory Tasks[4];
    
    game_assets *Assets;
    
    platform_work_queue *HighPriorityQueue;
    platform_work_queue *LowPriorityQueue;
    
    audio_state AudioState;
    playing_sound *Music;
    
    game_mode GameMode;
    union
    {
        game_mode_title_screen *TitleScreen;
        game_mode_cutscene *CutScene;
        game_mode_world *WorldMode;
    };
    
    dev_mode DevMode;
    dev_ui DevUI;
    in_game_editor Editor;
};

// TODO(casey): I think we can now safely remove BeginTaskWithMemory - I don't think
// it's actually going to be necessary anymore since textures and audio both read
// directly into a known location with no temp store...
internal task_with_memory *BeginTaskWithMemory(game_state *GameState, b32 DependsOnGameMode);
internal void EndTaskWithMemory(task_with_memory *Task);
internal void SetGameMode(game_state *GameState, game_mode GameMode);

internal void DEBUGDumpData(char *FileName, umm DumpSize, void *DumpData);
