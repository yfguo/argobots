/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#include "abti.h"
#include <unistd.h>

#define ABTD_THREAD_DEFAULT_STACKSIZE   16384
#define ABTD_SCHED_DEFAULT_STACKSIZE    (4*1024*1024)
#define ABTD_SCHED_EVENT_FREQ           50


#ifdef HAVE_PTHREAD_SETAFFINITY_NP
static cpu_set_t g_cpusets[CPU_SETSIZE];

static void ABTD_env_get_affinity(void)
{
    cpu_set_t cpuset;
    int i, ret;
    int num_cores = 0;

    ret = sched_getaffinity(getpid(), sizeof(cpu_set_t), &cpuset);
    ABTI_ASSERT(ret == 0);

    for (i = 0; i < CPU_SETSIZE; i++) {
        CPU_ZERO(&g_cpusets[i]);
        if (CPU_ISSET(i, &cpuset)) {
            CPU_SET(i, &g_cpusets[num_cores]);
            num_cores++;
        }
    }
    gp_ABTI_global->num_cores = num_cores;
}

cpu_set_t ABTD_env_get_cpuset(int rank)
{
    return g_cpusets[rank % gp_ABTI_global->num_cores];
}
#else
#define ABTD_env_get_affinity()
#endif


void ABTD_env_init(ABTI_global *p_global)
{
    char *env;

    /* Get the number of available cores in the system */
    p_global->num_cores = sysconf(_SC_NPROCESSORS_ONLN);

    /* By default, we use the CPU affinity */
    p_global->set_affinity = ABT_TRUE;
    env = getenv("ABT_ENV_SET_AFFINITY");
    if (env != NULL) {
        if (strcmp(env, "0") == 0 || strcmp(env, "NO") == 0 ||
            strcmp(env, "no") == 0 || strcmp(env, "No") == 0) {
            p_global->set_affinity = ABT_FALSE;
        }
    }
    if (p_global->set_affinity == ABT_TRUE) {
        ABTD_env_get_affinity();
    }

#ifdef ABT_CONFIG_USE_DEBUG_LOG
    /* If the debug logging is set in configure, logging is turned on by
     * default. */
    p_global->use_logging = ABT_TRUE;
    p_global->use_debug = ABT_TRUE;
#else
    /* Otherwise, logging is not turned on by default. */
    p_global->use_logging = ABT_FALSE;
    p_global->use_debug = ABT_FALSE;
#endif
    env = getenv("ABT_ENV_USE_LOG");
    if (env != NULL) {
        if (strcmp(env, "0") == 0 || strcmp(env, "NO") == 0 ||
            strcmp(env, "no") == 0 || strcmp(env, "No") == 0) {
            p_global->use_logging = ABT_FALSE;
        } else {
            p_global->use_logging = ABT_TRUE;
        }
    }
    env = getenv("ABT_ENV_USE_DEBUG");
    if (env != NULL) {
        if (strcmp(env, "0") == 0 || strcmp(env, "NO") == 0 ||
            strcmp(env, "no") == 0 || strcmp(env, "No") == 0) {
            p_global->use_debug = ABT_FALSE;
        } else {
            p_global->use_debug = ABT_TRUE;
        }
    }

    /* Maximum size of the internal ES array */
    env = getenv("ABT_ENV_MAX_NUM_XSTREAMS");
    if (env != NULL) {
        p_global->max_xstreams = atoi(env);
    } else {
        p_global->max_xstreams = p_global->num_cores;
    }

    /* Default stack size for ULT */
    env = getenv("ABT_ENV_THREAD_STACKSIZE");
    if (env != NULL) {
        p_global->thread_stacksize = (size_t)atol(env);
        ABTI_ASSERT(p_global->thread_stacksize >= 512);
    } else {
        p_global->thread_stacksize = ABTD_THREAD_DEFAULT_STACKSIZE;
    }

    /* Default stack size for scheduler */
    env = getenv("ABT_ENV_SCHED_STACKSIZE");
    if (env != NULL) {
        p_global->sched_stacksize = (size_t)atol(env);
        ABTI_ASSERT(p_global->sched_stacksize >= 512);
    } else {
        p_global->sched_stacksize = ABTD_SCHED_DEFAULT_STACKSIZE;
    }

    /* Default frequency for event checking by the scheduler */
    env = getenv("ABT_ENV_SCHED_EVENT_FREQ");
    if (env != NULL) {
        p_global->sched_event_freq = (uint32_t)atol(env);
        ABTI_ASSERT(p_global->sched_event_freq >= 1);
    } else {
        p_global->sched_event_freq = ABTD_SCHED_EVENT_FREQ;
    }

    /* Init timer */
    ABTD_time_init();
}

