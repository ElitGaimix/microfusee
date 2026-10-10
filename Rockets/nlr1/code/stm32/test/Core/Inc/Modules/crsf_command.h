/*
 * crsf_command.h
 *
 *  Created on: 5 août 2026
 *      Author: ElitGaimix
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "crsf.h"

#ifndef CRSF_COMMAND_H_
#define CRSF_COMMAND_H_

typedef enum {
    CRSF_CMD_ACK               = 0xFF,
    CRSF_CMD_FC           = 0x01,
    CRSF_CMD_BLTH        = 0x03, // Bluetooth
    CRSF_CMD_OSD               = 0x05,
    CRSF_CMD_VTX      = 0x08,
    CRSF_CMD_LED       = 0x09,
    CRSF_CMD_GENERAL            = 0x0A,
    CRSF_CMD_CROSSFIRE           = 0x10,
    CRSF_CMD_ROW                = 0x13, // RC over WiFi
    CRSF_CMD_FCF            = 0x20, // Flow Control Frame
    CRSF_CMD_SCREEN       = 0x22
} crsf_cmd_type_t;

typedef struct {
	crsf_cmd_type_t type;
	uint8_t payload[60];
	uint8_t payload_len;
}crsf_cmd_frame_t;

typedef struct {
	crsf_cmd_type_t type;
	uint8_t value;
	uint8_t more_payload[64];
}crsf_cmd_simple_t; //For all type who have only one value

typedef struct {
	uint8_t Command_ID;;
	uint8_t SubCommand_ID;
	uint8_t Action;
	    // 1 mean target already take action
	    // 0 mean not a correct command or no function on that command yet
	uint8_t Information[]; // null terminated string
}crsf_cmd_ack_t;


typedef struct {
	enum {
	    CRSF_CMD_SCREEN_PMS = 0x01, // Pop up message start
	    CRSF_CMD_SCREEN_SRV = 0x02  // Selection Return Value
	} type;

    // --- Champs pour CRSF_CMD_SCREEN_PMS (0x01) ---
    struct {
        const char *header;                 // Pointe directement dans le buffer ou sur un string
        const char *info_message;
        uint8_t max_timeout_interval;
        bool close_button_option;

        bool has_add_data;                  // Vrai si add_data est présent
        struct {
            const char *selectionText;
            uint8_t value;
            uint8_t minValue;
            uint8_t maxValue;
            uint8_t defaultValue;
            const char *unit;
        } add_data;

        const char *possible_values;        // Séparés par ';' ou NULL si absent
    } pms;

    // --- Champs pour CRSF_CMD_SCREEN_SRV (0x02) ---
    struct {
        uint8_t value;
        bool response;
    } srv;

} crsf_cmd_screen_t;

typedef struct{
	enum {
		CRSF_CMD_GENERAL_PSP = 0x70,
		CRSF_CMD_GENERAL_PSR = 0x71,
	} type;
	uint8_t port_id;
	//0x70
	uint32_t proposed_baudrate;
	//0x71
	bool response;
}crsf_cmd_general_t;

uint8_t command_crc8(const uint8_t *data, uint8_t len);
bool command_decode_unique(const crsf_cmd_frame_t *frame, crsf_cmd_simple_t *out);
bool command_decode_ack(const crsf_cmd_frame_t *frame, crsf_cmd_ack_t *out);
bool command_decode_general(const crsf_cmd_frame_t *frame, crsf_cmd_general_t *out);
bool command_decode_screen(const crsf_cmd_frame_t *frame, crsf_cmd_screen_t *out);
size_t crsf_build_command_general(uint8_t *buf,
                                   uint8_t dest_addr, uint8_t orig_addr,
                                   const crsf_cmd_general_t *cmd);
uint8_t crsf_encode_general(uint8_t *out, const crsf_cmd_general_t *in);


#endif /* INC_MODULES_CRSF_COMMAND_H_ */
