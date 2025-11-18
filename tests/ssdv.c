#include "../src/celemetry.h"
#include <stdio.h>

const char* ssdv_file = "tests/data/ssdv.bin";
const char* ssdv_out_file = "tests/data/ssdvout.bin";
uint8_t buffer[256];
uint8_t* ssdvbuf;
uint8_t ssdvpart;

int main(void) {
    // create two packets
    celemetry_packet_t *packet1 = celemetry_new_packet(1000);
    celemetry_packet_t *packet2 = celemetry_new_packet(1001);

    if (packet1 && packet2) {
        // open ssdv file and read buffer (two packets)
        FILE* f = fopen(ssdv_file, "rb");
        if (f) {
            fread(buffer, 1, 256, f);
            fclose(f);
        } else {
            printf("Can't open ssdv file!\n");
            celemetry_free(packet1);
            celemetry_free(packet2);
            exit(1);
        }
        // ok, now we need to split SSDV data in the two packets
        celemetry_add_field(packet1, CELEMETRY_SSDV, CELEMETRY_SSDV_BYTES, buffer);
        celemetry_add_field(packet2, CELEMETRY_SSDV, CELEMETRY_SSDV_BYTES, buffer+128);

        // now we need to get both parts
        if (celemetry_get_ssdv(packet1, &ssdvbuf)==CELEMETRY_OK) {
            FILE* fo = fopen(ssdv_out_file, "ab");
            fwrite(ssdvbuf, 1, 128, fo);
            fclose(fo);
            printf("Written first packet\n");
        } else {
            printf("Can't find SSDV field in packet 1\n");
        }
        if (celemetry_get_ssdv(packet2, &ssdvbuf)==CELEMETRY_OK) {
            FILE* fo = fopen(ssdv_out_file, "ab");
            fwrite(ssdvbuf, 1, 128, fo);
            fclose(fo);
            printf("Written second packet\n");
        } else {
            printf("Can't find SSDV field in packet 2\n");
        }

        // ok, free memory
        celemetry_free(packet1);
        celemetry_free(packet2);
    } else {
        printf("Can't create packets!\n");
        exit(1);
    }

    return 0;
}
