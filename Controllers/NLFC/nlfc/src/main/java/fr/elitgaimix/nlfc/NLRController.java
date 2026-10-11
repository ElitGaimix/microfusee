package fr.elitgaimix.nlfc;

import com.fazecast.jSerialComm.SerialPort;
import com.fazecast.jSerialComm.SerialPortDataListener;
import com.fazecast.jSerialComm.SerialPortEvent;
import javafx.application.Platform;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.fxml.FXML;
import javafx.scene.control.Button;
import javafx.scene.control.ChoiceBox;
import javafx.scene.control.TextArea;

import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.nio.charset.StandardCharsets;

public class NLRController {

    public static NLRController INSTANCE;

    public NLRController() {
        INSTANCE = this;
    }

    @FXML
    private ChoiceBox<String> com;

    @FXML
    private Button connect;

    @FXML
    private TextArea console;

    public SerialPort actualPort;

    public final PacketParser packetParser = new PacketParser();

    @FXML
    protected void onRefreshButtonClick(){
        SerialPort[] ports = SerialPort.getCommPorts();
        if (ports.length==0){
            com.setItems(FXCollections.observableList(new ArrayList<>()));
            com.setValue("No ports found.");
            connect.setDisable(true);
            return;
        }
        com.setValue(ports[0].getSystemPortName());
        ObservableList<String> options = FXCollections.observableArrayList();
        for (SerialPort port : SerialPort.getCommPorts()){
            options.add(port.getSystemPortName());
        }
        com.setItems(options);
        connect.setDisable(false);
    }

    @FXML
    protected void sendHeatBeat(){
        SerialPort port = actualPort;
        if (port == null || !port.isOpen()) {
            appendConsole("Error: no serial port is connected." + System.lineSeparator());
            return;
        }

        byte[] data = buildPacket(0x00, "TEST".getBytes(StandardCharsets.US_ASCII));
        try {
            OutputStream out = port.getOutputStream();
            out.write(data);
            out.flush();
            appendConsole(">> Sent: " + formatBytes(data) + System.lineSeparator());
        } catch (IOException e) {
            appendConsole("Error sending packet: " + e.getMessage() + System.lineSeparator());
        }
    }

    @FXML
    protected void onConnectButtonClick(){
        String selectedPort = com.getValue();
        if (selectedPort == null){
            console.setText("Error: select a serial port first.");
            return;
        }

        SerialPort port = SerialPort.getCommPort(selectedPort);
        port.setBaudRate(9600);
        if (!port.openPort()){
            console.setText("Error: could not open " + selectedPort
                    + " (jSerialComm error " + port.getLastErrorCode()
                    + ", location " + port.getLastErrorLocation() + ").");
            return;
        }

        console.setText("Connected: " + selectedPort + System.lineSeparator());
        port.addDataListener(new ELRSListener());
        actualPort = port;

    }

    private static byte[] buildPacket(int packetId, byte[] payload) {
        if (payload.length > 255)
            throw new IllegalArgumentException("Payload is too long.");

        byte[] packet = new byte[payload.length + 4];
        packet[0] = (byte) 0xAA;
        packet[1] = (byte) payload.length;
        packet[2] = (byte) packetId;
        System.arraycopy(payload, 0, packet, 3, payload.length);
        packet[packet.length - 1] = 0x00;
        return packet;
    }

    private static String formatBytes(byte[] bytes) {
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
                + String.format("   Packet ID: 0x%02X%n", packet.packetId)
                + "   Payload: " + (packet.payload.length == 0 ? "(empty)" : formatBytes(packet.payload))
                + System.lineSeparator()
                + String.format("   Checksum: 0x%02X%n", packet.checksum);
    }

    public void appendConsole(String text) {
        console.appendText(text);
    }

    public static class Packet {
        private final int packetId;
        private final byte[] payload;
        private final int checksum;

        private Packet(int packetId, byte[] payload, int checksum) {
            this.packetId = packetId;
            this.payload = payload;
            this.checksum = checksum;
        }
    }

    public static class PacketParser {
        private int state;
        private int payloadLength;
        private int packetId;
        private byte[] payload;
        private int payloadIndex;

        Packet append(byte value) {
            int unsignedValue = value & 0xFF;
            switch (state) {
                case 0:
                    if (unsignedValue == 0xAA)
                        state = 1;
                    break;
                case 1:
                    payloadLength = unsignedValue;
                    payload = new byte[payloadLength];
                    state = 2;
                    break;
                case 2:
                    packetId = unsignedValue;
                    state = payloadLength == 0 ? 4 : 3;
                    break;
                case 3:
                    payload[payloadIndex++] = value;
                    if (payloadIndex == payloadLength)
                        state = 4;
                    break;
                case 4:
                    Packet packet = new Packet(packetId, payload, unsignedValue);
                    reset();
                    return packet;
                default:
                    reset();
            }
            return null;
        }

        private void reset() {
            state = 0;
            payloadLength = 0;
            packetId = 0;
            payload = null;
            payloadIndex = 0;
        }
    }
}
