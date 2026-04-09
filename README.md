# dns-proxy-cache

## Project Description

**dns-proxy-cache** is a custom DNS proxy cache written entirely from scratch in C using POSIX sockets. It acts as a local DNS resolver that listens for UDP queries on `localhost`, checks an in-memory TTL cache for a hit, forwards misses upstream to public resolvers such as `8.8.8.8`, and seamlessly falls back to TCP when responses are truncated.

The architecture is intentionally split into three separate concerns:

- **Event loop**: `select()`-based socket multiplexing and request dispatch
- **Wire format parsing**: DNS message encoding/decoding and flag handling
- **Cache management**: TTL-aware lookup, insertion, and eviction

This separation keeps networking, protocol logic, and state management isolated and maintainable.

## Milestone Roadmap

- [ ] **Milestone 1: Raw DNS Client** — Encoding/decoding wire format, querying upstream via UDP
- [ ] **Milestone 2: UDP Relay** — Binding local listener, multiplexing local and upstream sockets
- [ ] **Milestone 3: In-Memory TTL Cache** — Struct design, TTL tracking, cache insertion/eviction
- [ ] **Milestone 4: TCP Fallback** — Detecting the TC flag, re-querying over TCP with 2-byte length prefixes
- [ ] **Milestone 5: Polish & Telemetry** — Logging, cache hit/miss stats, graceful shutdown

## Build Instructions

### How to Build and Run

Use standard `gcc` or `make` commands once the source files are in place:

```bash
gcc -o dns-proxy-cache main.c dns_wire.c dns_cache.c proxy.c
./dns-proxy-cache
```

Or, if you prefer `make`:

```bash
make
./dns-proxy-cache
```

