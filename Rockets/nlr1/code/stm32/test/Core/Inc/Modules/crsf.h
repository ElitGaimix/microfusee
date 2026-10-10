/*
 * crsf.h — Librairie CRSF (Crossfire / TBS) pour microcontroleur (STM32, etc.)
 *
 * Base sur la spec officielle TBS :
 *   https://github.com/tbs-fpv/tbs-crsf-spec/blob/main/crsf.md
 * et recoupee avec des implementations de reference (Betaflight, ExpressLRS,
 * AlfredoCRSF) pour lever les ambiguites de la doc.
 *
 * Portable, sans dependance materielle : la lib ne fait aucun acces UART
 * elle-meme. C'est a l'appelant de :
 *   - nourrir le parser octet par octet avec crsf_parser_process_byte()
 *     (typiquement depuis l'interruption RX de l'UART, ou une tache qui lit
 *     le buffer DMA)
 *   - envoyer les buffers produits par les crsf_build_xxx() sur l'UART
 *     (typiquement en DMA/HAL_UART_Transmit)
 *
 * Bus CRSF cote "vehicule" (recepteur <-> FC/ordinateur de bord) :
 *   416666 baud (ou 420000 selon les sources), 8N1, non inverse, half ou
 *   full duplex selon montage.
 */

#ifndef CRSF_H
#define CRSF_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "crsf_command.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Constantes protocole ==================== */

#define CRSF_SYNC_BYTE          0xC8U
#define CRSF_MAX_FRAME_SIZE     64U   /* sync + len + type + payload + crc, max */
#define CRSF_MAX_PAYLOAD_SIZE   (CRSF_MAX_FRAME_SIZE - 4U)
#define CRSF_FRAME_LENGTH_MIN   2U    /* valeur min du champ "length" */
#define CRSF_FRAME_LENGTH_MAX   62U   /* valeur max du champ "length" */

/* ==================== Adresses des peripheriques ==================== */

typedef enum {
    CRSF_ADDR_BROADCAST         = 0x00,
    CRSF_ADDR_USB                = 0x10,
    CRSF_ADDR_BLUETOOTH_WIFI     = 0x12,
    CRSF_ADDR_WIFI_RX            = 0x13,
    CRSF_ADDR_VIDEO_RECEIVER     = 0x14,
    CRSF_ADDR_OSD_TBS_CORE       = 0x80,
    CRSF_ADDR_CURRENT_SENSOR     = 0xC0,
    CRSF_ADDR_GPS                = 0xC2,
    CRSF_ADDR_TBS_BLACKBOX       = 0xC4,
    CRSF_ADDR_FLIGHT_CONTROLLER  = 0xC8,
    CRSF_ADDR_RACE_TAG           = 0xCC,
    CRSF_ADDR_VTX                = 0xCE,
    CRSF_ADDR_RADIO_TRANSMITTER  = 0xEA,
    CRSF_ADDR_REPEATER_RX        = 0xEB,
    CRSF_ADDR_CRSF_RECEIVER      = 0xEC,
    CRSF_ADDR_REPEATER_TX        = 0xED,
    CRSF_ADDR_CRSF_TRANSMITTER   = 0xEE,
} crsf_addr_t;

/* ==================== Types de frame ====================
 * < 0x28  -> frame "broadcast" (en-tete court : type + payload + crc)
 * >= 0x28 -> frame "extended"  (en-tete long  : type + dest + orig + payload + crc)
 */

typedef enum {
    CRSF_FT_GPS                 = 0x02,
    CRSF_FT_GPS_TIME            = 0x03,
    CRSF_FT_GPS_EXTENDED        = 0x06,
    CRSF_FT_VARIO               = 0x07,
    CRSF_FT_BATTERY_SENSOR      = 0x08,
    CRSF_FT_BARO_ALTITUDE       = 0x09,
    CRSF_FT_AIRSPEED            = 0x0A,
    CRSF_FT_HEARTBEAT           = 0x0B,
    CRSF_FT_RPM                 = 0x0C,
    CRSF_FT_TEMP                = 0x0D,
    CRSF_FT_VOLTAGES            = 0x0E,
    CRSF_FT_VTX_TELEMETRY       = 0x10,
    CRSF_FT_BAROMETER           = 0x11,
    CRSF_FT_MAGNETOMETER        = 0x12,
    CRSF_FT_ACCEL_GYRO          = 0x13,
    CRSF_FT_LINK_STATISTICS     = 0x14,
    CRSF_FT_LINK_STATISTICS_REPEATER = 0x15,
    CRSF_FT_RC_CHANNELS_PACKED  = 0x16,
    CRSF_FT_SUBSET_RC_CHANNELS  = 0x17,
    CRSF_FT_LINK_STATISTICS_RX  = 0x1C,
    CRSF_FT_LINK_STATISTICS_TX  = 0x1D,
    CRSF_FT_ATTITUDE            = 0x1E,
    CRSF_FT_FLIGHT_MODE         = 0x21,
    CRSF_FT_DEVICE_PING         = 0x28,
    CRSF_FT_DEVICE_INFO         = 0x29,
    CRSF_FT_PARAM_ENTRY         = 0x2B,
    CRSF_FT_PARAM_READ          = 0x2C,
    CRSF_FT_PARAM_WRITE         = 0x2D,
    CRSF_FT_COMMAND             = 0x32,
    CRSF_FT_MSP_REQ             = 0x7A,
    CRSF_FT_MSP_RESP            = 0x7B,
    CRSF_FT_MSP_WRITE           = 0x7C,
    CRSF_FT_ARDUPILOT_RESP      = 0x80,
} crsf_frame_type_t;

/* ==================== Frame brute (sortie du parser) ==================== */

typedef struct {
    uint8_t type;
    uint8_t payload[CRSF_MAX_PAYLOAD_SIZE];
    uint8_t payload_len;
    bool    has_extended_header;   /* type >= 0x28 */
    uint8_t dest_addr;             /* valide seulement si has_extended_header */
    uint8_t orig_addr;             /* valide seulement si has_extended_header */
} crsf_frame_t;

/* ==================== Parser (state machine, octet par octet) ==================== */

typedef enum {
    CRSF_PARSE_WAIT_SYNC = 0,
    CRSF_PARSE_WAIT_LEN,
    CRSF_PARSE_WAIT_DATA,
} crsf_parse_state_t;

typedef struct {
    crsf_parse_state_t state;
    uint8_t buffer[CRSF_MAX_FRAME_SIZE];
    uint8_t frame_len;   /* valeur du champ "length" recu (type+payload+crc) */
    uint8_t index;       /* nb d'octets deja stockes dans buffer */
} crsf_parser_t;

void crsf_parser_init(crsf_parser_t *parser);

/*
 * A appeler pour chaque octet recu sur l'UART (idealement depuis l'ISR RX,
 * ou en boucle sur un buffer DMA).
 * Retourne true quand une frame complete ET valide (CRC ok) vient d'etre
 * decodee ; dans ce cas *out_frame est rempli et pret pour crsf_decode_xxx().
 * Retourne false sinon (frame en cours, resynchronisation, ou CRC invalide).
 */
bool crsf_parser_process_byte(crsf_parser_t *parser, uint8_t byte,
                               crsf_frame_t *out_frame);

/*
 * Variante pratique quand la reception fournit un buffer entier d'un coup
 * (typiquement : interruption idle-line + DMA, un burst = un appel).
 * Boucle en interne sur crsf_parser_process_byte() et appelle callback()
 * pour chaque trame complete et valide trouvee dans le buffer -- il peut y
 * en avoir 0, 1, ou plusieurs si le module a envoye plusieurs frames avant
 * la pause detectee par l'idle-line.
 * user_ctx est repasse tel quel a callback (contexte libre, peut etre NULL).
 */
typedef void (*crsf_frame_callback_t)(const crsf_frame_t *frame, void *user_ctx);

void crsf_parser_process_buffer(crsf_parser_t *parser, const uint8_t *data, size_t len,
                                 crsf_frame_callback_t callback, void *user_ctx);

/* ==================== CRC ==================== */

/* CRC8 poly 0xD5 (DVB-S2), utilise pour toutes les frames CRSF standard. */
uint8_t crsf_crc8(const uint8_t *data, uint8_t len);

/* ==================== Structures decodees ==================== */

typedef struct {
    int32_t  latitude;      /* degres * 1e7 */
    int32_t  longitude;     /* degres * 1e7 */
    uint16_t groundspeed;   /* km/h * 10 */
    uint16_t heading;       /* degres * 100 */
    uint16_t altitude_raw;  /* metres, offset de -1000m (0 => -1000m) */
    uint8_t  satellites;
} crsf_gps_t;

typedef struct {
    int16_t vertical_speed_cm_s;
} crsf_vario_t;

typedef struct {
    float    voltage_v;          /* volts */
    float    current_a;          /* amperes */
    uint32_t capacity_used_mah;  /* mAh */
    uint8_t  remaining_pct;      /* 0-100 */
} crsf_battery_t;

typedef struct {
    float altitude_m;         /* altitude au-dessus du point de calibration */
    bool  has_vertical_speed; /* le vario n'est present que si payload >= 3 octets */
    float vertical_speed_m_s;
} crsf_baro_altitude_t;

typedef struct {
    float pitch_rad;
    float roll_rad;
    float yaw_rad;
} crsf_attitude_t;

typedef struct {
    uint8_t up_rssi_ant1;      /* dBm (valeur positive, a negativiser) */
    uint8_t up_rssi_ant2;
    uint8_t up_link_quality;   /* % */
    int8_t  up_snr;            /* dB */
    uint8_t active_antenna;
    uint8_t rf_profile;        /* 0=4fps,1=50fps,2=150fps ... */
    uint8_t up_rf_power;       /* enum, voir crsf_rf_power_t */
    uint8_t down_rssi;
    uint8_t down_link_quality;
    int8_t  down_snr;
} crsf_link_statistics_t;

typedef struct {
    uint16_t channels[16]; /* valeurs brutes 11 bits (172-1811), centre = 992 */
} crsf_rc_channels_t;

typedef struct {
    int16_t origin_address;
} crsf_heartbeat_t;

/* ==================== Fonctions de decodage ====================
 * Chacune verifie frame->type et la taille du payload avant de decoder.
 * Retourne false si le type ne correspond pas ou si le payload est trop court.
 */
bool crsf_decode_command(const crsf_frame_t *frame, crsf_cmd_frame_t *out);
bool crsf_decode_gps(const crsf_frame_t *frame, crsf_gps_t *out);
bool crsf_decode_vario(const crsf_frame_t *frame, crsf_vario_t *out);
bool crsf_decode_battery(const crsf_frame_t *frame, crsf_battery_t *out);
bool crsf_decode_baro_altitude(const crsf_frame_t *frame, crsf_baro_altitude_t *out);
bool crsf_decode_attitude(const crsf_frame_t *frame, crsf_attitude_t *out);
bool crsf_decode_link_statistics(const crsf_frame_t *frame, crsf_link_statistics_t *out);
bool crsf_decode_rc_channels(const crsf_frame_t *frame, crsf_rc_channels_t *out);
bool crsf_decode_heartbeat(const crsf_frame_t *frame, crsf_heartbeat_t *out);
/* out_str est toujours null-termine (tronque si out_str_size trop petit) */
bool crsf_decode_flight_mode(const crsf_frame_t *frame, char *out_str, size_t out_str_size);

/* Conversion pratique ticks CRSF (11 bits, centre 992) <-> microsecondes,
 * comme sur une sortie PWM standard (992 <-> 1500us). */
uint16_t crsf_ticks_to_us(int ticks);
int      crsf_us_to_ticks(uint16_t us);

/* ==================== Fonctions de construction (telemetrie a emettre) ====================
 * Chaque fonction ecrit une frame CRSF complete (sync+len+type+payload+crc)
 * dans buf et retourne le nombre d'octets ecrits, prets a etre envoyes tels
 * quels sur l'UART. buf doit faire au moins CRSF_MAX_FRAME_SIZE octets.
 */

size_t crsf_build_gps(uint8_t *buf,
                       double latitude_deg, double longitude_deg,
                       float groundspeed_kmh, float heading_deg,
                       float altitude_m, uint8_t satellites);
size_t crsf_build_command(uint8_t *buf,
                           uint8_t dest_addr, uint8_t orig_addr,
                           uint8_t command_id,
                           const uint8_t *payload, uint8_t payload_len);
size_t crsf_build_vario(uint8_t *buf, float vertical_speed_m_s);

size_t crsf_build_battery(uint8_t *buf,
                           float voltage_v, float current_a,
                           uint32_t capacity_used_mah, uint8_t remaining_pct);

/* has_vertical_speed=false -> n'envoie que l'altitude (2 octets de payload) */
size_t crsf_build_baro_altitude(uint8_t *buf, float altitude_m,
                                 bool has_vertical_speed, float vertical_speed_m_s);

size_t crsf_build_attitude(uint8_t *buf, float pitch_rad, float roll_rad, float yaw_rad);

size_t crsf_build_flight_mode(uint8_t *buf, const char *mode);

size_t crsf_build_heartbeat(uint8_t *buf, uint16_t origin_address);

/* Construction generique pour un type de frame non couvert ci-dessus.
 * payload doit deja etre au format "fil" (big endian, tel que defini par
 * la spec pour le type en question). */
size_t crsf_build_frame(uint8_t *buf, uint8_t type,
                         const uint8_t *payload, uint8_t payload_len);

#ifdef __cplusplus
}
#endif

#endif /* CRSF_H */
