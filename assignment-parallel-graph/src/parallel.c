// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "log/log.h"
#include "utils.h"

#define NUM_THREADS 4

static int sum;
static os_graph_t *graph;
static os_threadpool_t *tp;
static pthread_mutex_t graph_mutex = PTHREAD_MUTEX_INITIALIZER;

static void process_node(unsigned int idx);

static void process_node_task(void *arg)
{
    unsigned int idx = *(unsigned int *)arg;
    process_node(idx);
}

static void process_node(unsigned int idx)
{
    os_node_t *node = graph->nodes[idx];

    pthread_mutex_lock(&graph_mutex);
    sum += node->info;
    graph->visited[idx] = DONE;
    pthread_mutex_unlock(&graph_mutex);

    for (unsigned int i = 0; i < node->num_neighbours; i++) {
        pthread_mutex_lock(&graph_mutex);
        if (graph->visited[node->neighbours[i]] == NOT_VISITED) {
            os_task_t *neighbor_task = create_task(process_node_task, &(node->neighbours[i]), NULL);
            enqueue_task(tp, neighbor_task);
            printf("%d\n", node->num_neighbours);
        }
        pthread_mutex_unlock(&graph_mutex);
    }
}

int main(int argc, char *argv[])
{
    FILE *input_file;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s input_file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    input_file = fopen(argv[1], "r");
    DIE(input_file == NULL, "fopen");

    graph = create_graph_from_file(input_file);

    tp = create_threadpool(NUM_THREADS);

    process_node(0);
    wait_for_completion(tp);
    destroy_threadpool(tp);

    printf("%d\n", sum);

    return 0;
}
