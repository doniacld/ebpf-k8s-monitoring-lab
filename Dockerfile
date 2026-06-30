FROM golang:1.23 AS builder

WORKDIR /workspace

# Copy go mod files
COPY go.mod go.sum ./
RUN go mod download

# Copy source
COPY . .

# Generate and build
RUN apt-get update && apt-get install -y clang llvm libbpf-dev
RUN make generate
RUN make build

# Final image
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y ca-certificates && rm -rf /var/lib/apt/lists/*

COPY --from=builder /workspace/http-monitor /usr/local/bin/http-monitor

ENTRYPOINT ["/usr/local/bin/http-monitor"]
