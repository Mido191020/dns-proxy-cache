# DNS Request/Response Summary

## Core idea

DNS is not a simple "ask for an IP" function. It is a structured message protocol where both sides exchange a full packet so the receiver can understand exactly what was requested and return a matching reply.

## DNS packet structure

### Header

The header carries the message ID, flags, and counters such as the number of questions and answers. The ID is important because it lets the client match a response to the original request.

### Question

The question section tells the resolver what name is being asked for, which record type is wanted, and which class applies. A common example is:

- Name: `example.com`
- Type: `A`
- Class: `IN`

### Answer

The answer section contains the result. For an `A` record, it usually includes:

- Name
- Type
- Class
- TTL
- Data (the IPv4 address)

## Why the question is repeated in the response

The question is echoed back in the response so the client can confirm that the reply belongs to the same query. This matters when many DNS requests are in flight at the same time.

## End-to-end flow

1. The client wants the IP for a domain such as `example.com`.
2. The domain name is encoded into DNS wire format.
3. A DNS request packet is built with a header and a question.
4. The packet is sent to a resolver over UDP, usually to port 53.
5. The resolver decodes the request, looks up the answer, and builds a response packet.
6. The response contains the original question plus the answer section.
7. The client parses the packet and extracts the IP address from the answer data.

## Wire-format detail

DNS names are encoded as length-prefixed labels:

- `example.com` becomes `07 example 03 com 00`

The IP address is not sent as text. It is returned as raw bytes and then formatted back into dotted notation by the client.

## Big picture

DNS is a message-based system, not a direct function call. The full request/response packet is what makes the protocol reliable, matchable, and extensible.
