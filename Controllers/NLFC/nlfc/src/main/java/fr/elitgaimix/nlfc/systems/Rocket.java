package fr.elitgaimix.nlfc.systems;

import fr.elitgaimix.nlfc.packets.Packet;
import fr.elitgaimix.nlfc.packets.PacketType;

public class Rocket {

    public RocketMode mode;
    public boolean connected;
    public Transmitter transmitter;

    public Rocket(RocketMode mode) {
        this.mode = mode;
        this.connected = false;
        this.transmitter = new Transmitter();
    }

    public void sendHeartBeat(){
        transmitter.transmit(new Packet(PacketType.HEARTBEAT, new byte[]{0x02}));
    }

    public void stop(){
        transmitter.stop();
    }


}
