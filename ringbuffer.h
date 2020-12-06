/*
 *  Simple Ring Buffer
 *
 *  Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include <utils/attribute.h>
#include <string.h>

//------------------------------------------------------------------------------
typedef struct
{
    void*  buffer;
    size_t size;
    size_t head; // position where data starts
    size_t fill; // ToDo: use tail
} ringbuffer_t;


//------------------------------------------------------------------------------
static inline void
ringbuffer_clear(
    ringbuffer_t* rb)
{
    assert( NULL != rb );

    rb->head = 0;
    rb->fill = 0;
}
//------------------------------------------------------------------------------
static inline size_t
ringbuffer_get_capacity(
    ringbuffer_t* rb)
{
    assert( NULL != rb );

    return rb->size;
}


//------------------------------------------------------------------------------
static inline size_t
ringbuffer_get_used(
    ringbuffer_t* rb)
{
    assert( NULL != rb );

    assert( rb->size >= rb->fill );

    return rb->fill;
}


//------------------------------------------------------------------------------
static inline bool
ringbuffer_is_empty(
    ringbuffer_t* rb)
{
    assert( NULL != rb );

    assert( rb->size >= rb->fill );

    return (0 == rb->fill);
}


//------------------------------------------------------------------------------
static inline size_t
ringbuffer_get_free(
    ringbuffer_t* rb)
{
    assert( NULL != rb );

    assert( rb->size >= rb->fill );

    return rb->size - rb->fill;
}


//------------------------------------------------------------------------------
static inline bool
ringbuffer_is_full(
    ringbuffer_t* rb)
{
    assert( NULL != rb );

    assert( rb->size >= rb->fill );

    return (rb->size == rb->fill);
}


//------------------------------------------------------------------------------
static inline void
ringbuffer_init(
    ringbuffer_t* rb,
    void* buffer,
    size_t len)
{
    assert( NULL != rb );
    assert( (0 == len) || (NULL != buffer) );

    rb->buffer = buffer;
    rb->size = len;

    ringbuffer_clear(rb);
}


//------------------------------------------------------------------------------
static inline size_t
ringbuffer_write(
    ringbuffer_t* rb,
    const void* src,
    size_t len)
{
    assert( NULL != rb );
    assert( (0 == len) || (NULL != src) );

    size_t used = rb->fill; // read only once to guarantee consistency
    assert( used <= rb->size );

    size_t free = rb->size - used;
    if (len > free)
    {
        len = free;
    }

    size_t pos_head = rb->head;
    assert( pos_head <= rb->size );

    size_t pos_free = (pos_head + used) % rb->size;

    void* buf_free = (void*)( (uintptr_t)rb->buffer + pos_free );

    size_t len1 = (pos_free + len <= rb->size) ? len : rb->size - pos_free;
    if (len1 > 0)
    {
        memcpy(buf_free, src, len1);

        size_t len2 = len - len1;
        if (len2 > 0)
        {
            void* src2 = (void*)( (uintptr_t)src + len1 );
            memcpy(rb->buffer, src2, len2);
        }

        // this does read-copy-write and thus it's not thread-safe
        rb->fill += len;
        assert( rb->fill <= rb->size );
    }

    return len;
}


//------------------------------------------------------------------------------
// read or flush if dst is NULL
static inline size_t
ringbuffer_read(
    ringbuffer_t* rb,
    void* dst,
    size_t len)
{
    assert( NULL != rb );

    size_t avail = rb->fill; // read only once to guarantee consistency
    assert( avail <= rb->size );

    if (len > avail)
    {
        len = avail;
    }

    size_t pos_head = rb->head;

    if (NULL != dst)
    {
        void* head = (void*)( (uintptr_t)rb->buffer + pos_head );

        size_t len1 = (pos_head + len <= rb->size) ? len : rb->size - pos_head;
        if (len1 > 0)
        {
            memcpy(dst, head, len1);

            size_t len2 = len - len1;
            if (len2 > 0)
            {
                void* dst2 = (void*)( (uintptr_t)dst + len1 );
                memcpy(dst2, rb->buffer, len2);
            }
        }
    }

    if (len > 0)
    {
        rb->head = (pos_head + len) % rb->size;

        // we've adjusted len before, so the subtraction is safe. Furthermore,
        // a write shall only increase the amount of data, but never reduce it,
        // so this can't get negative. However, this is still not thread safe,
        // because it does a read-modify-write.
        rb->fill -= len;
    }

    return len;
}


//------------------------------------------------------------------------------
static inline size_t
ringbuffer_get_read_ptr(
    ringbuffer_t* rb,
    void** ptr)
{
    assert( NULL != rb );
    assert( NULL != ptr );

    size_t avail = rb->fill; // read only once to guarantee consistency
    assert( avail <= rb->size );

    size_t pos_head = rb->head;
    assert( pos_head <= rb->size );

    size_t len = (pos_head + avail <= rb->size) ? avail : rb->size - pos_head;
    *ptr = (void*)( (uintptr_t)rb->buffer + pos_head );

    return len;
}
