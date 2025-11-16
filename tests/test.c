#include <stdio.h>
#include "../src/celemetry.h"

void main(void)
{
    // create new packet
    celemetry_packet_t *packet = celemetry_new_packet(1234);

    if (packet)
    {
        // add data
        char *MISSION = "EKI2";
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

        // print it
        printf("Packet:  ");
        for (size_t i = 0; i < packet->size; i++)
        {
            printf("%.2x", packet->data[i]);
        }
        printf("\n");

        // encode it for transmission
        uint8_t coded[CELEMETRY_PACKET_LEN];
        size_t enc_size = celemetry_cobs_encode(packet->data, packet->size, coded);
        // print it
        printf("Encoded: ");
        for (size_t i = 0; i < enc_size; i++)
        {
            printf("%.2x", coded[i]);
        }
        printf("\n");

        // decode it and create new packet
        uint8_t decoded[CELEMETRY_PACKET_LEN];
        size_t dec_size = celemetry_cobs_decode(coded, enc_size, decoded);
        celemetry_packet_t *packet2 = celemetry_new_packet_from_data(decoded, dec_size);
        if (packet2)
        {
            // print it
            printf("Decoded: ");
            for (size_t i = 0; i < packet2->size; i++)
            {
                printf("%.2x", packet2->data[i]);
            }
            printf("\n");

            // ok, get fields
            uint32_t packet_num;
            if (celemetry_get_packet_number(packet, &packet_num) == CELEMETRY_OK)
            {
                printf("Packet num: %d\n", packet_num);
            }

            char id[6];
            if (celemetry_get_id(packet, id) == CELEMETRY_OK)
            {
                printf("ID: %s\n", id);
            }

            int16_t heading;
            if (celemetry_get_hdg(packet, &heading) == CELEMETRY_OK)
            {
                printf("Heading: %d\n", heading);
            }

            uint32_t d2;
            if (celemetry_get_u32(packet, &d2, 2) == CELEMETRY_OK)
            {
                printf("Data2: %d\n", d2);
            }
            celemetry_free(packet2);
        }
        celemetry_free(packet);
    }
}