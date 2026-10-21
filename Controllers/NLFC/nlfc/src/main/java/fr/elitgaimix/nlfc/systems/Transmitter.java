package fr.elitgaimix.nlfc.systems;

import com.fazecast.jSerialComm.SerialPort;
import fr.elitgaimix.nlfc.ELRSListener;
import fr.elitgaimix.nlfc.runnables.ELRSRunnable;
import fr.elitgaimix.nlfc.NLRController;
import fr.elitgaimix.nlfc.packets.Packet;
import fr.elitgaimix.nlfc.packets.PacketParser;

import java.io.IOException;
import java.io.OutputStream;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

public class Transmitter {

    public SerialPort port;
    public boolean connected = false;
    public PacketParser packetParser;
    public ScheduledFuture<?> scheduled;
    private ScheduledExecutorService heartbeatExecutor;
    public ELRSRunnable runnable;
    public ELRSListener listener;

    public Transmitter(){
        this.packetParser = new PacketParser();
        this.listener = new ELRSListener(this);
    }

    public void stop(){
        if (connected)
            disconnect();
    }

    public int transmit(Packet packet){
        return transmit(packet.encode());
    }

    public int transmit(byte[] data){
        NLRController controller = NLRController.INSTANCE;
        if (port == null || !port.isOpen()) {
            controller.appendConsole("Error: no serial port is connected." + System.lineSeparator());
            return -1;
        }
        try {
            OutputStream out = port.getOutputStream();
            out.write(data);
            out.flush();
            controller.appendConsole(">> Sent: " + PacketParser.formatBytes(data) + System.lineSeparator());
            return 1;
        } catch (IOException e) {
            controller.appendConsole("Error sending packet: " + e.getMessage() + System.lineSeparator());
            return -2;
        }
    }

    public void disconnect(){
        if (port == null)
            return;

        SerialPort disconnectedPort = port;
        port = null;
        connected = false;
        if (scheduled != null) {
            scheduled.cancel(false);
            scheduled = null;
        }
        if (heartbeatExecutor != null) {
            heartbeatExecutor.shutdownNow();
            heartbeatExecutor = null;
        }
        disconnectedPort.removeDataListener();
        boolean closed = disconnectedPort.closePort();
        packetParser.reset();
        NLRController controller = NLRController.INSTANCE;
        if (controller.rocket != null && controller.rocket.transmitter == this)
            controller.rocket = null;
        controller.appendConsole((closed ? "Serial port disconnected."
                : "Error closing serial port.") + System.lineSeparator());
    }

    public void connect(String selectedPort){
        NLRController controller = NLRController.INSTANCE;
        if (selectedPort == null) {
            controller.logError("Error: select a serial port first.");
            return;
        }

        SerialPort port = SerialPort.getCommPort(selectedPort);
        port.setBaudRate(9600);
        if (!port.openPort()) {
            controller.logError("Error: could not open " + selectedPort
                    + " (jSerialComm error " + port.getLastErrorCode()
                    + ", location " + port.getLastErrorLocation() + ").");
            return;
        }
        packetParser.reset();
        this.port = port;
        this.connected = true;
        heartbeatExecutor = Executors.newSingleThreadScheduledExecutor();
        runnable = new ELRSRunnable();
        port.addDataListener(new ELRSListener(this));
        scheduled = heartbeatExecutor.scheduleAtFixedRate(runnable, 0, 1, TimeUnit.SECONDS);
    }
}
