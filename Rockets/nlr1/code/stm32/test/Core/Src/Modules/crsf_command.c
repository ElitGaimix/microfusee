/*
 * crsf_command.c
 *
 *  Created on: 5 août 2026
 *      Author: ElitGaimix
 */
#include "Modules/crsf_command.h"

unsigned char command_crc8_table[256] = {
    0x00, 0xBA, 0xCE, 0x74, 0x26, 0x9C, 0xE8, 0x52, 0x4C, 0xF6, 0x82, 0x38, 0x6A, 0xD0, 0xA4, 0x1E,
    0x98, 0x22, 0x56, 0xEC, 0xBE, 0x04, 0x70, 0xCA, 0xD4, 0x6E, 0x1A, 0xA0, 0xF2, 0x48, 0x3C, 0x86,
    0x8A, 0x30, 0x44, 0xFE, 0xAC, 0x16, 0x62, 0xD8, 0xC6, 0x7C, 0x08, 0xB2, 0xE0, 0x5A, 0x2E, 0x94,
    0x12, 0xA8, 0xDC, 0x66, 0x34, 0x8E, 0xFA, 0x40, 0x5E, 0xE4, 0x90, 0x2A, 0x78, 0xC2, 0xB6, 0x0C,
    0xAE, 0x14, 0x60, 0xDA, 0x88, 0x32, 0x46, 0xFC, 0xE2, 0x58, 0x2C, 0x96, 0xC4, 0x7E, 0x0A, 0xB0,
    0x36, 0x8C, 0xF8, 0x42, 0x10, 0xAA, 0xDE, 0x64, 0x7A, 0xC0, 0xB4, 0x0E, 0x5C, 0xE6, 0x92, 0x28,
    0x24, 0x9E, 0xEA, 0x50, 0x02, 0xB8, 0xCC, 0x76, 0x68, 0xD2, 0xA6, 0x1C, 0x4E, 0xF4, 0x80, 0x3A,
    0xBC, 0x06, 0x72, 0xC8, 0x9A, 0x20, 0x54, 0xEE, 0xF0, 0x4A, 0x3E, 0x84, 0xD6, 0x6C, 0x18, 0xA2,
    0xE6, 0x5C, 0x28, 0x92, 0xC0, 0x7A, 0x0E, 0xB4, 0xAA, 0x10, 0x64, 0xDE, 0x8C, 0x36, 0x42, 0xF8,
    0x7E, 0xC4, 0xB0, 0x0A, 0x58, 0xE2, 0x96, 0x2C, 0x32, 0x88, 0xFC, 0x46, 0x14, 0xAE, 0xDA, 0x60,
    0x6C, 0xD6, 0xA2, 0x18, 0x4A, 0xF0, 0x84, 0x3E, 0x20, 0x9A, 0xEE, 0x54, 0x06, 0xBC, 0xC8, 0x72,
    0xF4, 0x4E, 0x3A, 0x80, 0xD2, 0x68, 0x1C, 0xA6, 0xB8, 0x02, 0x76, 0xCC, 0x9E, 0x24, 0x50, 0xEA,
    0x48, 0xF2, 0x86, 0x3C, 0x6E, 0xD4, 0xA0, 0x1A, 0x04, 0xBE, 0xCA, 0x70, 0x22, 0x98, 0xEC, 0x56,
    0xD0, 0x6A, 0x1E, 0xA4, 0xF6, 0x4C, 0x38, 0x82, 0x9C, 0x26, 0x52, 0xE8, 0xBA, 0x00, 0x74, 0xCE,
    0xC2, 0x78, 0x0C, 0xB6, 0xE4, 0x5E, 0x2A, 0x90, 0x8E, 0x34, 0x40, 0xFA, 0xA8, 0x12, 0x66, 0xDC,
    0x5A, 0xE0, 0x94, 0x2E, 0x7C, 0xC6, 0xB2, 0x08, 0x16, 0xAC, 0xD8, 0x62, 0x30, 0x8A, 0xFE, 0x44
};

uint8_t command_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0; // Initialisation à 0x00
    for (uint8_t i = 0; i < len; i++) {
        crc = command_crc8_table[crc ^ data[i]];
    }
    return crc;
}

bool command_decode_unique(const crsf_cmd_frame_t *frame, crsf_cmd_simple_t *out)
{
	if (!frame || !out) return false;

	switch(frame->type){
	case CRSF_CMD_FC:
	case CRSF_CMD_OSD:
	case CRSF_CMD_BLTH:
	case CRSF_CMD_VTX:
	case CRSF_CMD_LED:
	case CRSF_CMD_GENERAL:
	case CRSF_CMD_CROSSFIRE:
	case CRSF_CMD_ROW:
	case CRSF_CMD_FCF:
		if (frame->payload_len < 1) return false;
		out->type = frame->type;
		out->value = frame->payload[0];
		memcpy(out->more_payload, &frame->payload[1],frame->payload_len - 1 /*Value */);
		return true;

	default:
		return false;
	}
}
bool command_decode_ack(const crsf_cmd_frame_t *frame, crsf_cmd_ack_t *out)
{
	if (frame->type != CRSF_CMD_ACK) return false;
	out->Command_ID = frame->payload[0];
	out->SubCommand_ID = frame->payload[1];
	out->Action = frame->payload[2];
	memcpy(out->Information, &frame->payload[3],frame->payload_len - 3);
	return true;

}

// Ia generation, to check
bool command_decode_screen(const crsf_cmd_frame_t *frame, crsf_cmd_screen_t *out) {
    if (!frame || !out) return false;
    if (frame->type != CRSF_CMD_ACK) return false;
    if (frame->payload_len < 1) return false;

    // Remise à zéro de la structure de sortie
    memset(out, 0, sizeof(crsf_cmd_screen_t));

    const uint8_t *p = frame->payload;
    size_t remaining = frame->payload_len;

    // Octet 0 : Type de sous-commande
    out->type = p[0];
    p += 1;
    remaining -= 1;

    // =========================================================================
    // 0x01 : CRSF_CMD_SCREEN_PMS
    // =========================================================================
    if (out->type == CRSF_CMD_SCREEN_PMS) {
        size_t len;

        // 1. Header (chaîne terminée par '\0')
        len = strnlen((const char *)p, remaining);
        if (len >= remaining) return false; // '\0' non trouvé avant la fin du buffer
        out->pms.header = (const char *)p;
        p += (len + 1); remaining -= (len + 1);

        // 2. Info Message (chaîne terminée par '\0')
        len = strnlen((const char *)p, remaining);
        if (len >= remaining) return false;
        out->pms.info_message = (const char *)p;
        p += (len + 1); remaining -= (len + 1);

        // 3. Max Timeout Interval & Close Button Option (2 octets)
        if (remaining < 2) return false;
        out->pms.max_timeout_interval = p[0];
        out->pms.close_button_option  = (p[1] != 0);
        p += 2; remaining -= 2;

        // 4. Champ optionnel : add_data
        if (remaining > 0) {
            len = strnlen((const char *)p, remaining);
            if (len >= remaining) return false;

            // Si le premier octet n'est pas '\0', selectionText existe et contient du texte
            if (len > 0) {
                out->pms.has_add_data = true;
                out->pms.add_data.selectionText = (const char *)p;
                p += (len + 1); remaining -= (len + 1);

                // 4 variables uint8_t
                if (remaining < 4) return false;
                out->pms.add_data.value        = p[0];
                out->pms.add_data.minValue     = p[1];
                out->pms.add_data.maxValue     = p[2];
                out->pms.add_data.defaultValue = p[3];
                p += 4; remaining -= 4;

                // Unit (chaîne terminée par '\0')
                len = strnlen((const char *)p, remaining);
                if (len >= remaining) return false;
                out->pms.add_data.unit = (const char *)p;
                p += (len + 1); remaining -= (len + 1);
            } else {
                // Seulement l'octet NULL était présent ('\0'), add_data est ignoré
                out->pms.has_add_data = false;
                p += 1; remaining -= 1;
            }
        }

        // 5. Champ optionnel : possible_values
        if (remaining > 0) {
            len = strnlen((const char *)p, remaining);
            if (len >= remaining) return false;
            out->pms.possible_values = (const char *)p;
        }

        return true;
    }
    // =========================================================================
    // 0x02 : CRSF_CMD_SCREEN_SRV
    // =========================================================================
    else if (out->type == CRSF_CMD_SCREEN_SRV) {
        if (remaining < 2) return false; // 1 octet valeur + 1 octet booléen

        out->srv.value    = p[0];
        out->srv.response = (p[1] != 0);
        return true;
    }

    return false; // Type non géré
}

bool command_decode_general(const crsf_cmd_frame_t *frame, crsf_cmd_general_t *out)
{
	if (frame->type != CRSF_CMD_GENERAL || frame->payload_len < 3) return false;
	out->type = frame->payload[0];
	out->port_id = frame->payload[1];
	if (out->type == CRSF_CMD_GENERAL_PSP){
		if(frame->payload_len < 6) return false;
		out->proposed_baudrate = ((uint32_t)frame->payload[2] << 24) |
		           ((uint32_t)frame->payload[3] << 16) |
		           ((uint32_t)frame->payload[4] << 8)  |
		           ((uint32_t)frame->payload[5]);
	}else{
		out->response = frame->payload[2];
	}
	return true;
}

/**
 * Sérialise une crsf_cmd_general_t en payload binaire CRSF.
 *
 * @param out    buffer de sortie (doit faire au moins 6 octets)
 * @param in     structure à encoder
 * @return       taille du payload écrit, ou 0 si type inconnu
 */
uint8_t crsf_encode_general(uint8_t *out, const crsf_cmd_general_t *in)
{
    out[0] = (uint8_t)in->type;   // 0x70 ou 0x71
    out[1] = in->port_id;

    if (in->type == CRSF_CMD_GENERAL_PSP) {
        // uint32_t proposed_baudrate, big endian
        out[2] = (uint8_t)(in->proposed_baudrate >> 24);
        out[3] = (uint8_t)(in->proposed_baudrate >> 16);
        out[4] = (uint8_t)(in->proposed_baudrate >> 8);
        out[5] = (uint8_t)(in->proposed_baudrate);
        return 6;
    } else if (in->type == CRSF_CMD_GENERAL_PSR) {
        out[2] = in->response ? 1 : 0;
        return 3;
    }

    return 0; // type inconnu
}

/**
 * Construit une trame CRSF complète pour répondre à une proposition de
 * baudrate (0x71 Protocol Speed Proposal Response).
 */
size_t crsf_build_command_general(uint8_t *buf,
                                   uint8_t dest_addr, uint8_t orig_addr,
                                   const crsf_cmd_general_t *cmd)
{
    uint8_t payload[6];
    uint8_t payload_len = crsf_encode_general(payload, cmd);
    if (payload_len == 0) return 0;

    return crsf_build_command(buf, dest_addr, orig_addr,
                               CRSF_CMD_GENERAL, payload, payload_len);
}

