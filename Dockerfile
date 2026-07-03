FROM scratch

# Copy the statically compiled binary
COPY http-monitor /http-monitor

# Run as non-root where possible (though eBPF requires privileges)
ENTRYPOINT ["/http-monitor"]
