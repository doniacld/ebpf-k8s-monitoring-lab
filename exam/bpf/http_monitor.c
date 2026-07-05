//go:build ignore

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} http_events SEC(".maps");

// Exercise 1: add a protocol field to this struct
struct http_event {
    u32 pid;
    char comm[16];
    char method[8];
    char path[128];
};

struct http_event *unused __attribute__((unused));

SEC("kprobe/tcp_sendmsg")
int trace_tcp_sendmsg(struct pt_regs *ctx) {
    char comm[16];
    bpf_get_current_comm(&comm, sizeof(comm));

    int is_http_client = 0;
    if ((comm[0] == 'c' && comm[1] == 'u' && comm[2] == 'r' && comm[3] == 'l') ||
        (comm[0] == 'w' && comm[1] == 'g' && comm[2] == 'e' && comm[3] == 't')) {
        is_http_client = 1;
    }

    if (!is_http_client) {
        return 0;
    }

    struct http_event *event = bpf_ringbuf_reserve(&http_events, sizeof(struct http_event), 0);
    if (!event) {
        return 0;
    }

    event->pid = bpf_get_current_pid_tgid() >> 32;
    __builtin_memcpy(event->comm, comm, sizeof(event->comm));
    __builtin_memcpy(event->method, "GET", 3);
    __builtin_memcpy(event->path, "/*", 2);
    // Exercise 2: set event->protocol to "TCP" here

    bpf_ringbuf_submit(event, 0);
    return 0;
}

// Exercise 3: add a new kprobe on udp_sendmsg (trace_udp_sendmsg) here

char LICENSE[] SEC("license") = "GPL";
