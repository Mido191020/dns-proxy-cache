# Milestone 1 — DNS Wire Engine

## Goal

Build a standalone `dns_wire.c` module that:

- encodes a DNS A-record query using the trampoline pattern
- decodes the response
- leaves network I/O to the caller
- handles ownership cleanly on every exit path

## Todo list

- [ ] Implement `build_dns_query(hostname, out_buf, out_len)`
- [ ] Build the 12-byte DNS header plus question section in a stack buffer
- [ ] Copy the finished query into a heap buffer and return it to the caller
- [ ] Open a UDP socket and send the query to `8.8.8.8:53`
- [ ] Free the query buffer immediately after `sendto()` is done
- [ ] Wait for a reply with `select()` and a 3-second timeout
- [ ] Print `timeout` and exit if no data arrives
- [ ] Implement `parse_dns_reply(buf, len, out_ip)`
- [ ] Skip the DNS header and question section while parsing
- [ ] Read the first A-record RDATA and format it with `inet_ntop()`
- [ ] Trace every caller exit path and fix any leak risk

## Primary Socratic question

`build_dns_query()` returns a heap buffer. The caller sends it with `sendto()` and then frees it. What happens if `sendto()` fails? Trace every exit path and identify where the buffer would leak if cleanup is missed.
