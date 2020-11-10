

typedef enum dev_mode { 
    dev_mode_none,
    
    dev_mode_first_editor,
    dev_mode_camera = dev_mode_first_editor, //TODO: I dont think thats allowed in c
    dev_mode_editing_assets,
    dev_mode_editing_hha,
    dev_mode_last_editor = dev_mode_editing_hha,
    
    dev_mode_profiling,
    dev_mode_rendering,
    dev_mode_lighting,
    dev_mode_memory,
    
    dev_mode_dump,
    
    dev_mode_count
} dev_mode;

#define DLIST_INSERT(sentinel, element) \
(element)->next = (sentinel)->next; \
(element)->prev = (sentinel); \
(element)->next->prev = (element); \
(element)->prev->next = (element); 

#define DLIST_REMOVE(element) \
if((element)->next) { \
(element)->next->prev = (element)->prev; \
(element)->prev->next = (element)->next; \
(element)->next = (element)->prev = 0; \
} 

#define DLIST_INIT(sentinel) \
(sentinel)->next = (sentinel); \
(sentinel)->prev = (sentinel)

#define DLIST_IS_EMPTY(sentinel) \
((sentinel)->next == (sentinel))

#define FREELIST_ALLOCATE(result, free_list_pointer, allocation_code) \
(result) = (free_list_pointer); \
if (result) { free_list_pointer = (result)->next_free;} else {result = allocation_code;}

#define FREELIST_DEALLOCATE(pointer, free_list_pointer) \
if (pointer) {(pointer)->next_free = (free_list_pointer); (free_list_pointer) = (pointer); }

typedef struct task_with_memory {
    bool being_used;
    bool depends_on_game_mode;
    memory_arena arena;
    
    temporary_memory memory_flush;
} task_with_memory;


struct controlled hero{};
struct hero_bitmap_ids {};

typedef enum game_mode { 
    game_mode_none, 
    
    game_mode_title_screen,
    game_mode_cutscne,
    game_mode_world
} game_mode;

typedef struct game_state {
    memory_arena total_arena;
    
    memory_arena mode_arena;
    memory_arena audio_arena;
    
    memory_arena *frame_arena;
    tmeporary_memory frame_arena_temp;
    
    controlled_hero controlled_heroes[MAX_CONTROLLER_COUNT];
    
    task_with_memory tasks[4]; //NOTE: is this related o the amount of cores at hand
    
    game_assets *assets;
    
    platform_work_queue *high_priority_queue;
    platform_work_queue *low_priority_queue;
    
    audio_state audio_state;
    playing_sound *music;
    
    game_mode game_mode;
    union {
        game_mode_title_screen *title_screen;
        game_mode_cutscene *cutscene;
        game_mode_work *world_mode;
    };
    
    dev_mode dev_mode;
    dev_ui dev_ui;
    in_game_editor editor;
} game_state;

