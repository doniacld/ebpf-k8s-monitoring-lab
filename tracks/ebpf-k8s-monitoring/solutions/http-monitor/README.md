# eBPF HTTP Monitoring Lab

This repository contains the exercises for the "Build an eBPF Monitoring Tool for Kubernetes" Instruqt lab.

## Structure

```
.
├── bpf/
│   ├── http_monitor.c    # eBPF C program (COMPLETE - no editing needed)
│   ├── http_monitor.h    # Event struct definition
│   └── vmlinux.h         # Kernel types (generated)
├── manifests/
│   ├── daemonset.yaml    # Kubernetes DaemonSet (has TODOs)
│   ├── rbac.yaml         # RBAC configuration (complete)
│   └── service.yaml      # Service definition (complete)
├── main.go               # Go agent (has 4 TODOs)
├── go.mod                # Go dependencies
├── Makefile              # Build commands
├── Dockerfile            # Container image
└── README.md             # This file
```

## What You'll Build

An HTTP monitoring tool that:
- Traces HTTP requests using eBPF
- Exports Prometheus metrics
- Deploys as a Kubernetes DaemonSet
- Visualizes metrics in Grafana

## Prerequisites

- Go 1.23+
- Linux kernel 5.7+
- clang/llvm for eBPF compilation
- Kubernetes cluster (provided in lab)

## Exercises

### Exercise 1: Complete Go Agent (main.go)

Fill in 4 TODOs:
1. Define Prometheus metric
2. Load eBPF objects
3. Attach kprobe
4. Create ring buffer reader

### Exercise 2: Complete DaemonSet (manifests/daemonset.yaml)

Fill in 3 TODOs:
1. Add Prometheus annotations
2. Specify container image
3. Add control-plane node toleration

## Building

```bash
# Generate eBPF code and Go bindings
make generate

# Build the binary
make build

# Run locally (requires root)
sudo ./http-monitor
```

## Deploying to Kubernetes

```bash
# Apply manifests
kubectl apply -f manifests/rbac.yaml
kubectl apply -f manifests/service.yaml
kubectl apply -f manifests/daemonset.yaml

# Check status
kubectl get daemonset http-monitor
kubectl logs -l app=http-monitor
```

## Testing Metrics

```bash
# Port-forward to metrics endpoint
kubectl port-forward svc/http-monitor 9090:9090

# Query metrics
curl localhost:9090/metrics | grep http_requests_total
```

## License

Apache 2.0
