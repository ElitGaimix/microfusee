package fr.elitgaimix.nlfc.utils;

public class CRC16Utils {
    /* CRC-16/CCITT-FALSE: init FFFF, poly 1021, no reflection/final XOR.
     * Covers syncword, length, ID and payload; high byte is sent first.
     */
    public static int crc16(byte[] data, int length) {
        int crc = 0xFFFF;
        for (int i = 0; i < length; i++) {
            crc = crc16Update(crc, data[i] & 0xFF);
        }
        return crc;
    }

    public static int crc16Update(int crc, int value) {
        crc ^= value << 8;
        for (int bit = 0; bit < 8; bit++) {
            crc = ((crc & 0x8000) != 0)
                    ? ((crc << 1) ^ 0x1021) & 0xFFFF
                    : (crc << 1) & 0xFFFF;
        }
        return crc;
    }
}
