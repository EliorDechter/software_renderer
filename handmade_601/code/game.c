

task_with_memory *begin_task_with_memory(game_state *game_state, bool depends_on_game_mode) {
    task_with_emmory *found_task = 0;
    
    for (u32 task_index = 0; task_index < array_count(game_state->tasks); ++task_index) {
        task_with_memory *task = game_state->tasks + task_index;
        if (!task->being_used) {
            found_task = task;
            task->being_used = true;
            task->depends_on_game_mode = depends_on_game_mode;
            task->memory_flush = begin_temporary_memory(&task->arena);
            break;
        }
    }
    
    return found_task;
}

void end_task_with_memory(task_with_memory *task) {
    end_temporary_memory(task->memory_flush);
    
    complete_previous_writes_before_future_writes;
    task->being_used = false;
}


#if INTERNAL 
debug_table *global_debug_table;
game_memory *debug_global_memory;
#endif
platform_api platform;

GAME_UPDATE_AND_RENDER(game_update_and_render) {
    platform = memory->platform_api;
    
#if INTERNAL
    global_debug_table = memory->debug_table;
    debug_global_memory = memory;
    
    