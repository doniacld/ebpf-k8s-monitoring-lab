/* SPDX-License-Identifier: GPL-2.0 */
/* vmlinux.h - Minimal kernel types for eBPF program (Instruqt lab version) */

#ifndef __VMLINUX_H__
#define __VMLINUX_H__

/* Unsigned fixed-width types */
typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned int __u32;
typedef unsigned long long __u64;

/* Signed fixed-width types (referenced by libbpf's bpf_helper_defs.h) */
typedef signed char __s8;
typedef short __s16;
typedef int __s32;
typedef long long __s64;

/* Endian / checksum aliases (referenced by libbpf's bpf_helper_defs.h) */
typedef __u16 __be16;
typedef __u16 __le16;
typedef __u32 __be32;
typedef __u32 __le32;
typedef __u64 __be64;
typedef __u64 __le64;
typedef __u32 __wsum;

typedef __u8 u8;
typedef __u16 u16;
typedef __u32 u32;
typedef __u64 u64;

/* Only the map type this program uses. Value matches the kernel UAPI enum. */
enum bpf_map_type {
    BPF_MAP_TYPE_RINGBUF = 27,
};

/* Minimal kernel structures for http_monitor.c */
struct sock {};
struct iovec { void *iov_base; __u64 iov_len; };
struct iov_iter { const struct iovec *__iov; };
struct msghdr { void *msg_name; int msg_namelen; struct iov_iter msg_iter; };
struct pt_regs {};

#endif /* __VMLINUX_H__ */
