package fr.elitgaimix.nlfc;

import com.fazecast.jSerialComm.SerialPort;
import com.fazecast.jSerialComm.SerialPortDataListener;
import com.fazecast.jSerialComm.SerialPortEvent;
import javafx.application.Platform;

import static fr.elitgaimix.nlfc.NLRController.formatPacket;

public class ELRSListener implements SerialPortDataListener {

    @Override
    public int getListeningEvents() { return SerialPort.LISTENING_EVENT_DATA_AVAILABLE + SerialPort.LISTENING_EVENT_PORT_DISCONNECTED; }

    @Override
    public void serialEvent(SerialPortEvent event) {
        SerialPort serialPort = event.getSerialPort();
        if (event.getEventType() == SerialPort.LISTENING_EVENT_DATA_AVAILABLE) {
            int available = serialPort.bytesAvailable();
            if (available <= 0)
                return;
            byte[] data = new byte[available];
            int bytesRead = serialPort.readBytes(data, data.length);
            if (bytesRead > 0) {
                for (int i = 0; i < bytesRead; i++) {
                    NLRController.Packet packet = NLRController.INSTANCE.packetParser.append(data[i]);
                    if (packet != null) {
                        Platform.runLater(() -> NLRController.INSTANCE.appendConsole(formatPacket(packet)));
                    }
                }
            }
        }else if ((event.getEventType() & SerialPort.LISTENING_EVENT_PORT_DISCONNECTED) > 0) {
            serialPort.closePort();
            if (NLRController.INSTANCE.actualPort == serialPort)
                NLRController.INSTANCE.actualPort = null;
            Platform.runLater(() -> NLRController.INSTANCE.appendConsole("Serial port disconnected." + System.lineSeparator()));
        }
    }
}
