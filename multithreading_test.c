#include "common.c"
#include "multithreading.c"


void fib(Worker *worker, Job *job) {
    assert(job);
    assert(job->data);
    int n = *((int *)job->data);
    
    //fprintf(stderr, "job number %d with data  %d\n", job->id, n);
    
    if (n < 2) {
        job->result = n;
        fprintf(stderr, "fib result: %d\n", job->result);
        return; 
    }
    int n0 = n - 1;
    int n1 = n - 2;
    
    Job_group *job_group = create_job_group();
    Job *job0 = create_child_job(job_group, job, fib, &n0);
    Job *job1 = create_child_job(job_group, job, fib, &n1);
    
    run_job(worker, job0);
    run_job(worker, job1);
    wait_on_job_group(worker, job_group);
    
    job->result = job0->result + job1->result;
    fprintf(stderr, "fib result: %d\n", job->result);
}

void test_multithreading_with_fib() {
#if 0
    time_t t;
    srand(time(&t));
    
    pthread_t *threads;
    int num_threads;
    Worker_group worker_group = create_job_group(&threads, &num_threads);
    
    Worker *worker = &worker_group.workers[0];
    Job_group *job_group = create_job_group();
    int n  = 6;
    int job_result;
    Job *job = create_root_job(job_group, fib, &n);
    run_job(worker, job);
    wait_on_job_group(worker, job_group);
    printf("result: %d.\n", job->result);
    
    destroy_worker_group(&worker_group);
    
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }
#endif
}

void add_func(int index, void *array, void *data) {
    ((int *)array)[index] += 5;
    printf("%d\n", ((int *)array)[index]);
}

#define ARR_SIZE 16

void test_parallel_for() {
    Worker_group worker_group;
    Worker *main_worker;
    bool run_single_threaded = true;
    Allocator *allocator = create_allocator(0.5, 0.5);
    init_multithreading(allocator, run_single_threaded, &worker_group, &main_worker);
    int array[100] = {0};
    parallel_for(main_worker, 0, 100, 1, array, 100, NULL, add_func);
    deinit_multithreading(&worker_group);
    
#if 0
    time_t t;
    srand(time(&t));
    
    pthread_t *threads;
    int num_threads;
    Worker_group worker_group = create_job_group(&threads, &num_threads);
    
    Worker *worker = &worker_group.workers[0];
    const int grain_size = 4;
    int arr[ARR_SIZE] = {0};
    parallel_for(worker, 0, ARR_SIZE, grain_size, arr, ARR_SIZE, NULL, add_func);
    for (int i = 0; i < ARR_SIZE; ++i) {
        printf("%d, ", arr[i]);
    }
    printf("\n");
    destroy_worker_group(&worker_group);
#endif
}

int main() {
    //test_multithreading_with_fib();
    test_parallel_for();
}

