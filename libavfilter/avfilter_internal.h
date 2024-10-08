/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * APIs internal to the generic filter(graph) layer.
 *
 * MUST NOT be included by individual filters.
 */

#ifndef AVFILTER_AVFILTER_INTERNAL_H
#define AVFILTER_AVFILTER_INTERNAL_H

#include <stdint.h>

#include "avfilter.h"
#include "filters.h"
#include "framequeue.h"

typedef struct FilterLinkInternal {
    FilterLink l;

    struct FFFramePool *frame_pool;

    /**
     * Queue of frames waiting to be filtered.
     */
    FFFrameQueue fifo;

    /**
     * If set, the source filter can not generate a frame as is.
     * The goal is to avoid repeatedly calling the request_frame() method on
     * the same link.
     */
    int frame_blocked_in;

    /**
     * Link input status.
     * If not zero, all attempts of filter_frame will fail with the
     * corresponding code.
     */
    int status_in;

    /**
     * Timestamp of the input status change.
     */
    int64_t status_in_pts;

    /**
     * Link output status.
     * If not zero, all attempts of request_frame will fail with the
     * corresponding code.
     */
    int status_out;

    /**
     * True if a frame is currently wanted on the output of this filter.
     * Set when ff_request_frame() is called by the output,
     * cleared when a frame is filtered.
     */
    int frame_wanted_out;

    /**
     * Index in the age array.
     */
    int age_index;

    /** stage of the initialization of the link properties (dimensions, etc) */
    enum {
        AVLINK_UNINIT = 0,      ///< not started
        AVLINK_STARTINIT,       ///< started, but incomplete
        AVLINK_INIT             ///< complete
    } init_state;
} FilterLinkInternal;

static inline FilterLinkInternal *ff_link_internal(AVFilterLink *link)
{
    return (FilterLinkInternal*)link;
}

typedef struct AVFilterCommand {
    double time;                ///< time expressed in seconds
    char *command;              ///< command
    char *arg;                  ///< optional argument for the command
    int flags;
    struct AVFilterCommand *next;
} AVFilterCommand;

typedef struct FFFilterGraph {
    /**
     * The public AVFilterGraph. See avfilter.h for it.
     */
    AVFilterGraph p;

    struct FilterLinkInternal **sink_links;
    int sink_links_count;

    unsigned disable_auto_convert;

    void *thread;
    avfilter_execute_func *thread_execute;
    FFFrameQueueGlobal frame_queues;
} FFFilterGraph;

static inline FFFilterGraph *fffiltergraph(AVFilterGraph *graph)
{
    return (FFFilterGraph*)graph;
}

/**
 * Update the position of a link in the age heap.
 */
void ff_avfilter_graph_update_heap(AVFilterGraph *graph,
                                   struct FilterLinkInternal *li);

/**
 * Allocate a new filter context and return it.
 *
 * @param filter what filter to create an instance of
 * @param inst_name name to give to the new filter context
 *
 * @return newly created filter context or NULL on failure
 */
AVFilterContext *ff_filter_alloc(const AVFilter *filter, const char *inst_name);

/**
 * Remove a filter from a graph;
 */
void ff_filter_graph_remove_filter(AVFilterGraph *graph, AVFilterContext *filter);

int ff_filter_activate(AVFilterContext *filter);

/**
 * Parse filter options into a dictionary.
 *
 * @param logctx context for logging
 * @param priv_class a filter's private class for shorthand options or NULL
 * @param options dictionary to store parsed options in
 * @param args options string to parse
 *
 * @return a non-negative number on success, a negative error code on failure
 */
int ff_filter_opt_parse(void *logctx, const AVClass *priv_class,
                        AVDictionary **options, const char *args);

int ff_graph_thread_init(FFFilterGraph *graph);

void ff_graph_thread_free(FFFilterGraph *graph);

#endif /* AVFILTER_AVFILTER_INTERNAL_H */
