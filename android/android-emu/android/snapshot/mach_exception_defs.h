// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// This file contains definitions of Mach structs that make it
// easier to handle all sorts of exception flavors/behaviors,
// since we will need to be able to forward any exception that we
// should not handle on to a previous exception handler (i.e.,
// crash reporter or debugger).
#pragma once

#include "android/utils/compiler.h"

// Definitions from:
/* machtest/main.c -- Mac OS X / Mach exception handling prototype
 *
 * Created by Richard Brooksby on 2013-06-24.
 * Copyright 2013 Ravenbrook Limited.
 *
 * REFERENCES
 * http://www.mikeash.com/pyblog/friday-qa-2013-01-11-mach-exception-handlers.html
 */

#include <mach/mach_port.h>
#include <mach/mach_init.h>
#include <mach/task.h>
#include <mach/thread_act.h>
#include <mach/thread_status.h>
#include <mach/mach_error.h>
#include <mach/i386/thread_status.h>
#include <mach/mach_error.h>
#include <mach/exc.h>

ANDROID_BEGIN_HEADER

/* Almost all of this file is architecture-independent, but unfortunately
   the Mach headers don't provide architecture neutral symbols for simple
   things like thread states.  These definitions fix that */

// Only define for x86_64

#define MY_THREAD_STATE_COUNT x86_THREAD_STATE64_COUNT
#define MY_THREAD_STATE_FLAVOR x86_THREAD_STATE64
typedef x86_thread_state64_t my_thread_state_t;

#define MY_IP __rip

/* The Mach headers in /usr/include/mach and the code generated by the Mach
   Interface Generator (mig) program are not the truth.  In particular, if
   you specify your exception behaviour with the MACH_EXCEPTION_CODES flag
   then you get a different layout with 64-bit wide exception code and
   subcode fields.  We want those (so we can get the faulting address on
   x86_64) but also we must cope with passing them onwards to other
   exception handlers in the debugger or whatever.  So we're forced to
   define our own copies of these structures.  Makes a bit of a nonsense of
   having them in /usr/include or mig at all, really.
   
   See http://stackoverflow.com/questions/2824105/handling-mach-exceptions-in-64bit-os-x-application
   about MACH_EXCEPTION_CODES. */

/* Structure packing really makes a difference because the code field is
   not naturally aligned on 64-bit. */

#ifdef  __MigPackStructs
#pragma pack(4)
#endif

typedef struct {
    mach_msg_header_t Head;
    mach_msg_body_t msgh_body;
    mach_msg_port_descriptor_t thread;
    mach_msg_port_descriptor_t task;
    NDR_record_t NDR;
    exception_type_t exception;
    mach_msg_type_number_t codeCnt;
    int64_t code[2];
} __Request__mach_exception_raise_t;

typedef struct {
	mach_msg_header_t Head;
	NDR_record_t NDR;
	exception_type_t exception;
	mach_msg_type_number_t codeCnt;
	int64_t code[2];
	int flavor;
	mach_msg_type_number_t old_stateCnt;
	natural_t old_state[224];
} __Request__mach_exception_raise_state_t;

typedef struct {
	mach_msg_header_t Head;
	mach_msg_body_t msgh_body;
	mach_msg_port_descriptor_t thread;
	mach_msg_port_descriptor_t task;
	NDR_record_t NDR;
	exception_type_t exception;
	int codeCnt;
    int64_t code[2];
	int flavor;
	mach_msg_type_number_t old_stateCnt;
	natural_t old_state[224];
} __Request__mach_exception_raise_state_identity_t;

// Just define the reply struct for raise_state_identity only for now,
// because that's the only kind of exception we are interested in emitting
// for our purposes.

typedef struct {
    mach_msg_header_t Head;
    NDR_record_t NDR;
    kern_return_t RetCode;
    int flavor;
    mach_msg_type_number_t new_stateCnt;
    natural_t new_state[224];
} __Reply__mach_exception_raise_state_identity_t __attribute__((unused));
    
#ifdef  __MigPackStructs
#pragma pack()
#endif

typedef union AnyRequest {
    mach_msg_header_t Head;
    __Request__mach_exception_raise_t raise;
    __Request__mach_exception_raise_state_t raise_state;
    __Request__mach_exception_raise_state_identity_t raise_state_identity;
} AnyRequest;

/* These are the message IDs that appear in request messages, determined
   by experimentation.  The replies to these messages are these + 100. */

#define MSG_ID_REQUEST_32 2401
#define MSG_ID_REQUEST_STATE_32 2402
#define MSG_ID_REQUEST_STATE_IDENTITY_32 2403

#define MSG_ID_REQUEST_64 2405
#define MSG_ID_REQUEST_STATE_64 2406
#define MSG_ID_REQUEST_STATE_IDENTITY_64 2407

/* Convert request messages between 32- and 64-bit code code layouts.
   May truncate 64-bit codes, of course, but that appears to be what
   Mac OS X does. */

#define COPY_COMMON(dst, src, id, code_type) \
        do { \
                (dst)->Head = (src)->Head; \
                (dst)->Head.msgh_id = (id); \
                (dst)->Head.msgh_size = sizeof(*dst); \
                (dst)->NDR = (src)->NDR; \
                (dst)->exception = (src)->exception; \
                (dst)->codeCnt = (src)->codeCnt; \
                (dst)->code[0] = (code_type)(src)->code[0]; \
                (dst)->code[1] = (code_type)(src)->code[1]; \
        } while(0)

#define COPY_IDENTITY(dst, src) \
        do { \
                (dst)->thread = (src)->thread; \
                (dst)->task = (src)->task; \
        } while(0)

#define COPY_STATE(dst, src, state_flavor, state, state_count) \
        do { \
                mach_msg_size_t _s; \
                (dst)->flavor = (state_flavor); \
                (dst)->old_stateCnt = (state_count); \
                _s = (dst)->old_stateCnt * sizeof(natural_t); \
                assert(_s <= sizeof((dst)->old_state)); \
                memcpy((dst)->old_state, (state), _s); \
                (dst)->Head.msgh_size = \
                        offsetof(__Request__mach_exception_raise_state_t, old_state) + _s; \
        } while(0)

#define COPY_REQUEST(dst, src, width) \
        do { \
                COPY_COMMON(dst, src, MSG_ID_REQUEST_##width, __int##width##_t); \
                COPY_IDENTITY(dst, src); \
        } while(0);

#define COPY_REQUEST_STATE(dst, src, width, state_flavor, state, state_count) \
        do { \
                COPY_COMMON(dst, src, MSG_ID_REQUEST_STATE_##width, __int##width##_t); \
                COPY_STATE(dst, src, state_flavor, state, state_count); \
        } while(0)

#define COPY_REQUEST_STATE_IDENTITY(dst, src, width, state_flavor, state, state_count) \
        do { \
                COPY_COMMON(dst, src, MSG_ID_REQUEST_STATE_IDENTITY_##width, __int##width##_t); \
                COPY_IDENTITY(dst, src); \
                COPY_STATE(dst, src, state_flavor, state, state_count); \
        } while(0)

// For convenience when querying old exception ports, gather all
// the info into a single struct.
typedef struct  {
    mach_msg_type_number_t count;
    exception_mask_t masks[10];
    mach_port_name_t ports[10];
    exception_behavior_t behaviors[10];
    thread_state_flavor_t flavors[10];
} exception_port_settings;

ANDROID_END_HEADER
