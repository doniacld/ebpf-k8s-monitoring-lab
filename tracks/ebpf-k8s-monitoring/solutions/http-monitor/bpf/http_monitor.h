/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __HTTP_MONITOR_H
#define __HTTP_MONITOR_H

#define TASK_COMM_LEN 16
#define HTTP_METHOD_LEN 8
#define HTTP_PATH_LEN 64

struct http_event {
    __u32 pid;
    __u32 tid;
    char comm[TASK_COMM_LEN];
    char method[HTTP_METHOD_LEN];
    char path[HTTP_PATH_LEN];
    __u64 timestamp;
};

#endif /* __HTTP_MONITOR_H */
