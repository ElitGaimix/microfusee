package fr.elitgaimix.nlfc;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.fxml.FXMLLoader;
import javafx.scene.Scene;
import javafx.stage.Stage;

import java.io.IOException;

public class NLRApplication extends Application {
    @Override
    public void start(Stage stage) throws IOException {
        FXMLLoader fxmlLoader = new FXMLLoader(
                NLRApplication.class.getResource("/fr/elitgaimix/nlfc/main_window.fxml"));
        Scene scene = new Scene(fxmlLoader.load());
        stage.setTitle("NLFlightController");
        stage.setScene(scene);
        stage.setOnCloseRequest(event -> {
            if (NLRController.INSTANCE != null)
                NLRController.INSTANCE.stop();
            Platform.exit();
        });
        stage.show();
    }
}