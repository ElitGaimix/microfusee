package fr.elitgaimix.nlfc.rockets;

import com.fazecast.jSerialComm.SerialPort;
import fr.elitgaimix.nlfc.NLRController;
import fr.elitgaimix.nlfc.packets.PacketParser;

import java.io.IOException;
import java.io.OutputStream;

public class Rocket {

    public RocketMode mode;
    
    public volatile SerialPort port;
    public boolean connected;

    public Rocket(RocketMode mode, SerialPort port) {
        this.mode = mode;
        this.port = port;
        this.connected = false;
    }

    public void sendHeartBeat(){
        NLRController controller = new NLRController();
        if (port == null || !port.isOpen()) {
            controller.appendConsole("Error: no serial port is connected." + System.lineSeparator());
            return;
        }

        byte[] data = PacketParser.buildPacket(0x00, new byte[]{0x02});
        try {
            OutputStream out = port.getOutputStream();
            out.write(data);
            out.flush();
            controller.appendConsole(">> Sent: " + PacketParser.formatBytes(data) + System.lineSeparator());
        } catch (IOException e) {
            controller.appendConsole("Error sending packet: " + e.getMessage() + System.lineSeparator());
        }
    }


}
