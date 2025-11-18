Simple telemetry format that can be used with low resource platforms like 8-bit microcontrollers and low speed radio links.

# Data format

Packet data is a byte array with a varible length of bytes per data field, length of each field is encoded using a field marker byte before each field. Standard telemetry fields are already defined and also custom data fields can be used, 32, 16 and 8-bit signed or unsigned integers, 32-bit floats and custom binary data (blob) (blobs are encoded with a byte after field marker to store their lenght). To avoid endiannes problems, the library uses network byte order (big-endian) to store and transmit data.
Also the library provides functions to use Consistent Overhead Byte Stuffing (COBS) to encode the data without any 0 bytes, so 0 can be used as packet end marker. 

# Fields

The library defines some usual telemetry fields:
* 0x00: Packet number     - uint32
* 0x01: Packet type       - uint8: idientifies different packet types
* 0x02: Date              - uint32: date is stored in the format DDMMYY, like 031125 for 3th of November, 2025
* 0x03: Time              - uint32: time is stored in the format HHMMSS, like 153205 for 15:32:05
* 0x04: Milliseconds      - uint32 
* 0x05: Mission ID        - uint32: Char array of max 6 characters, stored as a base40 encoded number
* 0x06: Latitude          - float32: Decimal latitude
* 0x07: Longitude         - float32: Decimal longitude
* 0x08: Altitude          - uint32
* 0x09: Heading           - uint16: 0 - 359º
* 0x0A: Ascension rate    - int8: -127 - 128 m/s
* 0x0B: CRC32             - uint32: optional CRC32 calculation of all bytes in the packet (less the CRC32 field itself)
* 0x0C: SSDV              - 128 bytes: SSDV packets are 256 bytes by default, so they must be encoded using 128bytes packet lenght.

Also you can encode custom data fields with the field markers:
* 0x20: uint8
* 0x21: int8
* 0x22: uint16
* 0x23: int16
* 0x24: uint32
* 0x25: int32
* 0x26: float32
* 0x27: Blob: Custom lenght data array.

## Library use example
```c
    // create new packet
    celemetry_packet_t *packet = celemetry_new_packet(1234);

    if (packet) { 
        // add data
        char* MISSION = "EKI2";
        uint32_t mission_bytes = celemetry_base40_encode(MISSION);
        celemetry_add_field(packet, CELEMETRY_ID, CELEMETRY_ID_BYTES, &mission_bytes);

        int16_t hdg = 100;
        celemetry_add_field(packet, CELEMETRY_HDG, CELEMETRY_HDG_BYTES, &hdg);

        uint32_t data1 = 7890;
        celemetry_add_field(packet, CELEMETRY_U32, CELEMETRY_U32_BYTES, &data1);

        uint8_t data[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15};
        celemetry_add_field(packet, CELEMETRY_BLOB, sizeof(data), data);

        uint32_t data2 = 3456;
        celemetry_add_field(packet, CELEMETRY_U32, CELEMETRY_U32_BYTES, &data2);

        // include CRC32 calculation
        celemetry_add_crc32(packet);

        // encode it for transmission
        uint8_t coded[CELEMETRY_PACKET_LEN];
        size_t enc_size = celemetry_cobs_encode(packet->data, packet->size, coded);

        // decode it and create new packet
        uint8_t decoded[CELEMETRY_PACKET_LEN];
        size_t dec_size = celemetry_cobs_decode(coded, enc_size, decoded);
        celemetry_packet_t *packet2 = celemetry_new_packet_from_data(decoded, dec_size);

        if (packet2) {
            // ok, first check CRC32
            if (celemetry_check_crc32(packet2) == CELEMETRY_OK) {
                printf("CRC32 correct!\n");
            } else {
                printf("CRC32 incorrect!\n");
            }

            // and get fields
            uint32_t packet_num;
            if (celemetry_get_packet_number(packet, &packet_num) == CELEMETRY_OK) {
                printf("Packet num: %d\n", packet_num);
            }

            char id[6];
            if (celemetry_get_id(packet, id) == CELEMETRY_OK) {
                printf("ID: %s\n", id);
            }

            int16_t heading;
            if (celemetry_get_hdg(packet, &heading) == CELEMETRY_OK) {
                printf("Heading: %d\n", heading);
            }

            uint32_t d2;
            if (celemetry_get_u32(packet, &d2, 2) == CELEMETRY_OK) {
                printf("Data2: %d\n", d2);
            }

            celemetry_free(packet2);
        }
        celemetry_free(packet);
    }

```

Complete examples of basic packet manipulation and SSDV packets can be found on /tests 