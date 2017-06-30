// Copyright (c) 2017, pexeer@gmail.com All rights reserved.
// Licensed under a BSD-style license that can be found in the LICENSE file.

#pragma once

#include <unistd.h>                 // size_t
#include <stdint.h>                 // uint64_t
#include "p/thread/pcontext.h"      // jump_pcontext
#include "p/base/object_pool.h"     // ObjectPool<>

namespace p {
namespace thread {

struct TaskHandle;

struct TaskStack {
public:
    constexpr static size_t kTaskStackSize = 32 * 4096;

    TaskStack(size_t size);

    TaskStack() : stack_size(0), pcontext(nullptr) {}

    static TaskStack* NewThis();

    static void FreeThis(TaskStack* ts);

    const size_t    stack_size;
    void*           pcontext;
    const char      stack_guard[8] = {'T', 'A', 'S', 'K', 'T', 'A', 'S', 'K'};
};

struct JumpContext {
    TaskHandle* from;
    TaskHandle* to;
};

struct TaskHandle {
    uint64_t tid;
    uint64_t flag;
    void* (*func)(void*);
    void* arg;
    TaskStack* task_stack = nullptr;
public:
    typedef base::ArenaObjectPool<TaskHandle>   TaskHandleFactory;
    // static method
    static TaskHandle* build() {
        uint64_t obj_id;
        TaskHandle* ret = TaskHandleFactory::get(&obj_id);
        ret->tid = (ret->tid & 0xFFFFFFFF) + 0x100000000 + obj_id;
        return ret;
    }

    static void destroy(TaskHandle* obj) {
        obj->tid += 0x100000000;
        TaskHandleFactory::put(obj->tid);
    }

public:
    JumpContext* jump(TaskHandle* from) {
        if (task_stack == nullptr) {
            task_stack = base::ObjectPool<TaskStack>::get();
        }
        JumpContext jump_context;
        jump_context.from = from;
        jump_context.to = this;

        transfer_t jump_from = jump_pcontext(task_stack->pcontext, &jump_context);
        JumpContext* tmp_context = (JumpContext*)(jump_from.data);
        tmp_context->from->task_stack->pcontext = jump_from.fctx;
        return tmp_context;
    }
};

} // end namespace thread
} // end namespace p
