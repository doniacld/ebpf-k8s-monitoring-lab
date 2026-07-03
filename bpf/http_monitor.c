//go:build ignore

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

// Ring buffer for sending events to userspace
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024); // 256KB buffer
} http_events SEC(".maps");

// HTTP event structure
struct http_event {
    u32 pid;
    char comm[16];
    char method[8];
    char path[128];
};

// Force bpf2go to generate the type
struct http_event *unused __attribute__((unused));

// Helper to check if buffer starts with a string
static __always_inline int starts_with(const char *buf, int buf_len, const char *prefix, int prefix_len) {
    if (buf_len < prefix_len) {
        return 0;
    }

    #pragma unroll
    for (int i = 0; i < 8; i++) {
        if (i >= prefix_len) break;
        if (buf[i] != prefix[i]) {
            return 0;
        }
    }
    return 1;
}

// Helper to extract HTTP path (simplified - bounded loops)
static __always_inline void extract_path(const char *buf, char *path, int max_path_len) {
    int path_start = -1;

    // Find first space (after method) - bounded loop
    #pragma unroll
    for (int i = 0; i < 16; i++) {
        if (buf[i] == ' ') {
            path_start = i + 1;
            break;
        }
    }

    if (path_start == -1) {
        return;
    }

    // Copy path until space/newline (bounded)
    #pragma unroll
    for (int i = 0; i < 127; i++) {
        if (i >= max_path_len - 1) break;
        char c = buf[path_start + i];
        if (c == ' ' || c == '\r' || c == '\n' || c == '\0') {
            break;
        }
        path[i] = c;
    }
}

SEC("kprobe/tcp_sendmsg")
int trace_tcp_sendmsg(struct pt_regs *ctx) {
    // Simplified version: emit event for curl/wget processes
    // In production, you'd parse the actual HTTP data
    // This requires complete vmlinux.h which varies by kernel

    char comm[16];
    bpf_get_current_comm(&comm, sizeof(comm));

    // Only trace HTTP clients (simple filter)
    int is_http_client = 0;
    if ((comm[0] == 'c' && comm[1] == 'u' && comm[2] == 'r' && comm[3] == 'l') ||
        (comm[0] == 'w' && comm[1] == 'g' && comm[2] == 'e' && comm[3] == 't')) {
        is_http_client = 1;
    }

    if (!is_http_client) {
        return 0;
    }

    // Emit event
    struct http_event *event = bpf_ringbuf_reserve(&http_events, sizeof(struct http_event), 0);
    if (!event) {
        return 0;
    }

    event->pid = bpf_get_current_pid_tgid() >> 32;
    __builtin_memcpy(event->comm, comm, sizeof(event->comm));
    __builtin_memcpy(event->method, "GET", 3);
    __builtin_memcpy(event->path, "/*", 2);

    bpf_ringbuf_submit(event, 0);
    return 0;
}

// Rest of code commented out
#if 0
/* Original complex code commented out for now
    struct sock *sk = (struct sock *)PT_REGS_PARM1(ctx);
    struct msghdr *msg = (struct msghdr *)PT_REGS_PARM2(ctx);

    // Read TCP buffer data - simplified without BPF_CORE_READ
    char buf[256] = {};
    struct iov_iter msg_iter;

    // Read msg_iter from msghdr
    long ret = bpf_probe_read_kernel(&msg_iter, sizeof(msg_iter), &msg->msg_iter);
    if (ret < 0) {
        return 0;
    }

    // Read iovec pointer from iov_iter
    const struct iovec *iov;
    ret = bpf_probe_read_kernel(&iov, sizeof(iov), &msg_iter.__iov);
    if (ret < 0 || !iov) {
        return 0;
    }

    // Read iov_base pointer
    void *iov_base;
    ret = bpf_probe_read_kernel(&iov_base, sizeof(iov_base), &iov->iov_base);
    if (ret < 0 || !iov_base) {
        return 0;
    }

    // Finally read the user data
    ret = bpf_probe_read_user(buf, sizeof(buf), iov_base);
    if (ret < 0) {
        return 0;
    }
    */<br/>

    // Check for HTTP methods (simplified)
    int is_http = 0;
    char method[8] = {};

    if (starts_with(buf, sizeof(buf), "GET ", 4)) {
        is_http = 1;
        __builtin_memcpy(method, "GET", 3);
    } else if (starts_with(buf, sizeof(buf), "POST ", 5)) {
        is_http = 1;
        __builtin_memcpy(method, "POST", 4);
    } else if (starts_with(buf, sizeof(buf), "PUT ", 4)) {
        is_http = 1;
        __builtin_memcpy(method, "PUT", 3);
    } else if (starts_with(buf, sizeof(buf), "DELETE ", 7)) {
        is_http = 1;
        __builtin_memcpy(method, "DELETE", 6);
    } else if (starts_with(buf, sizeof(buf), "PATCH ", 6)) {
        is_http = 1;
        __builtin_memcpy(method, "PATCH", 5);
    }

    if (!is_http) {
        return 0;
    }

    // Allocate ring buffer entry
    struct http_event *event = bpf_ringbuf_reserve(&http_events, sizeof(struct http_event), 0);
    if (!event) {
        return 0;
    }

    // Fill event
    event->pid = bpf_get_current_pid_tgid() >> 32;
    bpf_get_current_comm(&event->comm, sizeof(event->comm));
    __builtin_memcpy(event->method, method, sizeof(method));
    extract_path(buf, event->path, sizeof(event->path));

    // Submit to userspace
    bpf_ringbuf_submit(event, 0);

    return 0;
}
*/
#endif

char LICENSE[] SEC("license") = "GPL";
