/*
 * crsf.c — Implementation de la librairie CRSF.
 * Voir crsf.h pour la doc des fonctions publiques.
 */

#include "Modules/crsf.h"
#include "Modules/crsf_command.h"
#include <string.h>
#include <stdio.h>
#include "usbd_cdc_if.h"
#include <math.h>

/* ==================== CRC8 (poly 0xD5, DVB-S2) ====================
 * Table reprise telle quelle de la spec officielle TBS.
 */
static const uint8_t crsf_crc8_table[256] = {
    0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
    0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06, 0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
    0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
    0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2, 0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
    0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9, 0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
    0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
    0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
    0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F, 0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
    0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
    0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
    0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F, 0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
    0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
    0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
    0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74, 0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
    0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
    0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0, 0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9
};

uint8_t crsf_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc = crsf_crc8_table[crc ^ data[i]];
    }
    return crc;
}

/* ==================== Helpers lecture/ecriture big-endian ==================== */

static inline uint16_t rd_u16(const uint8_t *p) { return (uint16_t)(((uint16_t)p[0] << 8) | p[1]); }
static inline int16_t  rd_i16(const uint8_t *p) { return (int16_t)rd_u16(p); }
static inline uint32_t rd_u24(const uint8_t *p) { return ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2]; }
static inline uint32_t rd_u32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}
static inline int32_t  rd_i32(const uint8_t *p) { return (int32_t)rd_u32(p); }

static inline void wr_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static inline void wr_i16(uint8_t *p, int16_t v)  { wr_u16(p, (uint16_t)v); }
static inline void wr_u24(uint8_t *p, uint32_t v) { p[0] = (uint8_t)(v >> 16); p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)v; }
static inline void wr_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}
static inline void wr_i32(uint8_t *p, int32_t v) { wr_u32(p, (uint32_t)v); }

/* ==================== Parser ==================== */

void crsf_parser_init(crsf_parser_t *parser)
{
    parser->state = CRSF_PARSE_WAIT_SYNC;
    parser->frame_len = 0;
    parser->index = 0;
}

bool crsf_parser_process_byte(crsf_parser_t *parser, uint8_t byte, crsf_frame_t *out_frame)
{
    switch (parser->state) {

    case CRSF_PARSE_WAIT_SYNC:
        if (byte == CRSF_SYNC_BYTE) {
            parser->state = CRSF_PARSE_WAIT_LEN;
        }
        return false;

    case CRSF_PARSE_WAIT_LEN:
        if (byte < CRSF_FRAME_LENGTH_MIN || byte > CRSF_FRAME_LENGTH_MAX) {
            /* longueur hors plage valide -> on abandonne et on resynchronise */
            parser->state = CRSF_PARSE_WAIT_SYNC;
            return false;
        }
        parser->frame_len = byte;
        parser->index = 0;
        parser->state = CRSF_PARSE_WAIT_DATA;
        return false;

    case CRSF_PARSE_WAIT_DATA:
        parser->buffer[parser->index++] = byte;

        if (parser->index < parser->frame_len) {
            return false;
        }

        /* frame complete : buffer[0 .. frame_len-1] = type + payload + crc */
        parser->state = CRSF_PARSE_WAIT_SYNC;

        {
            uint8_t crc_received = parser->buffer[parser->frame_len - 1];
            uint8_t crc_computed = crsf_crc8(parser->buffer, (uint8_t)(parser->frame_len - 1));

            if (crc_received != crc_computed) {
                return false; /* frame corrompue, on l'ignore */
            }

            uint8_t type = parser->buffer[0];
            out_frame->type = type;

            if (type >= 0x28) {
                /* en-tete etendu : type, dest, orig, payload..., crc */
                if (parser->frame_len < 4) {
                    return false; /* trop court pour contenir dest+orig+crc */
                }
                out_frame->has_extended_header = true;
                out_frame->dest_addr = parser->buffer[1];
                out_frame->orig_addr = parser->buffer[2];
                out_frame->payload_len = (uint8_t)(parser->frame_len - 1 /*type*/ - 2 /*dest+orig*/ - 1 /*crc*/);
                memcpy(out_frame->payload, &parser->buffer[3], out_frame->payload_len);
            } else {
                /* en-tete court : type, payload..., crc */
                out_frame->has_extended_header = false;
                out_frame->dest_addr = 0;
                out_frame->orig_addr = 0;
                out_frame->payload_len = (uint8_t)(parser->frame_len - 1 /*type*/ - 1 /*crc*/);
                memcpy(out_frame->payload, &parser->buffer[1], out_frame->payload_len);
            }
        }
        return true;

    default:
        parser->state = CRSF_PARSE_WAIT_SYNC;
        return false;
    }
}

void crsf_parser_process_buffer(crsf_parser_t *parser, const uint8_t *data, size_t len,
                                 crsf_frame_callback_t callback, void *user_ctx)
{
    crsf_frame_t frame;
    for (size_t i = 0; i < len; i++) {
        if (crsf_parser_process_byte(parser, data[i], &frame)) {
            if (callback != NULL) {
                callback(&frame, user_ctx);
            }
        }
    }
}

/* ==================== Decodage ==================== */

bool crsf_decode_gps(const crsf_frame_t *frame, crsf_gps_t *out)
{
    if (frame->type != CRSF_FT_GPS || frame->payload_len < 15) return false;
    const uint8_t *p = frame->payload;
    out->latitude     = rd_i32(&p[0]);
    out->longitude    = rd_i32(&p[4]);
    out->groundspeed  = rd_u16(&p[8]);
    out->heading      = rd_u16(&p[10]);
    out->altitude_raw = rd_u16(&p[12]);
    out->satellites   = p[14];
    return true;
}

bool crsf_decode_vario(const crsf_frame_t *frame, crsf_vario_t *out)
{
    if (frame->type != CRSF_FT_VARIO || frame->payload_len < 2) return false;
    out->vertical_speed_cm_s = rd_i16(frame->payload);
    return true;
}

bool crsf_decode_battery(const crsf_frame_t *frame, crsf_battery_t *out)
{
    if (frame->type != CRSF_FT_BATTERY_SENSOR || frame->payload_len < 8) return false;
    const uint8_t *p = frame->payload;
    int16_t  raw_v = rd_i16(&p[0]);   /* decivolts */
    int16_t  raw_i = rd_i16(&p[2]);   /* deciamps */
    out->voltage_v         = raw_v * 0.1f;
    out->current_a         = raw_i * 0.1f;
    out->capacity_used_mah = rd_u24(&p[4]);
    out->remaining_pct     = p[7];
    return true;
}

bool crsf_decode_baro_altitude(const crsf_frame_t *frame, crsf_baro_altitude_t *out)
{
    if (frame->type != CRSF_FT_BARO_ALTITUDE || frame->payload_len < 2) return false;
    const uint8_t *p = frame->payload;

    uint16_t packed = rd_u16(&p[0]);
    int32_t altitude_dm;
    if (packed & 0x8000) {
        altitude_dm = (int32_t)(packed & 0x7FFF) * 10; /* metres -> dm */
    } else {
        altitude_dm = (int32_t)packed - 10000;          /* deja en dm, offset -1000m */
    }
    out->altitude_m = altitude_dm * 0.1f;

    if (frame->payload_len >= 3) {
        int8_t vspeed_packed = (int8_t)p[2];
        const float Kl = 100.0f;
        const float Kr = 0.026f;
        float sign = (vspeed_packed < 0) ? -1.0f : 1.0f;
        float mag = fabsf((float)vspeed_packed) * Kr;
        out->vertical_speed_m_s = sign * (expf(mag) - 1.0f) * Kl / 100.0f; /* cm/s -> m/s */
        out->has_vertical_speed = true;
    } else {
        out->vertical_speed_m_s = 0.0f;
        out->has_vertical_speed = false;
    }
    return true;
}

bool crsf_decode_attitude(const crsf_frame_t *frame, crsf_attitude_t *out)
{
    if (frame->type != CRSF_FT_ATTITUDE || frame->payload_len < 6) return false;
    const uint8_t *p = frame->payload;
    const float LSB = 0.0001f; /* 100 microradians */
    out->pitch_rad = rd_i16(&p[0]) * LSB;
    out->roll_rad  = rd_i16(&p[2]) * LSB;
    out->yaw_rad   = rd_i16(&p[4]) * LSB;
    return true;
}

bool crsf_decode_link_statistics(const crsf_frame_t *frame, crsf_link_statistics_t *out)
{
    if (frame->type != CRSF_FT_LINK_STATISTICS || frame->payload_len < 10) return false;
    const uint8_t *p = frame->payload;
    out->up_rssi_ant1     = p[0];
    out->up_rssi_ant2     = p[1];
    out->up_link_quality  = p[2];
    out->up_snr           = (int8_t)p[3];
    out->active_antenna   = p[4];
    out->rf_profile       = p[5];
    out->up_rf_power      = p[6];
    out->down_rssi        = p[7];
    out->down_link_quality= p[8];
    out->down_snr         = (int8_t)p[9];
    return true;
}

bool crsf_decode_rc_channels(const crsf_frame_t *frame, crsf_rc_channels_t *out)
{
    if (frame->type != CRSF_FT_RC_CHANNELS_PACKED || frame->payload_len < 22) return false;
    const uint8_t *p = frame->payload;
    uint32_t bitbuf = 0;
    int bitcount = 0;
    int byte_idx = 0;

    for (int ch = 0; ch < 16; ch++) {
        while (bitcount < 11) {
            bitbuf |= ((uint32_t)p[byte_idx++]) << bitcount;
            bitcount += 8;
        }
        out->channels[ch] = (uint16_t)(bitbuf & 0x7FF);
        bitbuf >>= 11;
        bitcount -= 11;
    }
    return true;
}

bool crsf_decode_heartbeat(const crsf_frame_t *frame, crsf_heartbeat_t *out)
{
    if (frame->type != CRSF_FT_HEARTBEAT || frame->payload_len < 2) return false;
    out->origin_address = rd_i16(frame->payload);
    return true;
}

bool crsf_decode_command(const crsf_frame_t *frame, crsf_cmd_frame_t *out)
{
    if (!frame || !out) return false;
    if (frame->type != CRSF_FT_COMMAND || frame->payload_len < 2) return false;
    const uint8_t *p = frame->payload;

    // Le Command_CRC8 (poly 0xBA) couvre : Type(0x32) + Destination + Origin + Command_ID + Payload
    // (Type/Dest/Orig ont déjà été retirés par le parser -> il faut les remettre devant)
    uint8_t data_len = (uint8_t)(frame->payload_len - 1); // tout sauf le dernier octet (CRC)
    uint8_t crc_buf[3 + CRSF_MAX_PAYLOAD_SIZE];
    if ((size_t)(3 + data_len) > sizeof(crc_buf)) return false;

    crc_buf[0] = frame->type;
    crc_buf[1] = frame->dest_addr;
    crc_buf[2] = frame->orig_addr;
    memcpy(&crc_buf[3], p, data_len);

    uint8_t crc_received = p[frame->payload_len - 1];
    uint8_t crc_computed = command_crc8(crc_buf, (uint8_t)(3 + data_len));

//    static char dbg[128];
//    int dlen = snprintf(
//        dbg, sizeof(dbg),
//        "payload_len=%u dest=0x%02X orig=0x%02X crc_recv=0x%02X crc_calc=0x%02X data:",
//        (unsigned)frame->payload_len,
//        frame->dest_addr, frame->orig_addr,
//        crc_received, crc_computed
//    );
//    if (dlen < 0) dlen = 0;
//    if (dlen > (int)sizeof(dbg)) dlen = (int)sizeof(dbg);
//
//    for (uint8_t i = 0; i < frame->payload_len && dlen < (int)sizeof(dbg) - 4; i++) {
//        int n = snprintf(dbg + dlen, sizeof(dbg) - dlen, " %02X", p[i]);
//        if (n < 0) break;
//        dlen += n;
//        if (dlen > (int)sizeof(dbg)) dlen = (int)sizeof(dbg);
//    }
//    if (dlen < (int)sizeof(dbg) - 2) {
//        dbg[dlen++] = '\r';
//        dbg[dlen++] = '\n';
//    }

//    extern USBD_HandleTypeDef hUsbDeviceFS;
//    while (((USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData)->TxState != 0) { }
//    CDC_Transmit_FS((uint8_t *)dbg, (uint16_t)dlen);

    if (crc_received != crc_computed) {
       return false;
    }

    out->payload_len = frame->payload_len - 1 /*Type */ - 1 /*CRC */;
    if (out->payload_len > sizeof(frame->payload)) return false;
    out->type = p[0];
    memcpy(out->payload, &p[1], out->payload_len);
    return true;
}

bool crsf_decode_flight_mode(const crsf_frame_t *frame, char *out_str, size_t out_str_size)
{
    if (frame->type != CRSF_FT_FLIGHT_MODE || out_str_size == 0) return false;
    size_t n = frame->payload_len;
    if (n >= out_str_size) n = out_str_size - 1;
    memcpy(out_str, frame->payload, n);
    out_str[n] = '\0';
    return true;
}

uint16_t crsf_ticks_to_us(int ticks)
{
    return (uint16_t)((ticks - 992) * 5 / 8 + 1500);
}

int crsf_us_to_ticks(uint16_t us)
{
    return ((int)us - 1500) * 8 / 5 + 992;
}

/* ==================== Construction ==================== */

size_t crsf_build_frame(uint8_t *buf, uint8_t type, const uint8_t *payload, uint8_t payload_len)
{
    /* buf[0] = sync, buf[1] = length, buf[2] = type, buf[3..] = payload, dernier = crc */
    buf[0] = CRSF_SYNC_BYTE;
    buf[1] = (uint8_t)(payload_len + 2); /* type + payload + crc */
    buf[2] = type;
    if (payload_len > 0) {
        memcpy(&buf[3], payload, payload_len);
    }
    uint8_t crc = crsf_crc8(&buf[2], (uint8_t)(payload_len + 1)); /* type + payload */
    buf[3 + payload_len] = crc;
    return (size_t)(4 + payload_len);
}

/**
 * Construit une trame CRSF de type Command (0x32) avec sous-commande.
 *
 * @param buf         buffer de sortie (doit faire au moins 8 + payload_len octets, max 64)
 * @param dest_addr   adresse destination (ex: CRSF_ADDR_CROSSFIRE_RX, CRSF_ADDR_FLIGHT_CONTROLLER...)
 * @param orig_addr   adresse origine (ex: CRSF_ADDR_RADIO_TRANSMITTER)
 * @param command_id  identifiant de la sous-commande (ex: CRSF_CMD_CROSSFIRE, CRSF_CMD_VTX...)
 * @param payload     payload de la sous-commande (peut être NULL si payload_len == 0)
 * @param payload_len taille du payload
 * @return nombre d'octets écrits dans buf, ou 0 si payload_len trop grand
 */
size_t crsf_build_command(uint8_t *buf,
                           uint8_t dest_addr, uint8_t orig_addr,
                           uint8_t command_id,
                           const uint8_t *payload, uint8_t payload_len)
{
    /* Taille totale = sync+len+type+dest+orig+cmd_id+payload+cmd_crc+frame_crc = 8 + payload_len
       Une trame CRSF fait 64 octets max au total. */
    if ((size_t)payload_len + 8 > 64) {
        return 0;
    }

    buf[0] = CRSF_SYNC_BYTE;
    /* frame_len = Type + Dest + Orig + Command_ID + Payload + Command_CRC8 + Frame_CRC8 */
    buf[1] = (uint8_t)(payload_len + 6);
    buf[2] = CRSF_FT_COMMAND;
    buf[3] = dest_addr;
    buf[4] = orig_addr;
    buf[5] = command_id;
    if (payload_len > 0 && payload != NULL) {
        memcpy(&buf[6], payload, payload_len);
    }

    /* Command_CRC8 (poly 0xBA) : couvre Type + Dest + Orig + Command_ID + Payload */
    uint8_t cmd_crc_len = (uint8_t)(4 + payload_len); /* type+dest+orig+cmd_id + payload */
    uint8_t cmd_crc = command_crc8(&buf[2], cmd_crc_len);
    buf[6 + payload_len] = cmd_crc;

    /* Frame_CRC8 (poly 0xD5) : couvre Type + Dest + Orig + Command_ID + Payload + Command_CRC8 */
    uint8_t frame_crc_len = (uint8_t)(cmd_crc_len + 1);
    uint8_t frame_crc = crsf_crc8(&buf[2], frame_crc_len);
    buf[7 + payload_len] = frame_crc;

    return (size_t)(8 + payload_len);
}

size_t crsf_build_gps(uint8_t *buf,
                       double latitude_deg, double longitude_deg,
                       float groundspeed_kmh, float heading_deg,
                       float altitude_m, uint8_t satellites)
{
    uint8_t payload[15];
    wr_i32(&payload[0], (int32_t)(latitude_deg * 1e7));
    wr_i32(&payload[4], (int32_t)(longitude_deg * 1e7));
    wr_u16(&payload[8], (uint16_t)(groundspeed_kmh * 10.0f));
    wr_u16(&payload[10], (uint16_t)(heading_deg * 100.0f));
    wr_u16(&payload[12], (uint16_t)(altitude_m + 1000.0f));
    payload[14] = satellites;
    return crsf_build_frame(buf, CRSF_FT_GPS, payload, sizeof(payload));
}

size_t crsf_build_vario(uint8_t *buf, float vertical_speed_m_s)
{
    uint8_t payload[2];
    wr_i16(&payload[0], (int16_t)(vertical_speed_m_s * 100.0f));
    return crsf_build_frame(buf, CRSF_FT_VARIO, payload, sizeof(payload));
}

size_t crsf_build_battery(uint8_t *buf,
                           float voltage_v, float current_a,
                           uint32_t capacity_used_mah, uint8_t remaining_pct)
{
    uint8_t payload[8];
    wr_i16(&payload[0], (int16_t)(voltage_v * 10.0f)); /* decivolts */
    wr_i16(&payload[2], (int16_t)(current_a * 10.0f)); /* deciamps */
    wr_u24(&payload[4], capacity_used_mah);
    payload[7] = remaining_pct;
    return crsf_build_frame(buf, CRSF_FT_BATTERY_SENSOR, payload, sizeof(payload));
}

size_t crsf_build_baro_altitude(uint8_t *buf, float altitude_m,
                                 bool has_vertical_speed, float vertical_speed_m_s)
{
    int32_t altitude_dm = (int32_t)lroundf(altitude_m * 10.0f);
    uint16_t packed;

    const int32_t ALT_MIN_DM = 10000;
    const int32_t ALT_THRESHOLD_DM = 0x8000 - ALT_MIN_DM;
    const int32_t ALT_MAX_DM = 0x7FFE * 10 - 5;

    if (altitude_dm < -ALT_MIN_DM) {
        packed = 0;
    } else if (altitude_dm > ALT_MAX_DM) {
        packed = 0xFFFE;
    } else if (altitude_dm < ALT_THRESHOLD_DM) {
        packed = (uint16_t)(altitude_dm + ALT_MIN_DM);
    } else {
        packed = (uint16_t)(((altitude_dm + 5) / 10) | 0x8000);
    }

    uint8_t payload[3];
    wr_u16(&payload[0], packed);

    uint8_t payload_len = 2;
    if (has_vertical_speed) {
        const float Kl = 100.0f;
        const float Kr = 0.026f;
        float vs_cm_s = vertical_speed_m_s * 100.0f;
        float sign = (vs_cm_s < 0) ? -1.0f : 1.0f;
        float mag = fabsf(vs_cm_s) / Kl + 1.0f;
        long packed_val = lroundf((logf(mag) / Kr) * sign);
        if (packed_val > 127) packed_val = 127;
        if (packed_val < -127) packed_val = -127;
        int8_t vspeed_packed = (int8_t)packed_val;
        payload[2] = (uint8_t)vspeed_packed;
        payload_len = 3;
    }

    return crsf_build_frame(buf, CRSF_FT_BARO_ALTITUDE, payload, payload_len);
}

size_t crsf_build_attitude(uint8_t *buf, float pitch_rad, float roll_rad, float yaw_rad)
{
    uint8_t payload[6];
    const float INV_LSB = 10000.0f; /* 1 / 100 microradians */
    wr_i16(&payload[0], (int16_t)(pitch_rad * INV_LSB));
    wr_i16(&payload[2], (int16_t)(roll_rad * INV_LSB));
    wr_i16(&payload[4], (int16_t)(yaw_rad * INV_LSB));
    return crsf_build_frame(buf, CRSF_FT_ATTITUDE, payload, sizeof(payload));
}

size_t crsf_build_flight_mode(uint8_t *buf, const char *mode)
{
    size_t len = strlen(mode);
    if (len > CRSF_MAX_PAYLOAD_SIZE - 1) {
        len = CRSF_MAX_PAYLOAD_SIZE - 1;
    }
    uint8_t payload[CRSF_MAX_PAYLOAD_SIZE];
    memcpy(payload, mode, len);
    payload[len] = '\0';
    return crsf_build_frame(buf, CRSF_FT_FLIGHT_MODE, payload, (uint8_t)(len + 1));
}

size_t crsf_build_heartbeat(uint8_t *buf, uint16_t origin_address)
{
    uint8_t payload[2];
    wr_u16(&payload[0], origin_address);
    return crsf_build_frame(buf, CRSF_FT_HEARTBEAT, payload, sizeof(payload));
}
