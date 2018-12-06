#include "ailua.h"
#include "task.h"
#include "task_queue.h"
#include "task_pool.h"

struct _task_worker
{
    ErlNifTid tid;
    ErlNifThreadOpts* opts;

    task_queue_t* q;
    int alive;
    int id;
};
typedef struct _task_worker task_worker_t;

struct _task_pool
{
    int count;
    task_worker_t* workers;
};

static void *
lua_worker_thread_run(void* arg)
{
    task_worker_t* w = (task_worker_t*) arg;
    task_t* task;
    int continue_running = 1;

    w->alive = 1;
    while(continue_running) {
        task = task_queue_pop(w->q);
        if(NULL == task) {
            continue_running = 0;
        }else {
            task_run(task);
        }
    }

    w->alive = 0;
    return NULL;
}
int 
worker_alloc(task_worker_t* worker,int id)
{
    task_queue_t* q = NULL;
    ErlNifThreadOpts* opts = NULL;

    q = task_queue_alloc();
    if(NULL == q ) {
        goto error;
    }

    worker->q = q;
    worker->id = id;
    opts = enif_thread_opts_create("lua_worker_thread_opts");

    if(NULL == opts ) {
        goto error;
    }

    worker->opts = opts;
    if(enif_thread_create("lua_worker_thread",
                          &worker->tid,
                          lua_worker_thread_run,
                          worker,
                          worker->opts) != 0) {
        goto error;
    }

    return 0;

 error:
    if(NULL != opts) enif_thread_opts_destroy(opts);
    if(NULL != q) task_queue_free(q);
    return -1;
}

void 
woker_free(task_worker_t* w)
{
    task_queue_push(w->q, NULL);

    enif_thread_join(w->tid, NULL);
    enif_thread_opts_destroy(w->opts);

    task_queue_free(w->q);
}


task_pool_t*
pool_alloc(int max_wokers)
{
    int i;
    task_pool_t* pool = (task_pool_t*) enif_alloc(sizeof(struct _task_pool));
    if(NULL == pool) goto error;
    pool->workers = NULL;
    pool->count = 0;
    pool->workers = (task_worker_t*) enif_alloc(max_wokers * sizeof(struct _task_worker));
    if(NULL == pool->workers) return NULL;

    pool->count = max_wokers;
    for(i = 0; i < max_wokers; i++) {
        if(worker_alloc(&pool->workers[i], i) < 0 ) {
            goto error;
        }
    }

    return pool;

 error:
    if(NULL == pool) return NULL;
    while(i > 0) {
        --i;
        work_free(&pool->workers[i]);
    }
    enif_free(pool->workers);
    pool->workers = NULL;
    pool->count = 0;
    enif_free(pool);
    return NULL;
}
void
pool_free(task_pool_t* pool)
{
    int i;
    if(NULL == pool) return;
    if(NULL == pool->workers){
        enif_free(pool);
        return;
    }
    for(i = 0; i < pool->count; i++){
        work_free(&pool->workers[i]);
    }
    enif_free(pool->workers);
    pool->workers = NULL;
    pool->count = 0;
    enif_free(pool);
}

ERL_NIF_TERM
add_to_pool(ErlNifEnv* env, void* lua, void* task)
{
    task_pool_t* pool = (task_pool_t*) enif_priv_data(env);

    lua_State* L = ailua_lua((ai_lua_t*)lua);
    unsigned int idx = (unsigned int)L;
    int hash_idx = idx % pool->count;

    assert(hash_idx>=0 && hash_idx < pool->count);
    task_worker_t* w = &pool->workers[hash_idx];
    enif_keep_resource(lua);

    if(!task_queue_push(w->q, (task_t*)task)){
        enif_release_resource(lua);
        return make_error_tuple(env, "task_schedule_failed");
    }
    return atom_ok;
}