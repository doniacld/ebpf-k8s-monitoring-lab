.PHONY: all generate build clean docker-build

# Binary name
BINARY=http-monitor

# Docker image
IMAGE_NAME=http-monitor
IMAGE_TAG=v1.0

# Detect architecture
GOARCH ?= $(shell go env GOARCH)

all: generate build

# Generate eBPF bytecode and Go bindings
generate:
	go generate ./...

# Build Go binary
build:
	CGO_ENABLED=0 GOOS=linux GOARCH=$(GOARCH) go build -o $(BINARY) .

# Clean generated files
clean:
	rm -f $(BINARY)
	rm -f *_bpfel.go *_bpfel.o
	rm -f *_bpfeb.go *_bpfeb.o

# Build Docker image
docker-build: generate build
	docker build -t $(IMAGE_NAME):$(IMAGE_TAG) .

# Run locally (requires root)
run: build
	sudo ./$(BINARY)

# Install dependencies
deps:
	go mod download
	go mod tidy

# Verify eBPF program
verify:
	clang -target bpf -Wall -O2 -g -c bpf/http_monitor.c -o /tmp/http_monitor.o
	llvm-objdump -S /tmp/http_monitor.o

# ============================================
# Kubernetes Testing with Kind
# ============================================

CLUSTER_NAME=http-monitor-test

# Install kind and kubectl
kind-install:
	@echo "$(GREEN)Installing kubectl...$(NC)"
	curl -LO "https://dl.k8s.io/release/$$(curl -L -s https://dl.k8s.io/release/stable.txt)/bin/linux/$$(go env GOARCH)/kubectl"
	chmod +x kubectl
	sudo mv kubectl /usr/local/bin/
	kubectl version --client
	@echo "$(GREEN)✓ kubectl installed$(NC)"
	@echo ""
	@echo "$(GREEN)Installing kind...$(NC)"
	curl -Lo ./kind https://kind.sigs.k8s.io/dl/v0.20.0/kind-linux-$$(go env GOARCH)
	chmod +x ./kind
	sudo mv ./kind /usr/local/bin/kind
	kind version
	@echo "$(GREEN)✓ kind installed$(NC)"

# Create kind cluster
kind-create:
	@echo "$(GREEN)Creating kind cluster...$(NC)"
	kind create cluster --name $(CLUSTER_NAME) || true
	kubectl cluster-info --context kind-$(CLUSTER_NAME)
	@echo "$(GREEN)✓ Cluster ready$(NC)"

# Delete kind cluster
kind-delete:
	@echo "$(GREEN)Deleting kind cluster...$(NC)"
	kind delete cluster --name $(CLUSTER_NAME)
	@echo "$(GREEN)✓ Cluster deleted$(NC)"

# Build and load image into kind
kind-load: docker-build
	@echo "$(GREEN)Loading image into kind...$(NC)"
	kind load docker-image $(IMAGE_NAME):$(IMAGE_TAG) --name $(CLUSTER_NAME)
	@echo "$(GREEN)✓ Image loaded$(NC)"

# Deploy to kind cluster
kind-deploy: kind-load
	@echo "$(GREEN)Deploying to Kubernetes...$(NC)"
	kubectl apply -f manifests/rbac.yaml
	kubectl apply -f manifests/daemonset.yaml
	kubectl apply -f manifests/service.yaml
	@echo "$(GREEN)✓ Deployed$(NC)"
	@echo ""
	@echo "Waiting for pods to be ready..."
	kubectl wait --for=condition=ready pod -l app=http-monitor --timeout=60s || true
	@echo ""
	kubectl get pods -l app=http-monitor -o wide

# Deploy Prometheus
kind-prometheus:
	@echo "$(GREEN)Deploying Prometheus...$(NC)"
	kubectl apply -f manifests/prometheus.yaml
	@echo "$(GREEN)✓ Prometheus deployed$(NC)"
	@echo ""
	@echo "Waiting for Prometheus to be ready..."
	kubectl wait --for=condition=ready pod -l app=prometheus --timeout=120s || true
	@echo ""
	@echo "$(GREEN)Prometheus is ready!$(NC)"
	@echo ""

# Deploy Grafana
kind-grafana: kind-prometheus
	@echo "$(GREEN)Deploying Grafana...$(NC)"
	kubectl apply -f manifests/grafana.yaml
	@echo "$(GREEN)✓ Grafana deployed$(NC)"
	@echo ""
	@echo "Waiting for Grafana to be ready..."
	kubectl wait --for=condition=ready pod -l app=grafana --timeout=120s || true
	@echo ""
	@echo "$(GREEN)✓ Full monitoring stack ready!$(NC)"
	@echo "  - HTTP Monitor: collecting metrics"
	@echo "  - Prometheus: scraping and storing metrics"
	@echo "  - Grafana: visualizing dashboards"
	@echo ""
	@echo "Access Grafana at: http://localhost:3000"
	@echo "  kubectl port-forward svc/grafana 3000:3000"
	@echo ""

# Deploy with Grafana
kind-deploy-all: kind-deploy kind-grafana

# Test the deployment
kind-test: kind-deploy-all
	@echo ""
	@echo "$(GREEN)Testing HTTP monitor in Kubernetes...$(NC)"
	@echo ""
	@echo "Step 1: Check pod status"
	kubectl get pods -l app=http-monitor
	@echo ""
	@echo "Step 2: Generate HTTP traffic with test pod"
	kubectl run test-curl --image=curlimages/curl --rm -i --restart=Never -- curl -s http://example.com > /dev/null || true
	sleep 5
	@echo ""
	@echo "Step 3: Check metrics"
	@POD=$$(kubectl get pod -l app=http-monitor -o jsonpath='{.items[0].metadata.name}'); \
	kubectl port-forward pod/$$POD 9090:9090 > /dev/null 2>&1 & \
	PF_PID=$$!; \
	sleep 2; \
	echo "Fetching metrics..."; \
	curl -s localhost:9090/metrics | grep http_requests_total || echo "No metrics yet"; \
	kill $$PF_PID 2>/dev/null || true
	@echo ""
	@echo "$(GREEN)✓ Test complete$(NC)"

# Show logs from DaemonSet
kind-logs:
	@POD=$$(kubectl get pod -l app=http-monitor -o jsonpath='{.items[0].metadata.name}'); \
	kubectl logs $$POD --tail=50 -f

# Port-forward to access metrics
kind-forward:
	@POD=$$(kubectl get pod -l app=http-monitor -o jsonpath='{.items[0].metadata.name}'); \
	echo "Port-forwarding to pod $$POD..."; \
	echo "Metrics available at: http://localhost:9090/metrics"; \
	kubectl port-forward pod/$$POD 9090:9090

# Generate HTTP traffic
kind-traffic:
	@echo "$(GREEN)Generating HTTP traffic...$(NC)"
	@kubectl delete pod traffic-gen --force --grace-period=0 2>/dev/null || true
	@sleep 2
	kubectl run traffic-gen --image=curlimages/curl --rm -i --restart=Never -- sh -c \
	"for i in {1..20}; do curl -s http://example.com > /dev/null; echo 'Request '$$i; sleep 2; done"

# Generate continuous traffic (runs in background)
kind-traffic-continuous:
	@echo "$(GREEN)Starting continuous traffic generation...$(NC)"
	@kubectl delete pod traffic-continuous --force --grace-period=0 2>/dev/null || true
	@sleep 2
	kubectl run traffic-continuous --image=curlimages/curl -- sh -c \
	"while true; do curl -s http://example.com > /dev/null; sleep 3; done" &
	@echo "$(GREEN)✓ Continuous traffic started$(NC)"
	@echo "  Stop with: kubectl delete pod traffic-continuous"

# Stop continuous traffic
kind-traffic-stop:
	@echo "$(GREEN)Stopping traffic generation...$(NC)"
	kubectl delete pod traffic-gen --force --grace-period=0 2>/dev/null || true
	kubectl delete pod traffic-continuous --force --grace-period=0 2>/dev/null || true
	@echo "$(GREEN)✓ Traffic stopped$(NC)"

# Full kind workflow
kind-full: kind-create kind-test
	@echo ""
	@echo "$(GREEN)========================================$(NC)"
	@echo "$(GREEN)✓ Full Kind Test Complete!$(NC)"
	@echo "$(GREEN)========================================$(NC)"
	@echo ""
	@echo "Useful commands:"
	@echo "  make kind-logs      - View DaemonSet logs"
	@echo "  make kind-forward   - Port-forward to metrics endpoint"
	@echo "  make kind-delete    - Delete the cluster"
	@echo ""

# Cleanup
kind-clean: kind-delete

# Colors
GREEN = \033[0;32m
YELLOW = \033[1;33m
NC = \033[0m
