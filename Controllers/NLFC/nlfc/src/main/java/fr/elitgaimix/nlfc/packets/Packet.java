package fr.elitgaimix.nlfc.packets;

public class Packet {
    public final PacketType type;
    public final byte[] payload;
    public int crc; //For receive packets only

    public Packet(PacketType type, byte[] payload, int crc) {
        this.type = type;
        this.payload = payload;
        this.crc = crc;
    }

    public Packet(PacketType type, byte[] payload) {
        this.type = type;
        this.payload = payload;
    }

    public Packet(PacketType type) {
        this.type = type;
        this.payload = new byte[]{};
    }

    public byte[] encode(){
        return PacketParser.buildPacket(type.id, payload);
    }
}
