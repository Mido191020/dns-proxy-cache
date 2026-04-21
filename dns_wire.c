#include "dns_wire.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// function to encode domain name e.g. "example.com" -> "\x07example\x03com\x00"
int  encode_dns_name(const char *domain, char *out_buffer) {
    // Write your pointer logic here
    //point to it not call the value
    int len = 0;
    char *start = out_buffer;
    char *out_ptr=out_buffer;
    char *len_ptr = out_ptr;
    out_ptr++;
    const char *in_ptr=domain;
    while (*in_ptr!= '\0'){

        if (*in_ptr =='.'){
            //writing value
            *len_ptr=len;
            len=0;
            len_ptr=out_ptr;
            out_ptr++;

        } else{
           *out_ptr=*in_ptr;
           out_ptr++;
         len++;
        }
        in_ptr++;
    }

    *len_ptr=len;
    *out_ptr=0x00;
    out_ptr++;
    return (int)(out_ptr-start);
}


char *build_dns_query(const char *hostname, size_t *out_len) {
char *buffer=malloc(512);
char *write_ptr=buffer;
    *write_ptr++ = 0xAA; /* Transaction ID part 1 */
    *write_ptr++ = 0xAA; /* Transaction ID part 2 */
    *write_ptr++ = 0x01; /* Flags part 1 */
    *write_ptr++ = 0x00; /* Flags part 2 */
    *write_ptr++ = 0x00; /* Questions part 1 */
    *write_ptr++ = 0x01; /* Questions part 2 */
    *write_ptr++ = 0x00; /* Answer RRs part 1 */
    *write_ptr++ = 0x00; /* Answer RRs part 2 */
    *write_ptr++ = 0x00; /* Authority part 1 */
    *write_ptr++ = 0x00; /* Authority part 2 */
    *write_ptr++ = 0x00; /* Additional part 1 */
    *write_ptr++ = 0x00; /* Additional part 2 */

size_t name_len= encode_dns_name(hostname,write_ptr);
write_ptr+=name_len;
    *write_ptr++=0x00;
    *write_ptr++=0x01;
    *write_ptr++=0x00;
    *write_ptr++=0x01;
    *out_len=write_ptr-buffer;
    return buffer;

}
int parse_dns_reply(const char *buf, size_t len, char *out_ip, size_t ip_len) {
    size_t offset = 12; // Skip DNS header

    // ---- Skip Question Section ----
    while (offset < len && buf[offset] != 0) {
        offset++;
    }
    offset++; // Skip null byte at end of QNAME
    offset += 4; // Skip QTYPE (2 bytes) + QCLASS (2 bytes)

    // ---- Now at Answer Section ----
    // Note: This is a simple parser assuming the first answer is our A record.
    offset += 2; // Skip NAME (usually a 2-byte pointer like C0 0C)

    // Read TYPE (using unsigned char to avoid negative bit-shifting issues)
    unsigned short type = ((unsigned char)buf[offset] << 8) | (unsigned char)buf[offset + 1];
    offset += 2;

    offset += 2; // Skip CLASS
    offset += 4; // Skip TTL

    // Read RDLENGTH
    unsigned short rdlength = ((unsigned char)buf[offset] << 8) | (unsigned char)buf[offset + 1];
    offset += 2;

    // ---- Extract IP if A record ----
    if (type == 1 && rdlength == 4) {
        unsigned char ip1 = buf[offset];
        unsigned char ip2 = buf[offset + 1];
        unsigned char ip3 = buf[offset + 2];
        unsigned char ip4 = buf[offset + 3];

        // Format the IP into the provided string buffer
        snprintf(out_ip, ip_len, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
        return 0; // Success
    }

    return -1; // Failure / Not an A record
}
