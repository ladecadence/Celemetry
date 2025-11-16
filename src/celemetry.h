#ifndef __CELEMETRY_H
#define __CELEMETRY_H

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


// Base 40
#define CELEMETRY_BASE40_FIELD_MAX_CHARS  6

// network byte order 
#define __bswap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |                \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

#define __bswap_16(x) \
     ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))

#define CELEMETRY_BIG_ENDIAN    1
#define CELEMETRY_LITTLE_ENDIAN 0

// errors
#define CELEMETRY_OK        0
#define CELEMETRY_ERR_MEM   1
#define CELEMETRY_BAD_DATA  2
#define CELEMETRY_ERR_FIELD 3

// packet max len (LoRa)
#define CELEMETRY_PACKET_LEN 255

// known fields
#define CELEMETRY_PACKET_NUM  0x00
#define CELEMETRY_DATE        0x01
#define CELEMETRY_TIME        0x02
#define CELEMETRY_MILLIS      0x03
#define CELEMETRY_ID          0x04
#define CELEMETRY_LAT         0x05
#define CELEMETRY_LON         0x06
#define CELEMETRY_ALT         0x07
#define CELEMETRY_HDG         0x08
#define CELEMETRY_ARATE       0x09
// generic fields
#define CELEMETRY_U8          0x0A
#define CELEMETRY_I8          0x0B
#define CELEMETRY_U16         0x0C
#define CELEMETRY_I16         0x0D
#define CELEMETRY_U32         0x0E
#define CELEMETRY_I32         0x0F
#define CELEMETRY_F32         0x10
#define CELEMETRY_BLOB        0x11

// field sizes
#define CELEMETRY_PACKET_NUM_BYTES  4
#define CELEMETRY_DATE_BYTES        4
#define CELEMETRY_TIME_BYTES        4
#define CELEMETRY_MILLIS_BYTES      4
#define CELEMETRY_ID_BYTES          4
#define CELEMETRY_LAT_BYTES         4
#define CELEMETRY_LON_BYTES         4
#define CELEMETRY_ALT_BYTES         4
#define CELEMETRY_HDG_BYTES         2
#define CELEMETRY_ARATE_BYTES       1
#define CELEMETRY_U8_BYTES          1
#define CELEMETRY_I8_BYTES          1
#define CELEMETRY_U16_BYTES         2
#define CELEMETRY_I16_BYTES         2
#define CELEMETRY_U32_BYTES         4
#define CELEMETRY_I32_BYTES         4
#define CELEMETRY_F32_BYTES         4

typedef struct {
    uint8_t size;
    uint8_t system_endianness;
    uint8_t* data;
} celemetry_packet_t;

celemetry_packet_t* celemetry_new_packet(uint32_t packet_id);
celemetry_packet_t* celemetry_new_packet_from_data(uint8_t* rdata, size_t len);
void celemetry_free(celemetry_packet_t* packet);
uint8_t* celemetry_get_data(celemetry_packet_t* packet);
uint8_t celemetry_add_field(celemetry_packet_t* packet, uint8_t field_type, uint8_t field_len, void* field);
void* celemetry_get_field(celemetry_packet_t* packet, uint8_t field_type);
uint8_t celemetry_get_packet_number(celemetry_packet_t* packet, uint32_t *packet_num);
uint8_t celemetry_get_id(celemetry_packet_t *packet, char* id);
uint8_t celemetry_get_millis(celemetry_packet_t *packet, uint32_t* millis);
uint8_t celemetry_get_lat(celemetry_packet_t *packet, float* lat);
uint8_t celemetry_get_lon(celemetry_packet_t *packet, float* lon);
uint8_t celemetry_get_alt(celemetry_packet_t *packet, uint32_t* alt);
uint8_t celemetry_get_hdg(celemetry_packet_t *packet, uint16_t* hdg);
uint8_t celemetry_get_arate(celemetry_packet_t *packet, int8_t* arate);
uint8_t celemetry_get_u32(celemetry_packet_t *packet, uint32_t *value, uint8_t number);
uint8_t celemetry_get_i32(celemetry_packet_t *packet, int32_t *value, uint8_t number);
size_t celemetry_cobs_encode(uint8_t *data, size_t length, uint8_t *buffer);
size_t celemetry_cobs_decode(const uint8_t *buffer, size_t length, void *data);
uint32_t celemetry_base40_encode(char* data);
char* celemetry_base40_decode(char* data, uint32_t code);

#endif