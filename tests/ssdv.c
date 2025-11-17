#include "../src/celemetry.h"
#include <stdio.h>

const char* ssdv_file = "tests/data/ssdv.bin";
const char* ssdv_out_file = "tests/data/ssdvout.bin";
uint8_t buffer[256];
uint8_t* ssdvbuf;
uint8_t ssdvpart;

void main(void) {
    // create two packets
    celemetry_packet_t *packet1 = celemetry_new_packet(1000);
    celemetry_packet_t *packet2 = celemetry_new_packet(1001);

    if (packet1 && packet2) {
        // open ssdv file and read buffer
        FILE* f = fopen(ssdv_file, "rb");
        if (f) {
            fread(buffer, 1, 256, f);
            fclose(f);
        }
        // ok, now we need to split SSDV data in the two packets
        celemetry_add_ssdv(packet1, 0, buffer);
        celemetry_add_ssdv(packet2, 1, buffer+128);

        // now we need to get both parts
        if (celemetry_get_ssdv(packet1, &ssdvpart, &ssdvbuf)==CELEMETRY_OK) {
            FILE* fo = fopen(ssdv_out_file, "ab");
            fwrite(ssdvbuf, 1, 128, fo);
            fclose(fo);
        }
        if (celemetry_get_ssdv(packet2, &ssdvpart, &ssdvbuf)==CELEMETRY_OK) {
            FILE* fo = fopen(ssdv_out_file, "ab");
            fwrite(ssdvbuf, 1, 128, fo);
            fclose(fo);
        }

        // ok, free memory
        celemetry_free(packet1);
        celemetry_free(packet2);
    }
}
