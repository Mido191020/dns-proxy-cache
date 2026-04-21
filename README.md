# dns-proxy-cache

A custom DNS proxy with in-memory TTL caching, built from scratch in C.

Accepts UDP DNS queries from local clients, checks an in-memory cache, forwards cache misses upstream to `8.8.8.8` over raw UDP, and falls back to TCP when responses are truncated. Written using only POSIX socket APIs -- no libraries, no abstractions.

---

## Why this exists

This project was built as a deliberate deep-dive into three areas of systems programming that are hard to learn from tutorials alone:

- **DNS wire format** -- encoding and decoding binary protocol messages by hand, including label compression, TTL extraction, and flag bit manipulation
- **I/O multiplexing** -- using `select()` to watch multiple UDP and TCP sockets simultaneously in a single-threaded event loop, without blocking
- **Dynamic memory management** -- heap-allocated linked lists for both the TTL cache and the in-flight query table, with explicit ownership, lazy eviction, and Valgrind-verified zero leaks

The goal was not to build the fastest DNS proxy. The goal was to understand exactly what happens between your browser typing a URL and a socket connecting to an IP address.

---

## Architecture

The codebase is split into three modules with hard boundaries. A function either moves bytes over the network, interprets those bytes, or stores data -- never more than one of those at the same time.

```
dns_wire.c / dns_wire.h   -- DNS protocol logic only. No sockets, no cache.
dns_cache.c / dns_cache.h -- In-memory TTL cache only. No sockets, no parsing.
proxy.c                   -- select() event loop. Owns all sockets. Calls into the other two.
```

### Data flow

```
local client (dig / browser)
        |  UDP query
        v
  [proxy.c -- select() loop]
        |
        |- cache hit? ------------------------------> sendto() reply immediately
        |
        \- cache miss
                |
                |- forward via UDP --> 8.8.8.8:53
                |        |
                |        \- TC bit set? --> retry via TCP --> 8.8.8.8:53
                |
                \- on reply: cache entry + sendto() client
```

### Dynamic data structures

**Packet buffers** -- The Trampoline pattern. A 64 KB stack buffer receives each datagram via `recvfrom()`. Only the exact bytes received are `malloc`'d to the heap for further processing. Stack allocation is free; heap allocation is exact.

**TTL cache** -- A heap-allocated singly-linked list of `dns_cache_entry_t` nodes. Entries are inserted on every upstream reply and lazily evicted during lookup when `time(NULL) > expiry`. `cache_destroy()` frees every node on shutdown.

**Pending query table** -- A heap-allocated singly-linked list of `pending_query_t` nodes. Each node holds the forwarded query's DNS ID, the original client's `sockaddr_storage`, and a timestamp for timeout eviction. Nodes are freed when a matching upstream reply arrives or when the timeout fires.

---

## Building

```bash
git clone https://github.com/Mido191020/dns-proxy-cache.git
cd dns-proxy-cache
make
```

Requires a C99 compiler and POSIX sockets. Tested on Linux with GCC.

```bash
# With debug symbols and AddressSanitizer
make debug

# Check for memory errors
valgrind --leak-check=full --track-origins=yes ./proxy
```

---

## Running

```bash
# Start the proxy (binds to 127.0.0.1:5353)
./proxy

# In another terminal -- send a test query
dig @127.0.0.1 -p 5353 example.com A

# Force TCP path (tests truncation fallback)
dig @127.0.0.1 -p 5353 example.com A +tcp

# Send the same query twice -- second should be a cache hit
dig @127.0.0.1 -p 5353 example.com A
dig @127.0.0.1 -p 5353 example.com A
```

To route your system's DNS through the proxy:

```bash
# Linux -- add to /etc/resolv.conf
nameserver 127.0.0.1

# Then observe real browser queries flowing through the proxy log
```

---

## Milestone structure

The project was built milestone by milestone. Each milestone produced a working, testable binary before the next one began.

| Milestone | What it builds | Key concept |
|-----------|---------------|-------------|
| 1 | Standalone DNS query client | DNS wire format encoding, Trampoline pattern, `select()` timeout |
| 2 | UDP relay with pending query list | `select()` over two sockets, heap-allocated linked list, `sockaddr` preservation |
| 3 | In-memory TTL cache | Lazy eviction, linked list node deletion, `time_t` expiry |
| 4 | TCP fallback for truncated responses | TC bit detection, 2-byte length prefix, partial `recv()` loop |
| 5 | Stats, shutdown, Valgrind clean | `SIGINT` handler, `cache_destroy()`, zero leaks verified |

---

## Implementation notes

### DNS label encoding

Hostnames in DNS queries are not dot-separated strings. They are length-prefixed label sequences. `example.com` becomes:

```
07 65 78 61 6D 70 6C 65   <- 7, 'e','x','a','m','p','l','e'
03 63 6F 6D               <- 3, 'c','o','m'
00                        <- end of name
```

The encoder walks the hostname string, reserves a byte for the label length, copies characters until a dot or null terminator, then backfills the length.

### Flags field bit extraction

The DNS header flags field is 16 bits packed with sub-fields. The two flags this proxy cares about:

```c
uint16_t flags = ((uint8_t)buf[2] << 8) | (uint8_t)buf[3];

int is_response  = (flags >> 15) & 1;   // QR bit
int is_truncated = (flags >> 9)  & 1;   // TC bit -- triggers TCP fallback
```

### TCP DNS length prefix

DNS over TCP requires a 2-byte big-endian message length before every message. UDP does not. Forgetting this causes the server to interpret your query bytes as the length field.

```c
uint16_t prefix = htons((uint16_t)query_len);
send(tcp_sock, &prefix, 2, 0);
send(tcp_sock, query_buf, query_len, 0);
```

TCP `recv()` can return partial data. The receive loop must accumulate bytes until the full message length is received.

### Memory ownership convention

Every `malloc()` in this codebase has a comment marking its owner:

```c
char *packet = malloc(n);  /* OWNER: caller of receive_packet() */
```

The rule: free on every exit path, not just the happy path. Verified with Valgrind after every milestone.

---

## Debugging

```bash
# Watch all DNS traffic on the wire
sudo tcpdump -i any '(udp port 53 or udp port 5353)' -X

# Watch only upstream traffic
sudo tcpdump -i any host 8.8.8.8 -X

# Trace every syscall the proxy makes
strace ./proxy

# Full memory error check
valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all ./proxy
```

A milestone is not complete until `dig` returns a correct answer and Valgrind reports zero errors.

---

## What I learned

- How DNS actually works at the byte level -- not the concept, the wire format
- Why `select()` modifies your `fd_set` and `timeval` in place and why you must rebuild both every loop iteration
- Why TCP DNS needs a 2-byte length prefix and UDP does not -- the difference between stream and datagram semantics
- How to safely delete a node from a singly-linked list without losing the tail or causing a use-after-free
- How the Trampoline pattern combines stack speed with heap precision for buffer allocation
- Why "free on every exit path" is a discipline, not an afterthought

---

## References

- [RFC 1035 -- Domain Names: Implementation and Specification](https://tools.ietf.org/html/rfc1035)
- [RFC 1034 -- Domain Names: Concepts and Facilities](https://tools.ietf.org/html/rfc1034)
- *Hands-On Network Programming with C* -- Lewis Van Winkle (Packt)
- `man 2 select`, `man 2 recvfrom`, `man 2 sendto`, `man 3 getaddrinfo`, `man 3 inet_ntop`
