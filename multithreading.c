#ifndef PR_MULTITHREADING
#define PR_MULTITHREADING

#define DELETE_CODE 0

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <x86intrin.h>
#include <pthread.h>
#include <time.h>

#define compare_and_swap_val(ptr, old_val, new_val) __sync_val_compare_and_swap(ptr, old_val, new_val)
#define compare_and_swap_bool(ptr, old_val, new_val) __sync_bool_compare_and_swap(ptr, old_val, new_val)
#define fetch_and_add(ptr, value) __sync_add_and_fetch(ptr, value)
#define compiler_barrier __sync_synchronize
#define fetch_and_sub(ptr, value) __sync_fetch_and_sub(ptr, value)
#define atomic_exchange(ptr, value) __sync_lock_test_and_set(ptr, value)

struct Job;
struct Worker;

typedef void (*Job_function)(struct Worker*, struct Job*);
//typedef  void (*Parallel_for_function)(int begin_index, int end_index, void *array, void *data);
typedef  void (*Parallel_for_function)(int index, void *array, void *data);

#define MAX_JOB_NUM 4096
#define JOBS_MASK 4095

typedef struct Job {
    Job_function function;
    struct Job *parent;
    volatile int unfinished_jobs;
    volatile int jobs_to_delete_count;
    struct Job *jobs_to_delete;
    //padding to avoid false sharing
    void *data;
    int result;
    u32 continuation_count;
    struct Job *continuations;
    int id;
} Job;

typedef struct Job_deque {
    volatile long top, bottom;
    Job *job_array[MAX_JOB_NUM];
} Job_deque;

typedef struct Worker {
    Job_deque *deque;
    Job_deque *all_deques;
    int num_workers;
    int id;
    bool is_active;
} Worker;

typedef struct Job_group {
    Job *jobs[MAX_JOB_NUM];
    int job_count;
} Job_group;

typedef struct Worker_group {
    int workers_count;
    Worker *workers;
    pthread_t *threads;
    u32 num_created_threads;
} Worker_group;

typedef struct Parallel_for_data {
    int grain_size;
    void *array;
    int array_count;
    Parallel_for_function function;
    int begin_index, end_index;
    void *data;
} Parallel_for_data;

int g_job_count = 0; //TODO: consider removing this

void add_continuation(Job *ancestor, Job continuation) {
    fetch_and_add(&ancestor->continuation_count, 1);
    u32 count = ancestor->continuation_count;
    ancestor->continuations[count - 1] = continuation;
}

Job *create_root_job(Job_group *job_group, Job_function function, void *data) {
    Job *job = calloc(1, sizeof(Job));
    
    job->function = function;
    job->parent = 0;
    job->unfinished_jobs = 1;
    job->data = data;
    job->jobs_to_delete_count = 0;
    job->continuations = malloc(sizeof(Job) * MAX_JOB_NUM);
    job->jobs_to_delete = malloc(sizeof(Job) * MAX_JOB_NUM);
    //job->id = g_job_count++;
    job->id = 0;
    
    assert(job_group->job_count + 1 < MAX_JOB_NUM);
    job_group->jobs[job_group->job_count++] = job;
    
    return job;
}


Job *create_child_job(Job_group *job_group, Job *parent, Job_function function, void *data) {
    fetch_and_add(&parent->unfinished_jobs, 1);
    
    Job *job = calloc(1, sizeof(Job));
    job->function = function;
    job->parent = parent;
    job->unfinished_jobs = 1;
    job->data = data;
    job->jobs_to_delete_count = 0;
    job->continuations = malloc(sizeof(Job) * MAX_JOB_NUM);
    job->jobs_to_delete = malloc(sizeof(Job) * MAX_JOB_NUM);
    //job->id = g_job_count++;
    
    assert(job_group->job_count + 1 < MAX_JOB_NUM);
    job_group->jobs[job_group->job_count++] = job;
    
    return job;
}

bool push_to_deque(Job_deque *deque, Job *job) {
    long bottom = deque->bottom;
    deque->job_array[bottom & JOBS_MASK] = job;
    compiler_barrier;
    deque->bottom++;
}

Job *steal_from_deque(Job_deque *deque) {
    long top  = deque->top;
    compiler_barrier;
    long bottom = deque->bottom;
    if (top < bottom) {
        Job *job = deque->job_array[top & JOBS_MASK];
        
        if (compare_and_swap_bool(&deque->top, top + 1, top)) {
            return job;
        }
        return 0;
    }
    
    return 0;
}

Job *pop_from_deque(Job_deque *deque) {
    long bottom = deque->bottom - 1;
    atomic_exchange(&deque->bottom, bottom);
    
    long top = deque->top;
    if (top <= bottom) {
        Job *job = deque->job_array[bottom & JOBS_MASK];
        if (top != bottom) 
            return job;
        if (!compare_and_swap_bool(&deque->top, top, top + 1)) 
            return 0;
        deque->bottom = top + 1;
        return job;
    }
    
    deque->bottom = top;
    return 0;
}

Worker create_worker(Job_deque *deques, int id, int num_workers) {
    Worker worker = {0};
    worker.deque = &deques[id];
    worker.all_deques = deques;
    worker.id = id;
    worker.num_workers = num_workers;
    worker.is_active = true;
    
    return worker;
}

void yield() {
    _mm_pause();
}

Job *get_job(Worker *worker) {
    Job_deque *deque = worker->deque;
    
    Job *job = pop_from_deque(deque);
    if (!job) {
        u32 random_index = rand() % worker->num_workers;
        Job_deque *stolen_deque = &worker->all_deques[random_index];
        if (stolen_deque == worker->deque) {
            yield();
            return 0;
        }
        
        Job *stolen_job = steal_from_deque(stolen_deque); 
        if (!stolen_job) {
            yield();
            return 0;
        }
        
        return stolen_job;
    }
    
    return job;
}

void finish_job(Job_deque *deque, Job *job) {
    //TODO: finish writing this
    fetch_and_sub(&job->unfinished_jobs, 1);
    int unfinished_jobs = job->unfinished_jobs;
    if (unfinished_jobs == 0) {
        if (job->parent) {
            finish_job(deque, job->parent);
        }
    }
    
    for (u32 i = 0; i < job->continuation_count; ++i) {
        push_to_deque(deque, &job->continuations[i]);
    }
}

int g_safety_count = 0;

void execute_job(Worker *worker, Job *job) {
#if 0
    printf("deque: ");
    for (int i = 0; i < worker->deque->bottom; ++i) {
        printf("%d ", worker->deque->job_array[i]->id);
    }
    printf("\n");
#endif
    //g_safety_count++;
    if (g_safety_count >= 100000) {
        fprintf(stderr, "failed");
        exit(1);
    }
    (job->function)(worker, job);
    finish_job(worker->deque, job);
}

void *worker_thread(void *data) {
    Worker *worker = (Worker *)data;
    
    while (worker->is_active) {
        Job *job = get_job(worker);
        if (job)
            execute_job(worker, job);
    }
    
    return 0;
}

void run_job(Worker *worker, Job *job) {
    Job_deque *deque = worker->deque;
    push_to_deque(deque, job);
}

bool has_job_completed(const Job *job) {
    if (job->unfinished_jobs == 0)
        return true;
    return false;
}

void wait_on_job(Worker *worker, Job *job) {
    assert(job);
    while (!has_job_completed(job)) {
        Job *next_job = get_job(worker);
        if (next_job) {
            execute_job(worker, next_job);
        }
    }
}

Job_deque create_job_deque() {
    Job_deque job_deque = {0};
    
    return job_deque;
}

Worker_group destroy_worker_group(Worker_group *worker_group) {
    for (int i = 0; i < worker_group->workers_count; ++i) {
        worker_group->workers[i].is_active = false;
    }
}

Job_group *create_job_group() {
    Job_group *job_group = calloc(1, sizeof(Job_group));
    return job_group;
}

void wait_on_job_group(Worker *worker, Job_group *job_group) {
    for (int i = 0; i < job_group->job_count; ++i) {
        wait_on_job(worker, job_group->jobs[i]);
    }
}

Parallel_for_data *create_parallel_for_data(int begin_index, int end_index, int grain_size, void *array, int array_count, void *data, Parallel_for_function function) {
    Parallel_for_data *parallel_for_job_data = malloc(sizeof(Parallel_for_data));
    parallel_for_job_data->begin_index = begin_index;
    parallel_for_job_data->end_index = end_index;
    parallel_for_job_data->grain_size = grain_size;
    parallel_for_job_data->array = array;
    parallel_for_job_data->array_count = array_count;
    parallel_for_job_data->function = function;
    parallel_for_job_data->data = data;
    
    return parallel_for_job_data;
}

void parallel_for_function(Worker *worker, Job *parent_job) {
    Parallel_for_data *parallel_for_data = (Parallel_for_data *)parent_job->data;
    Job_group *job_group = create_job_group();
    
    if (parallel_for_data->array_count <= parallel_for_data->grain_size) {
        for (int i = parallel_for_data->begin_index; i < parallel_for_data->end_index; ++i) {
            parallel_for_data->function(i, parallel_for_data->array, parallel_for_data->data);
        }
    }
    else {
        int array_count = parallel_for_data->array_count / 2;
        Parallel_for_data *first_half_parallel_for_data = create_parallel_for_data(parallel_for_data->begin_index, parallel_for_data->begin_index +  array_count, parallel_for_data->grain_size, parallel_for_data->array, array_count,  parallel_for_data->data, parallel_for_data->function);
        Parallel_for_data *second_half_parallel_for_data = create_parallel_for_data(parallel_for_data->begin_index +  array_count, parallel_for_data->end_index, parallel_for_data->grain_size, parallel_for_data->array, array_count,  parallel_for_data->data, parallel_for_data->function);
        Job *job0 = create_child_job(job_group, parent_job, parallel_for_function, first_half_parallel_for_data);
        Job *job1 = create_child_job(job_group, parent_job, parallel_for_function, second_half_parallel_for_data);
        
        run_job(worker, job0);
        run_job(worker, job1);
    }
    
    //wait_on_job_group(worker, job_group);
}

Job *parallel_for(Worker *worker, int begin_index, int end_index, int grain_size, void *array, int array_count, void *data, Parallel_for_function function) {
    Job_group *job_group = create_job_group();
    Parallel_for_data *parallel_for_data = create_parallel_for_data(begin_index, end_index, grain_size, array, array_count, data, function);
    Job *job = create_root_job(job_group, parallel_for_function, parallel_for_data);
    run_job(worker, job);
    wait_on_job_group(worker, job_group);
}

void init_multithreading(Allocator *allocator, bool run_single_threaded, Worker_group *worker_group, Worker **out_main_worker) {
    time_t t;
    srand(time(&t));
    
    u32 num_created_threads;
    if (run_single_threaded) 
        num_created_threads = 0;
    else
        num_created_threads = get_nprocs();
    u32 total_num_threads = num_created_threads + 1;
    
    pthread_t *threads = (pthread_t *)allocate_perm(allocator, sizeof(pthread_t) * num_created_threads);
    
    Job_deque *job_deques = (Job_deque *)allocate_perm(allocator, total_num_threads * sizeof(Job_deque));
    Worker *workers = (Worker *)allocate_perm(allocator, total_num_threads * sizeof(Worker));
    
    for (int i = 0; i < total_num_threads; ++i) {
        job_deques[i] = create_job_deque();
        workers[i] = create_worker(job_deques, i, total_num_threads);
    }
    
    for (int i = 0; i < num_created_threads; ++i) {
        int result_code = pthread_create(&threads[i], NULL, worker_thread, (void *)&workers[i + 1]);
        if (result_code != 0) {
            fprintf(stderr, "failed to create thread, error code %d", result_code);
            exit(1);
        }
    }
    
    worker_group->workers_count = total_num_threads;
    worker_group->workers = workers;
    worker_group->threads = threads;
    worker_group->num_created_threads = num_created_threads;
    
    *out_main_worker = &worker_group->workers[0];
}

void deinit_multithreading(Worker_group *worker_group) {
    for (int i = 0; i < worker_group->num_created_threads; ++i) {
        assert(worker_group->threads + i);
        pthread_join(worker_group->threads[i], NULL);
    }
}

#endif