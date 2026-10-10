#include <Arduino.h>
#define MAVLINK_COMM_NUM_BUFFERS 1
#include <MAVLink.h>

#define LED_CARTE PB2
HardwareSerial MAV_Serial(PA3, PA2);

static float sim_roll = -0.03f;
static float sim_pitch = 0.05f;
static float sim_yaw = 1.57f;
static float altitude_m = 450.0f;
static float climb_ms = 1.0f;
static int telem_interval = 500;
static float baro_alt_offset = 0.0f;
static float sim_time = 0.0f;
static uint32_t cmdCount = 0;

// ─── MAVLink helpers ────────────────────────────────────────────

void sendToMAV(mavlink_message_t* msg) {
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, msg);
    MAV_Serial.write(buf, len);
}

void sendHB() {
    mavlink_message_t msg;
    mavlink_msg_heartbeat_pack(1, MAV_COMP_ID_AUTOPILOT1, &msg,
        MAV_TYPE_ROCKET, MAV_AUTOPILOT_GENERIC, MAV_MODE_MANUAL_ARMED, 0, MAV_STATE_ACTIVE);
    sendToMAV(&msg);
}

void sendSYS() {
    mavlink_message_t msg;
    mavlink_msg_sys_status_pack(1, MAV_COMP_ID_AUTOPILOT1, &msg,
        0, 0, 0, 500, 14800, -1, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    sendToMAV(&msg);
}

void sendText(const char* text, uint8_t severity) {
    mavlink_message_t msg;
    mavlink_msg_statustext_pack(1, MAV_COMP_ID_AUTOPILOT1, &msg, severity, text, 0, 0);
    sendToMAV(&msg);
}

void sendVFR(float alt) {
    mavlink_message_t msg;
    uint8_t payload[20];
    float airspeed = 0.0f, groundspeed = 0.0f;
    int16_t heading = 180;
    uint16_t throttle = 0;
    memcpy(payload + 0,  &airspeed,    4);
    memcpy(payload + 4,  &groundspeed, 4);
    memcpy(payload + 8,  &heading,     2);
    memcpy(payload + 10, &throttle,    2);
    memcpy(payload + 12, &alt,         4);
    memcpy(payload + 16, &climb_ms,    4);
    memcpy(_MAV_PAYLOAD_NON_CONST(&msg), payload, 20);
    msg.len = 20; msg.msgid = 74;
    mavlink_finalize_message(&msg, 1, MAV_COMP_ID_AUTOPILOT1, 20, 20, 20);
    sendToMAV(&msg);
}

void sendAtt(float roll, float pitch, float yaw) {
    mavlink_message_t msg;
    uint8_t payload[16];
    uint32_t t = millis();
    memcpy(payload + 0, &t,    4);
    memcpy(payload + 4, &roll,  4);
    memcpy(payload + 8, &pitch, 4);
    memcpy(payload + 12, &yaw,  4);
    memcpy(_MAV_PAYLOAD_NON_CONST(&msg), payload, 16);
    msg.len = 16; msg.msgid = 30;
    mavlink_finalize_message(&msg, 1, MAV_COMP_ID_AUTOPILOT1, 16, 16, 39);
    sendToMAV(&msg);
}

// ─── Command handler ────────────────────────────────────────────

void handleCommand(mavlink_message_t* msg) {
    if (msg->msgid == MAVLINK_MSG_ID_TIMESYNC) {
        mavlink_timesync_t ts;
        mavlink_msg_timesync_decode(msg, &ts);
        mavlink_message_t resp;
        mavlink_msg_timesync_pack(1, MAV_COMP_ID_AUTOPILOT1, &resp,
            micros(), ts.tc1, msg->sysid, msg->compid);
        sendToMAV(&resp);
        return;
    }

    if (msg->msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
        mavlink_command_long_t cmd;
        mavlink_msg_command_long_decode(msg, &cmd);
        uint16_t c = cmd.command;

        if (c == 42001) {
            telem_interval = (int)cmd.param1;
            if (telem_interval < 50) telem_interval = 50;
            char buf[30];
            sprintf(buf, "TELEM %dms OK", telem_interval);
            sendText(buf, 0); cmdCount++;
        }
        else if (c == 42002) {
            baro_alt_offset = cmd.param1;
            int vi = (int)(baro_alt_offset * 10 + (baro_alt_offset < 0 ? -0.5f : 0.5f));
            char buf[30];
            sprintf(buf, "ALT %+d.%dm", vi / 10, (vi < 0 ? -vi : vi) % 10);
            sendText(buf, 0); cmdCount++;
        }
        else if (c == 42003) {
            sim_roll = cmd.param1;
            int vi = (int)(sim_roll * 1000 + (sim_roll < 0 ? -0.5f : 0.5f));
            char buf[30];
            sprintf(buf, "ROLL %+d.%03d", vi / 1000, (vi < 0 ? -vi : vi) % 1000);
            sendText(buf, 0); cmdCount++;
        }
        else if (c == 42004) {
            sim_pitch = cmd.param1;
            int vi = (int)(sim_pitch * 1000 + (sim_pitch < 0 ? -0.5f : 0.5f));
            char buf[30];
            sprintf(buf, "PITCH %+d.%03d", vi / 1000, (vi < 0 ? -vi : vi) % 1000);
            sendText(buf, 0); cmdCount++;
        }
        else if (c == 42005) {
            sim_yaw = cmd.param1;
            int vi = (int)(sim_yaw * 1000 + (sim_yaw < 0 ? -0.5f : 0.5f));
            char buf[30];
            sprintf(buf, "YAW %+d.%03d", vi / 1000, (vi < 0 ? -vi : vi) % 1000);
            sendText(buf, 0); cmdCount++;
        }
        return;
    }

    if (msg->msgid == MAVLINK_MSG_ID_RC_CHANNELS) {
        mavlink_rc_channels_t rc;
        mavlink_msg_rc_channels_decode(msg, &rc);
        sim_roll = (rc.chan1_raw - 1500) / 500.0f * 0.5f;
        sim_pitch = (rc.chan2_raw - 1500) / 500.0f * 0.5f;
    }
}

// ─── Setup / Loop ───────────────────────────────────────────────

unsigned long lastHB = 0, lastTelem = 0, lastPrint = 0;
bool ledState = false;

void setup() {
    delay(3000);
    pinMode(LED_CARTE, OUTPUT);
    digitalWrite(LED_CARTE, LOW);
    Serial.begin(115200);
    MAV_Serial.begin(115200);
    Serial.println("MAVLink simulator starting...");

    sendText("Simulator started", 0);
}

void loop() {
    // ── Receive ──
    while (MAV_Serial.available()) {
        uint8_t c = MAV_Serial.read();
        mavlink_message_t msg_in;
        mavlink_status_t status;
        if (mavlink_parse_char(MAVLINK_COMM_0, c, &msg_in, &status)) {
            handleCommand(&msg_in);
        }
    }

    // ── Simulate state ──
    sim_time += 0.01f;
    float wave = sinf(sim_time * 0.1f) * 2.0f;
    altitude_m = 450.0f + wave + baro_alt_offset;
    climb_ms = cosf(sim_time * 0.1f) * 0.2f;

    // ── Periodic sends ──
    if (millis() - lastHB >= 1000) {
        sendHB(); sendSYS();
        digitalWrite(LED_CARTE, ledState ? HIGH : LOW);
        ledState = !ledState;
        lastHB = millis();
    }

    if (millis() - lastTelem >= (unsigned long)telem_interval) {
        sendVFR(altitude_m);
        sendAtt(sim_roll, sim_pitch, sim_yaw);
        lastTelem = millis();
    }

    if (millis() - lastPrint >= 3000) {
        Serial.print("alt=");
        Serial.print(altitude_m, 1);
        Serial.print("m roll=");
        Serial.print(sim_roll, 2);
        Serial.print(" pitch=");
        Serial.print(sim_pitch, 2);
        Serial.print(" cmds=");
        Serial.println(cmdCount);
        lastPrint = millis();
    }
}
