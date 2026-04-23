#include "dns_wire.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int encode_dns_name(const char *domain, unsigned char *out_buffer) {
    if (domain == NULL || out_buffer == NULL) {
        return -1;
    }

        if (*in_ptr =='.'){
            //writing value
            *len_ptr=len;
            len=0;
            len_ptr=out_ptr;
            out_ptr++;

    uint8_t len = 0;
    unsigned char *len_ptr = out_ptr++;

    while (*in_ptr != '\0') {

        if (*in_ptr == '.') {
            if (len == 0 || len > 63) return -1;

            *len_ptr = len;
            len = 0;
            len_ptr = out_ptr++;
        } else {
            if (len >= 63) return -1;

            *out_ptr++ = (unsigned char)*in_ptr;
            len++;
        }
        in_ptr++;
    }

    if (len == 0 || len > 63) return -1;

    *len_ptr = len;
    *out_ptr++ = 0x00;

    return (int)(out_ptr - start);
}

char *build_dns_query(const char *hostname, size_t *out_len) {

    if (!hostname || !out_len) return NULL;

    char *buffer = malloc(512);
    if (!buffer) return NULL;

    char *write_ptr = buffer;

    // ===== DNS HEADER =====
    *write_ptr++ = 0xAA;
    *write_ptr++ = 0xAA;

    *write_ptr++ = 0x01;
    *write_ptr++ = 0x00;

    *write_ptr++ = 0x00;
    *write_ptr++ = 0x01;

    *write_ptr++ = 0x00;
    *write_ptr++ = 0x00;

    *write_ptr++ = 0x00;
    *write_ptr++ = 0x00;

    *write_ptr++ = 0x00;
    *write_ptr++ = 0x00;

    // ===== QUESTION SECTION =====
    int name_len = encode_dns_name(hostname, (unsigned char *)write_ptr);
    if (name_len < 0) {
        free(buffer);
        return NULL;
    }

    write_ptr += name_len;

    // QTYPE = A
    *write_ptr++ = 0x00;
    *write_ptr++ = 0x01;

    // QCLASS = IN
    *write_ptr++ = 0x00;
    *write_ptr++ = 0x01;

    *out_len = write_ptr - buffer;

    return buffer;
}

int parse_dns_reply(const char *buf, size_t len, char *out_ip, size_t ip_len) {

    if (!buf || !out_ip || ip_len < 16) return -1;

    size_t offset = 12; // skip header

    // ===== Skip QUESTION NAME =====
    while (offset < len) {

        unsigned char byte = buf[offset];

        // compression pointer
        if ((byte & 0xC0) == 0xC0) {
            if (offset + 1 >= len) return -1;
            offset += 2;
            break;
        }

        // end of name
        if (byte == 0) {
            offset += 1;
            break;
        }

        offset += (byte + 1);

        if (offset > len) return -1;
    }

    // ===== FIXED: skip FULL QUESTION SECTION =====
    // (QTYPE + QCLASS = 4 bytes)
    if (offset + 4 > len) return -1;
    offset += 4;

    // ===== ANSWER NAME (pointer) =====
    if (offset + 2 > len) return -1;
    offset += 2;

    // TYPE
    unsigned short type =
            ((unsigned char)buf[offset] << 8) |
            (unsigned char)buf[offset + 1];
    offset += 2;

    // CLASS
    offset += 2;

    // TTL
    offset += 4;

    // RDLENGTH
    unsigned short rdlength =
            ((unsigned char)buf[offset] << 8) |
            (unsigned char)buf[offset + 1];
    offset += 2;

    if (offset + rdlength > len) return -1;

    // ===== A RECORD =====
    if (type == 1 && rdlength == 4) {

        unsigned char ip1 = buf[offset];
        unsigned char ip2 = buf[offset + 1];
        unsigned char ip3 = buf[offset + 2];
        unsigned char ip4 = buf[offset + 3];

        snprintf(out_ip, ip_len, "%u.%u.%u.%u",
                 ip1, ip2, ip3, ip4);

        return 0;
    }

    return -1;
}