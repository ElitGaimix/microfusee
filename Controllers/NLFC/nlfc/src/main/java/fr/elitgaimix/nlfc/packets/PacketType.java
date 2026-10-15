package fr.elitgaimix.nlfc.packets;

public enum PacketType {
    HEARTBEAT(0x00),
    INFO(0x01),
    GPS(0x02),
    CMD(0x03),
    ACK(0x04),
    ERROR(0xFF);

    public final int id;

    PacketType(int id){
        this.id = id;
    }

    public static PacketType fromId(int id){
        for(PacketType type : PacketType.values()){
            if(type.id == id){
                return type;
            }
        }
        return ERROR;
    }
}
