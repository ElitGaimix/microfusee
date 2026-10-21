module fr.elitgaimix.nlfc {
    requires javafx.controls;
    requires javafx.fxml;

    requires org.controlsfx.controls;
    requires org.kordamp.ikonli.javafx;
    requires com.fazecast.jSerialComm;

    opens fr.elitgaimix.nlfc to javafx.fxml;
    exports fr.elitgaimix.nlfc;
    exports fr.elitgaimix.nlfc.systems;
    opens fr.elitgaimix.nlfc.systems to javafx.fxml;
    exports fr.elitgaimix.nlfc.runnables;
    opens fr.elitgaimix.nlfc.runnables to javafx.fxml;
}