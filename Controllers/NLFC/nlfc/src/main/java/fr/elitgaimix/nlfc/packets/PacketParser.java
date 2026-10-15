package fr.elitgaimix.nlfc.packets;

import fr.elitgaimix.nlfc.utils.CRC16Utils;

public class PacketParser {
    private int state;
    private int payloadLength;
    private int packetId;
    private byte[] payload;
    private int payloadIndex;
    private int calculatedCrc;
    private int receivedCrc;

    public synchronized Packet append(byte value) {
        int unsignedValue = value & 0xFF;
        switch (state) {
            case 0:
                if (unsignedValue == 0xAA) {
                    calculatedCrc = CRC16Utils.crc16Update(0xFFFF, unsignedValue);
                    state = 1;
                }
                break;
            case 1:
                calculatedCrc = CRC16Utils.crc16Update(calculatedCrc, unsignedValue);
                payloadLength = unsignedValue;
                payload = new byte[payloadLength];
                state = 2;
                break;
            case 2:
                calculatedCrc = CRC16Utils.crc16Update(calculatedCrc, unsignedValue);
                packetId = unsignedValue;
                state = payloadLength == 0 ? 4 : 3;
                break;
            case 3:
                calculatedCrc = CRC16Utils.crc16Update(calculatedCrc, unsignedValue);
                payload[payloadIndex++] = value;
                if (payloadIndex == payloadLength)
                    state = 4;
                break;
            case 4:
                receivedCrc = unsignedValue << 8;
                state = 5;
                break;
            case 5:
                receivedCrc |= unsignedValue;
                Packet packet = receivedCrc == calculatedCrc
                        ? new Packet(PacketType.fromId(packetId), payload, receivedCrc) : null;
                reset();
                return packet;
            default:
                reset();
        }
        return null;
    }

    public synchronized void reset() {
        state = 0;
        payloadLength = 0;
        packetId = 0;
        payload = null;
        payloadIndex = 0;
        calculatedCrc = 0xFFFF;
        receivedCrc = 0;
    }

    public static byte[] buildPacket(int packetId, byte[] payload) {
        if (payload.length > 255)
            throw new IllegalArgumentException("Payload is too long.");

        byte[] packet = new byte[payload.length + 5];
        packet[0] = (byte) 0xAA;
        packet[1] = (byte) payload.length;
        packet[2] = (byte) packetId;
        System.arraycopy(payload, 0, packet, 3, payload.length);
        int crcOffset = 3 + payload.length;
        int crc = CRC16Utils.crc16(packet, crcOffset);
        packet[crcOffset] = (byte) (crc >>> 8);
        packet[crcOffset + 1] = (byte) crc;
        return packet;
    }

    public static String formatBytes(byte[] bytes) {
        StringBuilder formatted = new StringBuilder();
        for (byte value : bytes) {
            if (!formatted.isEmpty())
                formatted.append(' ');
            formatted.append(String.format("0x%02X", value & 0xFF));
        }
        return formatted.toString();
    }

    public static String formatPacket(Packet packet) {
        return "<< Packet received" + System.lineSeparator()
                + "   Syncword: 0xAA" + System.lineSeparator()
                + "   Payload length: " + packet.payload.length + System.lineSeparator()
                + String.format("   Packet ID: 0x%02X%n", packet.type.id)
                + "   Payload: " + (packet.payload.length == 0 ? "(empty)" : formatBytes(packet.payload))
                + System.lineSeparator()
                + String.format("   CRC-16: 0x%04X%n", packet.crc);
    }
}
