# eBPF Kubernetes Monitoring Lab - Exercises

This repository contains the starter code for the **eBPF Kubernetes Monitoring** lab on Instruqt.

## Structure

```
ebpf-k8s-monitoring-lab/
├── main.go                 # Go userspace with 4 TODOs
├── bpf/
│   ├── http_monitor.c      # eBPF program (complete)
│   └── vmlinux.h          # Kernel headers
├── manifests/              # Kubernetes YAML with TODOs
│   ├── daemonset.yaml      # 3 TODOs to complete
│   ├── rbac.yaml           # Complete
│   ├── service.yaml        # Complete
│   └── *.yaml
├── Makefile                # Build commands
├── Dockerfile              # Container image
├── go.mod                  # Go dependencies
└── go.sum
```

## TODOs

### Challenge 03: Complete Go Agent (4 TODOs)

**main.go:**
1. TODO 1: Define Prometheus metric (line ~27)
2. TODO 2: Load eBPF objects (line ~43)
3. TODO 3: Attach kprobe (line ~51)
4. TODO 4: Create ring buffer reader (line ~61)

### Challenge 04: Deploy DaemonSet (3 TODOs)

**manifests/daemonset.yaml:**
1. TODO 1: Add Prometheus annotations
2. TODO 2: Set container image
3. TODO 3: Add node tolerations

## What's Complete

- ✅ eBPF C program (`bpf/http_monitor.c`)
- ✅ RBAC configuration (`manifests/rbac.yaml`)
- ✅ Service configuration (`manifests/service.yaml`)
- ✅ Makefile, Dockerfile, dependencies

## Building Locally

```bash
# Generate eBPF bytecode
make generate

# Build binary
make build

# Build container image
docker build -t http-monitor:v1.0 .

# Load to Kind (if using Kind)
kind load docker-image http-monitor:v1.0

# Deploy to Kubernetes
kubectl apply -f manifests/
```

## Features

This HTTP monitoring agent:
- Traces HTTP requests using eBPF kprobe on `tcp_sendmsg`
- Exports Prometheus metrics
- Runs as a Kubernetes DaemonSet (one pod per node)
- Visualizes data in Grafana

## Lab Link

Complete this lab on Instruqt:
https://play.instruqt.com/isovalent/tracks/ebpf-k8s-monitoring

## License

Copyright © Isovalent
