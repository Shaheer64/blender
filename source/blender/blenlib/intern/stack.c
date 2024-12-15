/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <cstdlib>
#include <cstring>
#include <vector>
#include "BLI_stack.h"
#include "MEM_guardedalloc.h"

class StackChunk {
public:
    StackChunk *next;
    std::vector<char> data;

    StackChunk(size_t chunk_size) : next(nullptr), data(chunk_size) {}
};

class BLI_StackImpl {
public:
    StackChunk *chunk_curr;
    StackChunk *chunk_free;
    size_t chunk_index;
    size_t chunk_elem_max;
    size_t elem_size;
    size_t elem_num;

    BLI_StackImpl(size_t elem_size, size_t chunk_size)
        : chunk_curr(nullptr),
          chunk_free(nullptr),
          chunk_index(chunk_size - 1),
          chunk_elem_max(chunk_size),
          elem_size(elem_size),
          elem_num(0) {}

    ~BLI_StackImpl() {
        clear_chunks(chunk_curr);
        clear_chunks(chunk_free);
    }

    void *push_r() {
        if (++chunk_index == chunk_elem_max) {
            allocate_new_chunk();
            chunk_index = 0;
        }
        elem_num++;
        return get_last_elem();
    }

    void push(const void *src) {
        void *dst = push_r();
        std::memcpy(dst, src, elem_size);
    }

    void pop(void *dst) {
        std::memcpy(dst, get_last_elem(), elem_size);
        discard();
    }

    void *peek() {
        return get_last_elem();
    }

    void discard() {
        if (--chunk_index == SIZE_MAX) {
            auto *old_chunk = chunk_curr;
            chunk_curr = chunk_curr->next;
            old_chunk->next = chunk_free;
            chunk_free = old_chunk;
            chunk_index = chunk_elem_max - 1;
        }
        elem_num--;
    }

    void clear() {
        chunk_index = chunk_elem_max - 1;
        if (chunk_free) {
            if (chunk_curr) {
                StackChunk *last_free = chunk_free;
                while (last_free->next) {
                    last_free = last_free->next;
                }
                last_free->next = chunk_curr;
            }
        } else {
            chunk_free = chunk_curr;
        }
        chunk_curr = nullptr;
        elem_num = 0;
    }

    size_t count() const { return elem_num; }

    bool is_empty() const { return elem_num == 0; }

private:
    void *get_last_elem() {
        return chunk_curr->data.data() + (elem_size * chunk_index);
    }

    void allocate_new_chunk() {
        if (chunk_free) {
            chunk_curr = chunk_free;
            chunk_free = chunk_free->next;
        } else {
            chunk_curr = new StackChunk(elem_size * chunk_elem_max);
        }
        chunk_curr->next = nullptr;
    }

    void clear_chunks(StackChunk *chunk) {
        while (chunk) {
            StackChunk *next = chunk->next;
            delete chunk;
            chunk = next;
        }
    }
};

// C Interface Wrappers
extern "C" {
BLI_Stack *BLI_stack_new_ex(size_t elem_size, const char * /*description*/, size_t chunk_size) {
    return reinterpret_cast<BLI_Stack *>(new BLI_StackImpl(elem_size, chunk_size));
}

BLI_Stack *BLI_stack_new(size_t elem_size, const char *description) {
    return BLI_stack_new_ex(elem_size, description, 1 << 16);
}

void BLI_stack_free(BLI_Stack *stack) {
    delete reinterpret_cast<BLI_StackImpl *>(stack);
}

void *BLI_stack_push_r(BLI_Stack *stack) {
    return reinterpret_cast<BLI_StackImpl *>(stack)->push_r();
}

void BLI_stack_push(BLI_Stack *stack, const void *src) {
    reinterpret_cast<BLI_StackImpl *>(stack)->push(src);
}

void BLI_stack_pop(BLI_Stack *stack, void *dst) {
    reinterpret_cast<BLI_StackImpl *>(stack)->pop(dst);
}

void *BLI_stack_peek(BLI_Stack *stack) {
    return reinterpret_cast<BLI_StackImpl *>(stack)->peek();
}

void BLI_stack_discard(BLI_Stack *stack) {
    reinterpret_cast<BLI_StackImpl *>(stack)->discard();
}

void BLI_stack_clear(BLI_Stack *stack) {
    reinterpret_cast<BLI_StackImpl *>(stack)->clear();
}

size_t BLI_stack_count(const BLI_Stack *stack) {
    return reinterpret_cast<const BLI_StackImpl *>(stack)->count();
}

bool BLI_stack_is_empty(const BLI_Stack *stack) {
    return reinterpret_cast<const BLI_StackImpl *>(stack)->is_empty();
}
}
