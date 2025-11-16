#include "celemetry.h"

celemetry_packet_t *celemetry_new_packet(uint32_t packet_id)
{
    // reserve memory
    uint8_t *data = (uint8_t *)malloc(CELEMETRY_PACKET_LEN);
    if (!data)
    {
        return NULL;
    }
    celemetry_packet_t *packet = (celemetry_packet_t *)malloc(sizeof(celemetry_packet_t));

    // check endianness
    int e = 1;
    if (*((char *)&e) == 1)
    {
        packet->system_endianness = CELEMETRY_LITTLE_ENDIAN;
    }
    else
    {
        packet->system_endianness = CELEMETRY_BIG_ENDIAN;
    }

    // ok, add packet_id field (check endianess)
    data[0] = CELEMETRY_PACKET_NUM;
    // use network byte order
    if (packet->system_endianness == CELEMETRY_LITTLE_ENDIAN)
        packet_id = __bswap_32(packet_id);
    for (int i = 0; i < CELEMETRY_PACKET_NUM_BYTES; i++)
    {
        data[1 + i] = packet_id >> i * 8;
    }

    packet->data = data;
    packet->size = 5;
    return packet;
}

celemetry_packet_t *celemetry_new_packet_from_data(uint8_t *rdata, size_t len)
{
    // reserve memory
    uint8_t *data = (uint8_t *)malloc(CELEMETRY_PACKET_LEN);
    if (!data)
    {
        return NULL;
    }
    celemetry_packet_t *packet = (celemetry_packet_t *)malloc(sizeof(celemetry_packet_t));
    packet->data = data;

    // check endianness
    int e = 1;
    if (*((char *)&e) == 1)
    {
        packet->system_endianness = CELEMETRY_LITTLE_ENDIAN;
    }
    else
    {
        packet->system_endianness = CELEMETRY_BIG_ENDIAN;
    }

    // copy data
    memcpy(packet->data, rdata, len);
    packet->size = len;
    return packet;
}

void celemetry_free(celemetry_packet_t *packet)
{
    free(packet->data);
    free(packet);
}

uint8_t *celemetry_get_data(celemetry_packet_t *packet)
{
    // reserve memory
    uint8_t *data = (uint8_t *)malloc(packet->size);
    if (!data)
    {
        return NULL;
    }
    // copy data
    memcpy(data, packet->data, packet->size);
}

uint8_t celemetry_add_field(celemetry_packet_t *packet, uint8_t field_type, uint8_t field_len, void *field)
{
    // check size
    if (field_len + packet->size > CELEMETRY_PACKET_LEN)
    {
        return CELEMETRY_ERR_MEM;
    }
    // ok, add data
    switch (field_type)
    {
    case CELEMETRY_ID:
    case CELEMETRY_DATE:
    case CELEMETRY_TIME:
    case CELEMETRY_MILLIS:
    case CELEMETRY_LAT:
    case CELEMETRY_LON:
    case CELEMETRY_ALT:
    case CELEMETRY_U32:
    case CELEMETRY_I32:
    case CELEMETRY_F32:
    case CELEMETRY_CRC32:
        // add field type
        packet->data[packet->size] = field_type;
        // add data
        uint32_t field_data32 = *(uint32_t *)field;
        if (packet->system_endianness == CELEMETRY_LITTLE_ENDIAN)
        {
            field_data32 = __bswap_32(field_data32);
        }
        for (int i = 0; i < field_len; i++)
        {
            packet->data[packet->size + 1 + i] = field_data32 >> i * 8;
        }
        packet->size = packet->size + field_len + 1;
        break;
    case CELEMETRY_HDG:
    case CELEMETRY_U16:
    case CELEMETRY_I16:
        // add field type
        packet->data[packet->size] = field_type;
        // add data
        uint16_t field_data16 = *(uint16_t *)field;
        if (packet->system_endianness == CELEMETRY_LITTLE_ENDIAN)
        {
            field_data16 = __bswap_16(field_data16);
        }
        for (int i = 0; i < field_len; i++)
        {
            packet->data[packet->size + 1 + i] = field_data16 >> i * 8;
        }
        packet->size = packet->size + field_len + 1;
        break;
    case CELEMETRY_ARATE:
    case CELEMETRY_U8:
    case CELEMETRY_I8:
        // add field type
        packet->data[packet->size] = field_type;
        // add data
        uint8_t field_data8 = *(uint8_t *)field;
        packet->data[packet->size + 1] = field_data8;
        packet->size = packet->size + field_len + 1;
        break;
    case CELEMETRY_BLOB:
        // add field type
        packet->data[packet->size] = field_type;
        // add blob lenght
        packet->data[1 + packet->size] = field_len;
        // add data as is
        for (int i = 0; i < field_len; i++)
        {
            packet->data[packet->size + 2 + i] = ((uint8_t *)field)[i];
        }
        packet->size = packet->size + field_len + 2;
        break;
    default:
        return CELEMETRY_BAD_DATA;
    }
    return CELEMETRY_OK;
}

uint8_t celemetry_add_crc32(celemetry_packet_t *packet)
{
    // check size
    if (CELEMETRY_CRC32_BYTES + packet->size > CELEMETRY_PACKET_LEN)
    {
        return CELEMETRY_ERR_MEM;
    }
    // ok, add CRC32
    // calculate CRC
    uint32_t crc32 = celemetry_crc32b(packet->data, packet->size);
    if (packet->system_endianness == CELEMETRY_LITTLE_ENDIAN)
    {
        crc32 = __bswap_32(crc32);
    }
    // ok, add field type
    packet->data[packet->size] = CELEMETRY_CRC32;
    // and data
    for (int i = 0; i < CELEMETRY_CRC32_BYTES; i++)
    {
        packet->data[packet->size + 1 + i] = crc32 >> i * 8;
    }
    packet->size = packet->size + CELEMETRY_CRC32_BYTES + 1;
    return CELEMETRY_OK;
}

void *celemetry_get_field(celemetry_packet_t *packet, uint8_t field_type)
{
    // try to find field marker
    uint8_t found = 0;
    uint8_t pos = 0;
    while (!found)
    {
        // check field
        if (packet->data[pos] == field_type)
        {
            found = 1;
            switch (packet->data[pos])
            {
            case CELEMETRY_PACKET_NUM:
            case CELEMETRY_ID:
            case CELEMETRY_DATE:
            case CELEMETRY_TIME:
            case CELEMETRY_MILLIS:
            case CELEMETRY_LAT:
            case CELEMETRY_LON:
            case CELEMETRY_ALT:
            case CELEMETRY_U32:
            case CELEMETRY_I32:
            case CELEMETRY_F32:
            case CELEMETRY_CRC32:
                uint32_t *data32 = (uint32_t *)&packet->data[pos + 1];
                return data32;
                break;
            case CELEMETRY_HDG:
            case CELEMETRY_U16:
            case CELEMETRY_I16:
                uint16_t *data16 = (uint16_t *)&packet->data[pos + 1];
                return data16;
                break;
            case CELEMETRY_ARATE:
            case CELEMETRY_U8:
            case CELEMETRY_I8:
                uint8_t *data8 = &packet->data[pos + 1];
                return data8;
                break;
            case CELEMETRY_BLOB:
                return &packet->data[pos + 1];
            default:
                return NULL;
            }
        }
        else
        {
            switch (packet->data[pos])
            {
            case CELEMETRY_PACKET_NUM:
            case CELEMETRY_ID:
            case CELEMETRY_DATE:
            case CELEMETRY_TIME:
            case CELEMETRY_MILLIS:
            case CELEMETRY_LAT:
            case CELEMETRY_LON:
            case CELEMETRY_ALT:
            case CELEMETRY_U32:
            case CELEMETRY_I32:
            case CELEMETRY_F32:
            case CELEMETRY_CRC32:
                pos = pos + 5;
                break;
            case CELEMETRY_HDG:
            case CELEMETRY_U16:
            case CELEMETRY_I16:
                pos = pos + 3;
                break;
            case CELEMETRY_ARATE:
            case CELEMETRY_U8:
            case CELEMETRY_I8:
                pos = pos + 2;
                break;
            case CELEMETRY_BLOB:
                pos = pos + packet->data[pos + 1] + 1;
                break;
            default:
                return NULL;
            }
        }
    }

    if (!found)
    {
        return NULL;
    }
}

void *celemetry_get_field_number(celemetry_packet_t *packet, uint8_t field_type, uint8_t field_number)
{
    // try to find field marker
    uint8_t found = 0;
    uint8_t pos = 0;
    uint8_t counter = 0;
    while (!found)
    {
        // check field
        if (packet->data[pos] == field_type)
        {
            counter++;
            if (counter == field_number)
            {
                found = 1;
                switch (packet->data[pos])
                {
                case CELEMETRY_PACKET_NUM:
                case CELEMETRY_ID:
                case CELEMETRY_DATE:
                case CELEMETRY_TIME:
                case CELEMETRY_MILLIS:
                case CELEMETRY_LAT:
                case CELEMETRY_LON:
                case CELEMETRY_ALT:
                case CELEMETRY_U32:
                case CELEMETRY_I32:
                case CELEMETRY_F32:
                case CELEMETRY_CRC32:
                    uint32_t *data32 = (uint32_t *)&packet->data[pos + 1];
                    return data32;
                    break;
                case CELEMETRY_HDG:
                case CELEMETRY_U16:
                case CELEMETRY_I16:
                    uint16_t *data16 = (uint16_t *)&packet->data[pos + 1];
                    return data16;
                    break;
                case CELEMETRY_ARATE:
                case CELEMETRY_U8:
                case CELEMETRY_I8:
                    uint8_t *data8 = &packet->data[pos + 1];
                    return data8;
                    break;
                case CELEMETRY_BLOB:
                    return &packet->data[pos + 1];
                default:
                    return NULL;
                }
            }
            else
            {
                switch (packet->data[pos])
                {
                case CELEMETRY_PACKET_NUM:
                case CELEMETRY_ID:
                case CELEMETRY_DATE:
                case CELEMETRY_TIME:
                case CELEMETRY_MILLIS:
                case CELEMETRY_LAT:
                case CELEMETRY_LON:
                case CELEMETRY_ALT:
                case CELEMETRY_U32:
                case CELEMETRY_I32:
                case CELEMETRY_F32:
                case CELEMETRY_CRC32:
                    pos = pos + 5;
                    break;
                case CELEMETRY_HDG:
                case CELEMETRY_U16:
                case CELEMETRY_I16:
                    pos = pos + 3;
                    break;
                case CELEMETRY_ARATE:
                case CELEMETRY_U8:
                case CELEMETRY_I8:
                    pos = pos + 2;
                    break;
                case CELEMETRY_BLOB:
                    pos = pos + packet->data[pos + 1] + 2;
                    break;
                default:
                    return NULL;
                }
            }
        }
        else
        {
            switch (packet->data[pos])
            {
            case CELEMETRY_PACKET_NUM:
            case CELEMETRY_ID:
            case CELEMETRY_DATE:
            case CELEMETRY_TIME:
            case CELEMETRY_MILLIS:
            case CELEMETRY_LAT:
            case CELEMETRY_LON:
            case CELEMETRY_ALT:
            case CELEMETRY_U32:
            case CELEMETRY_I32:
            case CELEMETRY_F32:
            case CELEMETRY_CRC32:
                pos = pos + 5;
                break;
            case CELEMETRY_HDG:
            case CELEMETRY_U16:
            case CELEMETRY_I16:
                pos = pos + 3;
                break;
            case CELEMETRY_ARATE:
            case CELEMETRY_U8:
            case CELEMETRY_I8:
                pos = pos + 2;
                break;
            case CELEMETRY_BLOB:
                pos = pos + packet->data[pos + 1] + 2;
                break;
            default:
                return NULL;
            }
        }
    }

    if (!found)
    {
        return NULL;
    }
}

uint8_t celemetry_get_packet_number(celemetry_packet_t *packet, uint32_t *packet_num)
{
    uint32_t *ppacket_num = (uint32_t *)celemetry_get_field(packet, CELEMETRY_PACKET_NUM);
    if (ppacket_num)
    {
        *packet_num = *ppacket_num;
        if (packet->system_endianness == CELEMETRY_LITTLE_ENDIAN)
        {
            *packet_num = __bswap_32(*packet_num);
        }
        return CELEMETRY_OK;
    }
    else
    {
        return CELEMETRY_ERR_FIELD;
    }
}

uint8_t celemetry_get_date(celemetry_packet_t *packet, char *date)
{
    uint32_t *pdate = (uint32_t *)celemetry_get_field(packet, CELEMETRY_DATE);
    if (pdate)
    {
        uint32_t idate = *pdate;
        if (packet->system_endianness == CELEMETRY_LITTLE_ENDIAN)
        {
            idate = __bswap_32(idate);
        }
        // decode base 40
        celemetry_base40_decode(date, idate);

        return CELEMETRY_OK;
    }
    else
    {
        return CELEMETRY_ERR_FIELD;
    }
}

uint8_t celemetry_get_time(celemetry_packet_t *packet, char *time)
{
    uint32_t *ptime = (uint32_t *)celemetry_get_field(packet, CELEMETRY_TIME);
    if (ptime)
    {
        uint32_t itime = *ptime;
        if (packet->system_endianness == CELEMETRY_LITTLE_ENDIAN)
        {
            itime = __bswap_32(itime);
        }
        // decode base 40
        celemetry_base40_decode(time, itime);

        return CELEMETRY_OK;
    }
    else
    {
        return CELEMETRY_ERR_FIELD;
    }
}

uint8_t celemetry_get_millis(celemetry_packet_t *packet, uint32_t *millis)
{
    uint32_t *pmillis = (uint32_t *)celemetry_get_field(packet, CELEMETRY_MILLIS);
    if (pmillis)
    {
        *millis = *pmillis;
        if (packet->system_endianness == CELEMETRY_LITTLE_ENDIAN)
        {
            *millis = __bswap_32(*millis);
        }
        return CELEMETRY_OK;
    }
    else
    {
        return CELEMETRY_ERR_FIELD;
    }
}

uint8_t celemetry_get_id(celemetry_packet_t *packet, char *id)
{
    uint32_t *pid = (uint32_t *)celemetry_get_field(packet, CELEMETRY_ID);
    if (pid)
    {
        uint32_t iid = *pid;
        if (packet->system_endianness == CELEMETRY_LITTLE_ENDIAN)
        {
            iid = __bswap_32(iid);
        }
        // decode base 40
        celemetry_base40_decode(id, iid);

        return CELEMETRY_OK;
    }
    else
    {
        return CELEMETRY_ERR_FIELD;
    }
}

uint8_t celemetry_get_lat(celemetry_packet_t *packet, float *lat)
{
    uint32_t *plat = (uint32_t *)celemetry_get_field(packet, CELEMETRY_LAT);
    if (plat)
    {
        uint32_t ilat = *plat;
        if (packet->system_endianness == CELEMETRY_LITTLE_ENDIAN)
        {
            ilat = __bswap_32(ilat);
        }
        *lat = (float)ilat;
        return CELEMETRY_OK;
    }
    else
    {
        return CELEMETRY_ERR_FIELD;
    }
}

uint8_t celemetry_get_lon(celemetry_packet_t *packet, float *lon)
{
    uint32_t *plon = (uint32_t *)celemetry_get_field(packet, CELEMETRY_LON);
    if (plon)
    {
        uint32_t ilon = *plon;
        if (packet->system_endianness == CELEMETRY_LITTLE_ENDIAN)
        {
            ilon = __bswap_32(ilon);
        }
        *lon = (float)ilon;
        return CELEMETRY_OK;
    }
    else
    {
        return CELEMETRY_ERR_FIELD;
    }
}

uint8_t celemetry_get_alt(celemetry_packet_t *packet, uint32_t *alt)
{
    uint32_t *palt = (uint32_t *)celemetry_get_field(packet, CELEMETRY_ALT);
    if (palt)
    {
        *alt = *palt;
        if (packet->system_endianness == CELEMETRY_LITTLE_ENDIAN)
        {
            *alt = __bswap_32(*alt);
        }
        return CELEMETRY_OK;
    }
    else
    {
        return CELEMETRY_ERR_FIELD;
    }
}

uint8_t celemetry_get_hdg(celemetry_packet_t *packet, uint16_t *hdg)
{
    uint16_t *phdg = (uint16_t *)celemetry_get_field(packet, CELEMETRY_HDG);
    if (phdg)
    {
        *hdg = *phdg;
        if (packet->system_endianness == CELEMETRY_LITTLE_ENDIAN)
        {
            *hdg = __bswap_16(*hdg);
        }
        return CELEMETRY_OK;
    }
    else
    {
        return CELEMETRY_ERR_FIELD;
    }
}

uint8_t celemetry_get_arate(celemetry_packet_t *packet, int8_t *arate)
{
    int8_t *parate = (int8_t *)celemetry_get_field(packet, CELEMETRY_ARATE);
    if (parate)
    {
        *arate = *parate;
        return CELEMETRY_OK;
    }
    else
    {
        return CELEMETRY_ERR_FIELD;
    }
}

uint8_t celemetry_get_crc32(celemetry_packet_t *packet, uint32_t *crc)
{
    uint32_t *pcrc = (uint32_t *)celemetry_get_field(packet, CELEMETRY_CRC32);
    if (pcrc)
    {
        *crc = *pcrc;
        if (packet->system_endianness == CELEMETRY_LITTLE_ENDIAN)
        {
            *crc = __bswap_32(*crc);
        }
        return CELEMETRY_OK;
    }
    else
    {
        return CELEMETRY_ERR_FIELD;
    }
}

// custom fields
uint8_t celemetry_get_u32(celemetry_packet_t *packet, uint32_t *value, uint8_t number)
{
    uint32_t *pvalue = (uint32_t *)celemetry_get_field_number(packet, CELEMETRY_U32, number);
    if (pvalue)
    {
        *value = *pvalue;
        if (packet->system_endianness == CELEMETRY_LITTLE_ENDIAN)
        {
            *value = __bswap_32(*value);
        }
        return CELEMETRY_OK;
    }
    else
    {
        return CELEMETRY_ERR_FIELD;
    }
}

uint8_t celemetry_get_i32(celemetry_packet_t *packet, int32_t *value, uint8_t number)
{
    int32_t *pvalue = (int32_t *)celemetry_get_field_number(packet, CELEMETRY_I32, number);
    if (pvalue)
    {
        *value = *pvalue;
        if (packet->system_endianness == CELEMETRY_LITTLE_ENDIAN)
        {
            *value = __bswap_32(*value);
        }
        return CELEMETRY_OK;
    }
    else
    {
        return CELEMETRY_ERR_FIELD;
    }
}

size_t celemetry_cobs_encode(uint8_t *data, size_t length, uint8_t *buffer)
{
    uint8_t *encode = buffer;  // Encoded byte pointer
    uint8_t *codep = encode++; // Output code pointer
    uint8_t code = 1;          // Code value

    for (const uint8_t *byte = (const uint8_t *)data; length--; ++byte)
    {
        if (*byte) // Byte not zero, write it
            *encode++ = *byte, ++code;

        if (!*byte || code == 0xff) // Input is zero or block completed, restart
        {
            *codep = code, code = 1, codep = encode;
            if (!*byte || length)
                ++encode;
        }
    }
    *codep = code; // Write final code value

    return (size_t)(encode - buffer);
}

size_t celemetry_cobs_decode(const uint8_t *buffer, size_t length, void *data)
{
    const uint8_t *byte = buffer;      // Encoded input byte pointer
    uint8_t *decode = (uint8_t *)data; // Decoded output byte pointer

    for (uint8_t code = 0xff, block = 0; byte < buffer + length; --block)
    {
        if (block) // Decode block byte
            *decode++ = *byte++;
        else
        {
            block = *byte++;             // Fetch the next block length
            if (block && (code != 0xff)) // Encoded zero, write it unless it's delimiter.
                *decode++ = 0;
            code = block;
            if (!code) // Delimiter code found
                break;
        }
    }

    return (size_t)(decode - (uint8_t *)data);
}

uint32_t celemetry_base40_encode(char *data)
{
    uint32_t x;
    char *c;

    // point c at the end of the data, maximum of 6 characters
    for (x = 0, c = data; x < CELEMETRY_BASE40_FIELD_MAX_CHARS && *c; x++, c++)
        ;

    // encode it backwards
    x = 0;
    for (c--; c >= data; c--)
    {
        x *= 40;
        if (*c >= 'A' && *c <= 'Z')
            x += *c - 'A' + 14;
        else if (*c >= 'a' && *c <= 'z')
            x += *c - 'a' + 14;
        else if (*c >= '0' && *c <= '9')
            x += *c - '0' + 1;
    }

    return (x);
}

char *celemetry_base40_decode(char *data, uint32_t code)
{
    char *c;
    char s;

    *data = '\0';

    // is mission_id valid?
    if (code > 0xF423FFFF)
        return (data);

    for (c = data; code; c++)
    {
        s = code % 40;
        if (s == 0)
            *c = '-';
        else if (s < 11)
            *c = '0' + s - 1;
        else if (s < 14)
            *c = '-';
        else
            *c = 'A' + s - 14;
        code /= 40;
    }
    *c = '\0';

    return (data);
}

// standard crc32b algorithm
uint32_t celemetry_crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;

    for (uint32_t i = 0; i < len; i++)
    {
        uint8_t ch = data[i];
        for (uint32_t j = 0; j < 8; j++)
        {
            uint32_t b = (ch ^ crc) & 1;
            crc >>= 1;
            if (b)
                crc = crc ^ 0xEDB88320;
            ch >>= 1;
        }
    }

    return ~crc;
}