package fr.elitgaimix.nlfc;


import javafx.scene.layout.Border;
import javafx.scene.paint.Color;
import javafx.application.Platform;

import java.util.concurrent.TimeUnit;

public class ELRSRunnable implements Runnable {

    public boolean connectionState = false;

    private volatile long lastHeartbeat = System.nanoTime();

    public void onHeartbeatReceived() {
        lastHeartbeat = System.nanoTime();
        connectionState = true;
        Platform.runLater(() -> {
            NLRController.INSTANCE.nlrState.setText("Connected");
            NLRController.INSTANCE.nlrState.setTextFill(Color.GREEN);
            NLRController.INSTANCE.nlrBorderPane.setStyle("-fx-border-color: green;");
        });
    }

    @Override
    public void run() {
        long elapsed = System.nanoTime() - lastHeartbeat;

        if (elapsed > TimeUnit.SECONDS.toNanos(1)) {
            System.out.println("Heartbeat perdu !");
            connectionState = false;
            Platform.runLater(() -> {
                NLRController.INSTANCE.nlrState.setText("Disconnected");
                NLRController.INSTANCE.nlrState.setTextFill(Color.RED);
                NLRController.INSTANCE.nlrBorderPane.setStyle("-fx-border-color: red;");
            });
        }
        NLRController.INSTANCE.rocket.sendHeartBeat();
    }
}
