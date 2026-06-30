.PHONY: all generate build clean

# Build for the current architecture
all: generate build

# Generate eBPF bytecode and Go bindings
generate:
	go generate ./...

# Build the Go binary
build:
	CGO_ENABLED=0 go build -o http-monitor .

# Clean build artifacts
clean:
	rm -f http-monitor
	rm -f http_monitor_bpfel.go http_monitor_bpfel.o
	rm -f http_monitor_bpfeb.go http_monitor_bpfeb.o

# Run the monitor (requires root)
run: build
	sudo ./http-monitor
