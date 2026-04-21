#include "dns_wire.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// function to encode domain name e.g. "example.com" -> "\x07example\x03com\x00"
int encode_dns_name(const char *domain, unsigned char *out_buffer) {
    // 1. Validate inputs
    if (domain == NULL || out_buffer == NULL) {
        return -1;
    }

    const char *in_ptr = domain;
    unsigned char *out_ptr = out_buffer;
    unsigned char *start = out_buffer;

    uint8_t len = 0;
    unsigned char *len_ptr = out_ptr;

    out_ptr++; // reserve space for first label length

    // 2. Encode labels
    while (*in_ptr != '\0') {

        if (*in_ptr == '.') {
            // validate label length
            if (len == 0 || len > 63) {
                return -1;
            }

            *len_ptr = len;

            len = 0;
            len_ptr = out_ptr;
            out_ptr++; // reserve next length byte

        } else {
            if (len >= 63) {
                return -1;
            }

            *out_ptr++ = (unsigned char)*in_ptr;
            len++;
        }

        in_ptr++;
    }

    // 3. Final label validation
    if (len == 0 || len > 63) {
        return -1;
    }

    *len_ptr = len;

    // 4. Null terminator
    *out_ptr++ = 0x00;

    // 5. Return total bytes written
    return (int)(out_ptr - start);
}


