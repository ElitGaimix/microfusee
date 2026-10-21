package fr.elitgaimix.nlfc;

import com.fazecast.jSerialComm.SerialPort;
import fr.elitgaimix.nlfc.runnables.GUIRunnable;
import fr.elitgaimix.nlfc.systems.Rocket;
import fr.elitgaimix.nlfc.systems.RocketMode;
import fr.elitgaimix.nlfc.utils.TerminalConsole;
import javafx.animation.Animation;
import javafx.animation.KeyFrame;
import javafx.animation.Timeline;
import javafx.application.Platform;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.fxml.FXML;
import javafx.scene.control.Button;
import javafx.scene.control.ChoiceBox;
import javafx.scene.control.Label;
import javafx.scene.control.TextArea;
import javafx.scene.layout.BorderPane;
import javafx.util.Duration;

import java.io.IOException;
import java.io.PrintStream;
import java.util.ArrayList;

public class NLRController {

    public static NLRController INSTANCE;
    private Timeline guiTimeline;
    private TerminalConsole out;

    public NLRController() {
        INSTANCE = this;
    }

    @FXML
    private void initialize() {
        GUIRunnable runnable = new GUIRunnable(this);
        guiTimeline = new Timeline(new KeyFrame(Duration.millis(500), event -> runnable.run()));
        guiTimeline.setCycleCount(Animation.INDEFINITE);
        guiTimeline.play();
        out =  new TerminalConsole();
        PrintStream stream;
        try {
            stream = out.open();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        System.setOut(stream);
        System.setErr(stream);
        System.out.println("Terminal start !");
    }

    public void stop(){
        if (guiTimeline != null)
            guiTimeline.stop();
        if(rocket != null)
            rocket.stop();
        if(out != null)
            out.close();
    }

    @FXML
    public ChoiceBox<String> com;

    @FXML
    public Button connect;

    @FXML
    private TextArea console;

    @FXML
    public Label comState;

    @FXML
    public Label nlrState;

    @FXML
    public BorderPane nlrBorderPane;

    public Rocket rocket;

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
    protected void onConnectButtonClick(){
        if (rocket != null) {
            rocket.transmitter.disconnect();
        }else {
            rocket = new Rocket(RocketMode.DISARMED);
            rocket.transmitter.connect(com.getValue());
            if (!rocket.transmitter.connected)
                rocket = null;
        }
    }

    public void appendConsole(String text) {
        System.out.println(text);
//        if (Platform.isFxApplicationThread()) {
//            console.appendText(text);
//        } else {
//            Platform.runLater(() -> console.appendText(text));
//        }
    }

    public void resetConsole(){
        System.out.println("--------");
        if (Platform.isFxApplicationThread()) {
            console.setText("");
        } else {
            Platform.runLater(() -> console.setText(""));
        }
    }

    public void logError(String message) {
        System.err.println(message);
//        if (Platform.isFxApplicationThread()) {
//            console.setText(message);
//        } else {
//            Platform.runLater(() -> console.setText(message));
//        }
    }
}
