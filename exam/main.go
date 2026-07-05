package main

import (
	"bytes"
	"encoding/binary"
	"errors"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"

	"github.com/cilium/ebpf/link"
	"github.com/cilium/ebpf/ringbuf"
	"github.com/cilium/ebpf/rlimit"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promhttp"
)

//go:generate go run github.com/cilium/ebpf/cmd/bpf2go -type http_event -target native httpMonitor ./bpf/http_monitor.c -- -I./bpf

// Exercise 4a: add "protocol" to this metric's label list
var (
	httpRequestsTotal = prometheus.NewCounterVec(
		prometheus.CounterOpts{
			Name: "http_requests_total",
			Help: "Total number of HTTP requests traced by eBPF",
		},
		[]string{"method", "path", "comm"},
	)
)

func init() {
	prometheus.MustRegister(httpRequestsTotal)
}

func main() {
	if err := rlimit.RemoveMemlock(); err != nil {
		log.Fatalf("failed to remove memlock limit: %v", err)
	}

	objs := &httpMonitorObjects{}
	if err := loadHttpMonitorObjects(objs, nil); err != nil {
		log.Fatalf("failed to load eBPF objects: %v", err)
	}
	defer objs.Close()

	// Attach kprobe to tcp_sendmsg
	kpTCP, err := link.Kprobe("tcp_sendmsg", objs.TraceTcpSendmsg, nil)
	if err != nil {
		log.Fatalf("failed to attach TCP kprobe: %v", err)
	}
	defer kpTCP.Close()

	// Exercise 4b: attach a kprobe for objs.TraceUdpSendmsg on "udp_sendmsg" here

	log.Println("HTTP monitor started, tracing HTTP requests...")

	rd, err := ringbuf.NewReader(objs.HttpEvents)
	if err != nil {
		log.Fatalf("failed to create ring buffer reader: %v", err)
	}
	defer rd.Close()

	go func() {
		http.Handle("/metrics", promhttp.Handler())
		log.Println("Serving metrics on :9090/metrics")
		if err := http.ListenAndServe(":9090", nil); err != nil {
			log.Fatalf("failed to serve metrics: %v", err)
		}
	}()

	sig := make(chan os.Signal, 1)
	signal.Notify(sig, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		<-sig
		log.Println("Received signal, shutting down...")
		os.Exit(0)
	}()

	for {
		record, err := rd.Read()
		if err != nil {
			if errors.Is(err, ringbuf.ErrClosed) {
				log.Println("Ring buffer closed")
				return
			}
			log.Printf("failed to read from ring buffer: %v", err)
			continue
		}

		var event httpMonitorHttpEvent
		if err := binary.Read(bytes.NewReader(record.RawSample), binary.LittleEndian, &event); err != nil {
			log.Printf("failed to parse event: %v", err)
			continue
		}

		comm := int8ToString(event.Comm[:])
		// Exercise 4c: extract the protocol field, e.g. int8ToString(event.Protocol[:])
		method := int8ToString(event.Method[:])
		path := int8ToString(event.Path[:])

		if method == "" || path == "" {
			continue
		}

		log.Printf("HTTP request: pid=%d comm=%s method=%s path=%s", event.Pid, comm, method, path)

		// Exercise 4d: add "protocol" to these labels (must match the label list in Exercise 4a)
		httpRequestsTotal.With(prometheus.Labels{
			"method": method,
			"path":   path,
			"comm":   comm,
		}).Inc()
	}
}

func nullTerminatedString(b []byte) string {
	n := bytes.IndexByte(b, 0)
	if n == -1 {
		n = len(b)
	}
	return string(b[:n])
}

func int8ToString(arr []int8) string {
	b := make([]byte, len(arr))
	for i, v := range arr {
		b[i] = byte(v)
	}
	return nullTerminatedString(b)
}
