package fr.elitgaimix.nlfc.systems;

public enum RocketMode {
    DISARMED(0x00),
    ARMED(0x01),
    GND(0x02), // reserved for the transmitter
    ERROR(0xFF);
    public final int id;
    RocketMode(int id) {
        this.id = id;
    }

    public static RocketMode fromId(int id){
        for(RocketMode type : RocketMode.values()){
            if(type.id == id){
                return type;
            }
        }
        return ERROR;
    }
}
