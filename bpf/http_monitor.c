// SPDX-License-Identifier: GPL-2.0
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include "http_monitor.h"

char LICENSE[] SEC("license") = "GPL";

// Ring buffer for sending events to userspace
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024); // 256 KB
} http_events SEC(".maps");

// Parse HTTP request from buffer
static __always_inline int parse_http_request(const char *buf, __u64 buf_len,
                                               struct http_event *event) {
    // Parse HTTP method (GET, POST, PUT, DELETE, etc.)
    // Simple parser: look for first space
    int i = 0;
    for (i = 0; i < HTTP_METHOD_LEN - 1 && i < buf_len; i++) {
        char c;
        bpf_probe_read_kernel(&c, 1, buf + i);
        if (c == ' ') {
            break;
        }
        event->method[i] = c;
    }
    event->method[i] = '\0';

    // Skip space
    i++;

    // Parse HTTP path (until space or ?)
    int path_start = i;
    int path_idx = 0;
    for (; i < buf_len && path_idx < HTTP_PATH_LEN - 1; i++) {
        char c;
        bpf_probe_read_kernel(&c, 1, buf + i);
        if (c == ' ' || c == '?') {
            break;
        }
        event->path[path_idx++] = c;
    }
    event->path[path_idx] = '\0';

    return 0;
}

// Trace tcp_sendmsg to capture HTTP requests
SEC("kprobe/tcp_sendmsg")
int BPF_KPROBE(trace_tcp_sendmsg, struct sock *sk, struct msghdr *msg, size_t size) {
    // Only process if this looks like an HTTP request
    // HTTP requests start with methods: GET, POST, PUT, DELETE, HEAD, OPTIONS, etc.
    if (size < 4) {
        return 0;
    }

    // Read first 4 bytes to check for HTTP methods
    char buf[128] = {};
    struct iov_iter *iter = &msg->msg_iter;

    // Read from iovec
    if (iter->iov_offset >= size) {
        return 0;
    }

    // Simplified: read up to 128 bytes
    __u64 to_read = size < sizeof(buf) ? size : sizeof(buf);

    // BPF_CORE_READ is more reliable for kernel struct access
    const struct iovec *iov = BPF_CORE_READ(iter, iov);
    if (!iov) {
        return 0;
    }

    void *iov_base = BPF_CORE_READ(iov, iov_base);
    if (!iov_base) {
        return 0;
    }

    bpf_probe_read_kernel(buf, to_read, iov_base);

    // Check if it starts with HTTP methods
    if (buf[0] != 'G' && buf[0] != 'P' && buf[0] != 'D' &&
        buf[0] != 'H' && buf[0] != 'O') {
        return 0;
    }

    // Allocate event
    struct http_event *event = bpf_ringbuf_reserve(&http_events, sizeof(*event), 0);
    if (!event) {
        return 0;
    }

    // Fill event data
    event->pid = bpf_get_current_pid_tgid() >> 32;
    event->tid = bpf_get_current_pid_tgid();
    bpf_get_current_comm(&event->comm, sizeof(event->comm));
    event->timestamp = bpf_ktime_get_ns();

    // Parse HTTP request
    parse_http_request(buf, to_read, event);

    // Submit event to userspace
    bpf_ringbuf_submit(event, 0);

    return 0;
}
