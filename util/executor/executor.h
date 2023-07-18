#pragma once

typedef struct executor_t executor_t;

typedef struct executor_task_t executor_task_t;

typedef int (*executor_action_cb)(void *arg);

typedef void (*executor_cleanup_cb)(void *arg, int result);

executor_t *executor_create(const char *name, int num_threads);

void executor_destroy(executor_t *executor);

const executor_task_t *executor_execute(executor_t *executor, executor_action_cb action, executor_cleanup_cb finalize,
                                        void *arg);

/**
 *
 * @param executor
 * @param task Task to cancel. If null, all tasks will be cancelled
 */
int executor_cancel(executor_t *executor, const executor_task_t *task);

int executor_is_cancelled(const executor_t *executor, const executor_task_t *task);

int executor_active_tasks_count(const executor_t *executor);

int executor_is_destroyed(const executor_t *executor);

int executor_noop(void *arg);