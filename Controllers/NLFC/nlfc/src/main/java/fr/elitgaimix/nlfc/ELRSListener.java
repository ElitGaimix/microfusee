package fr.elitgaimix.nlfc;

import com.fazecast.jSerialComm.SerialPort;
import com.fazecast.jSerialComm.SerialPortDataListener;
import com.fazecast.jSerialComm.SerialPortEvent;
import fr.elitgaimix.nlfc.packets.Packet;
import fr.elitgaimix.nlfc.packets.PacketParser;
import fr.elitgaimix.nlfc.packets.PacketType;
import fr.elitgaimix.nlfc.systems.RocketMode;
import fr.elitgaimix.nlfc.systems.Transmitter;
import javafx.application.Platform;

public class ELRSListener implements SerialPortDataListener {

    private Transmitter transmitter;

    public ELRSListener(Transmitter transmitter) {
        this.transmitter = transmitter;
    }

    @Override
    public int getListeningEvents() { return SerialPort.LISTENING_EVENT_DATA_AVAILABLE | SerialPort.LISTENING_EVENT_PORT_DISCONNECTED; }

    @Override
    public void serialEvent(SerialPortEvent event) {
        SerialPort serialPort = event.getSerialPort();
        if (transmitter.port != serialPort)
            return;

        if ((event.getEventType() & SerialPort.LISTENING_EVENT_PORT_DISCONNECTED) != 0) {
            Platform.runLater(() -> transmitter.disconnect());
            return;
        }
        if ((event.getEventType() & SerialPort.LISTENING_EVENT_DATA_AVAILABLE) != 0) {
            int available = serialPort.bytesAvailable();
            if (available <= 0)
                return;
            byte[] data = new byte[available];
            int bytesRead = serialPort.readBytes(data, data.length);
            if (bytesRead > 0) {
                for (int i = 0; i < bytesRead; i++) {
                    Packet packet = transmitter.packetParser.append(data[i]);
                    if (packet != null) {
                        NLRController.INSTANCE.appendConsole(PacketParser.formatPacket(packet));
                        if (packet.type == PacketType.HEARTBEAT){
                            transmitter.runnable.onHeartbeatReceived();
                            if(packet.payload.length > 0)
                                NLRController.INSTANCE.rocket.mode = RocketMode.fromId(packet.payload[0]);
                            else
                                NLRController.INSTANCE.rocket.mode = RocketMode.ERROR;

                        }
                    }
                }
            }
        }
    }
}
