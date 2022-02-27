/*
 * kAFl/Nyx low-level interface definitions
 *
 * Copyright 2022 Sergej Schumilo, Cornelius Aschermann
 * Copyright 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef NYX_API_H
#define NYX_API_H

#define HYPERCALL_KAFL_RAX_ID				0x01f
#define HYPERCALL_KAFL_ACQUIRE				0
#define HYPERCALL_KAFL_GET_PAYLOAD			1
#define HYPERCALL_KAFL_GET_PROGRAM			2  /* deprecated */
#define HYPERCALL_KAFL_GET_ARGV				3  /* deprecated */
#define HYPERCALL_KAFL_RELEASE				4
#define HYPERCALL_KAFL_SUBMIT_CR3			5
#define HYPERCALL_KAFL_SUBMIT_PANIC			6
#define HYPERCALL_KAFL_SUBMIT_KASAN			7
#define HYPERCALL_KAFL_PANIC				8
#define HYPERCALL_KAFL_KASAN				9
#define HYPERCALL_KAFL_LOCK					10 /* deprecated */
#define HYPERCALL_KAFL_INFO					11 /* deprecated */
#define HYPERCALL_KAFL_NEXT_PAYLOAD			12
#define HYPERCALL_KAFL_PRINTF				13
#define HYPERCALL_KAFL_PRINTK_ADDR			14 /* deprecated */
#define HYPERCALL_KAFL_PRINTK				15 /* deprecated */

/* user space only hypercalls */
#define HYPERCALL_KAFL_USER_RANGE_ADVISE	16
#define HYPERCALL_KAFL_USER_SUBMIT_MODE		17
#define HYPERCALL_KAFL_USER_FAST_ACQUIRE	18
/* 19 is already used for exit reason KVM_EXIT_KAFL_TOPA_MAIN_FULL */
#define HYPERCALL_KAFL_USER_ABORT			20
#define HYPERCALL_KAFL_TIMEOUT				21 /* deprecated */
#define HYPERCALL_KAFL_RANGE_SUBMIT		29
#define HYPERCALL_KAFL_REQ_STREAM_DATA		30
#define HYPERCALL_KAFL_PANIC_EXTENDED		32

#define HYPERCALL_KAFL_CREATE_TMP_SNAPSHOT	33
#define HYPERCALL_KAFL_DEBUG_TMP_SNAPSHOT	34 /* hypercall for debugging / development purposes */

#define HYPERCALL_KAFL_GET_HOST_CONFIG		35
#define HYPERCALL_KAFL_SET_AGENT_CONFIG		36

#define HYPERCALL_KAFL_DUMP_FILE			37

#define HYPERCALL_KAFL_REQ_STREAM_DATA_BULK 38
#define HYPERCALL_KAFL_PERSIST_PAGE_PAST_SNAPSHOT 39

/* hypertrash only hypercalls */
#define HYPERTRASH_HYPERCALL_MASK			0xAA000000

#define HYPERCALL_KAFL_NESTED_PREPARE		(0 | HYPERTRASH_HYPERCALL_MASK)
#define HYPERCALL_KAFL_NESTED_CONFIG		(1 | HYPERTRASH_HYPERCALL_MASK)
#define HYPERCALL_KAFL_NESTED_ACQUIRE		(2 | HYPERTRASH_HYPERCALL_MASK)
#define HYPERCALL_KAFL_NESTED_RELEASE		(3 | HYPERTRASH_HYPERCALL_MASK)
#define HYPERCALL_KAFL_NESTED_HPRINTF		(4 | HYPERTRASH_HYPERCALL_MASK)gre

#define HPRINTF_MAX_SIZE					0x1000					/* up to 4KB hprintf strings */
#define PAYLOAD_MAX_SIZE                    (128*1024)

#define KAFL_MODE_64	0
#define KAFL_MODE_32	1
#define KAFL_MODE_16	2

typedef union {
    struct {
        unsigned int dump_observed :1;
        unsigned int dump_stats :1;
        unsigned int dump_callers :1;
    };
    UINT32 raw_data;
} __attribute__((packed)) agent_flags_t;

typedef struct {
	agent_flags_t flags;
	INT32 size;
	UINT8 data[];
} kAFL_payload;

typedef struct {
	UINT64 ip[4];
	UINT64 size[4];
	UINT8 enabled[4];
} kAFL_ranges; 

static inline UINT64 kAFL_hypercall(UINT64 p1, UINT64 p2)
{
	UINT64 nr = HYPERCALL_KAFL_RAX_ID;
	asm volatile ("vmcall"
				  : "=a" (nr)
				  : "a"(nr), "b"(p1), "c"(p2));
	return nr;
}

static void habort(char* msg) __attribute__ ((unused));
static void habort(char* msg){
	kAFL_hypercall(HYPERCALL_KAFL_USER_ABORT, (UINTN)msg);
}

static void hprintf(const char *msg)  __attribute__ ((unused));
static void hprintf(const char *msg){
	kAFL_hypercall(HYPERCALL_KAFL_PRINTF, (UINTN)msg);
}

#define NYX_HOST_MAGIC  0x4878794e
#define NYX_AGENT_MAGIC 0x4178794e

#define NYX_HOST_VERSION 1
#define NYX_AGENT_VERSION 1

typedef struct host_config_s {
	UINT32 host_magic;
	UINT32 host_version;
	UINT32 bitmap_size;
	UINT32 ijon_bitmap_size;
	UINT32 payload_buffer_size;
	UINT32 worker_id;
	/* more to come */
} __attribute__((packed)) host_config_t;

typedef struct agent_config_s {
	UINT32 agent_magic;
	UINT32 agent_version;
	UINT8 agent_timeout_detection;
	UINT8 agent_tracing;
	UINT8 agent_ijon_tracing;
	UINT8 agent_non_reload_mode;
	UINT64 trace_buffer_vaddr;
	UINT64 ijon_trace_buffer_vaddr;
	UINT32 coverage_bitmap_size;
	UINT32 input_buffer_size;
	UINT8 dump_payloads; /* set by hypervisor */
	/* more to come */
} __attribute__((packed)) agent_config_t;

typedef struct kafl_dump_file_s {
	UINT64 file_name_str_ptr;
	UINT64 data_ptr;
	UINT64 bytes;
	UINT8 append;
} __attribute__((packed)) kafl_dump_file_t;

#endif /* NYX_API_H */
