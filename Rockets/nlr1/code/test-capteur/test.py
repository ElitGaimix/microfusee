import sys
import math
import time
import serial
import serial.tools.list_ports
from PyQt6.QtCore import QTimer, pyqtSignal, QObject
from PyQt6.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QHBoxLayout, QPushButton, QLabel, QWidget
from PyQt6.QtGui import QVector3D
import pyqtgraph.opengl as gl

# --- GESTION DU PORT SÉRIE ---
class SerialReader(QObject):
    def __init__(self, port="COM3", baudrate=115200):
        super().__init__()
        self.ser = None
        try:
            self.ser = serial.Serial(port, baudrate, timeout=0.1)
            print(f"Connecté au port : {self.ser.name}")
        except Exception as e:
            print(f"Erreur connexion Série (Mode simulation activé) : {e}")

    def read_data(self):
        if self.ser and self.ser.in_waiting > 0:
            try:
                # --- SOLUTION ANTI-LATENCE ---
                last_line = None
                while self.ser.in_waiting > 0:
                    last_line = self.ser.readline()
                
                if last_line is None:
                    return None
                
                line = last_line.decode('utf-8').strip()
                
                # Sécurité : Si l'Arduino envoie du texte de calibration, on l'affiche
                if "[" in line or "->" in line or "---" in line:
                    print(f"[Arduino] {line}")
                    return None
                
                # Attendu : "pitch,roll,yaw"
                parts = line.split(',')
                if len(parts) == 3:
                    return float(parts[0]), float(parts[1]), float(parts[2])
            except Exception as e:
                pass
        return None

    def send_command(self, cmd):
        if self.ser and self.ser.is_open:
            self.ser.write(cmd.encode('utf-8'))
            print(f"Commande envoyée à la fusée : '{cmd}'")
        else:
            print(f"[Mode Simu] Commande virtuelle interceptée : '{cmd}'")


# --- INTERFACE GRAPHIQUE PRINCIPALE ---
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Ground Control - Télémétrie NLR2")
        self.resize(1000, 700)

        # Gestionnaire de port série
        self.serial_reader = SerialReader(port="COM3", baudrate=115200)

        # Variables pour mémoriser l'état et figer la position en cas de coupure
        self.has_received_serial = False
        self.last_pitch = 0.0
        self.last_roll = 0.0
        self.last_yaw = 0.0

        # --- Layout Principal ---
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        main_layout = QHBoxLayout(main_widget)

        # --- Panneau de Contrôle (Gauche) ---
        control_panel = QVBoxLayout()
        
        self.label_title = QLabel("=== DONNÉES EN DIRECT ===")
        self.label_title.setStyleSheet("font-weight: bold; font-size: 14px;")
        control_panel.addWidget(self.label_title)

        self.label_telemetry = QLabel("Pitch: 0.00°\nRoll: 0.00°\nYaw: 0.00°")
        self.label_telemetry.setStyleSheet("font-family: monospace; font-size: 16px; color: #1a1a1a;")
        control_panel.addWidget(self.label_telemetry)
        
        control_panel.addSpacing(20)

        # Boutons d'actions
        self.btn_center = QPushButton("Calibrer Gyroscope (c)")
        self.btn_center.setStyleSheet("background-color: #f39c12; color: white; padding: 10px; font-weight: bold;")
        self.btn_center.clicked.connect(lambda: self.serial_reader.send_command('c'))
        control_panel.addWidget(self.btn_center)

        self.btn_calibrate = QPushButton("Calibrer Accéléromètre (a)")
        self.btn_calibrate.setStyleSheet("background-color: #27ae60; color: white; padding: 10px; font-weight: bold;")
        self.btn_calibrate.clicked.connect(lambda: self.serial_reader.send_command('a'))
        control_panel.addWidget(self.btn_calibrate)
        
        control_panel.addStretch()
        main_layout.addLayout(control_panel, stretch=1)

        # --- Vue 3D (Droite) ---
        self.view3d = gl.GLViewWidget()
        self.view3d.setCameraPosition(distance=12, elevation=25, azimuth=45)
        main_layout.addWidget(self.view3d, stretch=3)

        grid = gl.GLGridItem()
        grid.scale(1, 1, 1)
        self.view3d.addItem(grid)

        # FIX ULTRA PROPRE : On définit la taille sous forme de QVector3D(Largeur_X, Profondeur_Y, Hauteur_Z).
        # En mettant la valeur 4 (ou 5) sur le troisième argument Z, la boîte grandit vers le haut nativement.
        self.rocket = gl.GLBoxItem(size=QVector3D(1, 1, 5), color=(0, 255, 255, 200))
        self.rocket.translate(-0.5, -0.5, 0) # On centre la base sur la grille
        self.view3d.addItem(self.rocket)

        # --- Boucle de rafraîchissement (100 Hz) ---
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_telemetry)
        self.timer.start(10) 

    def update_telemetry(self):
        data = self.serial_reader.read_data()
        
        if data:
            self.has_received_serial = True
            self.last_pitch, self.last_roll, self.last_yaw = data
            self.last_roll += 90
        elif not self.has_received_serial:
            # Mode Simulation s'il n'y a pas encore de flux série
            t = time.time()
            self.last_pitch = math.sin(t * 1.5) * 15.0
            self.last_roll = math.cos(t * 2.0) * 10.0
            self.last_yaw = (t * 12.0) % 360.0
        
        # Mise à jour de l'affichage texte
        self.label_telemetry.setText(
            f"Pitch : {self.last_pitch:>6.2f}°\n"
            f"Roll  : {self.last_roll:>6.2f}°\n"
            f"Yaw   : {self.last_yaw:>6.2f}°"
        )

        # --- MISE À JOUR DE L'ORIENTATION (ORDRE STRICT D'ORIGINE) ---
        self.rocket.transform().setToIdentity()
        self.rocket.translate(-0.5, -0.5, 0)   
        
        # Tes rotations d'origine s'appliquent directement sur le cube déjà vertical
        self.rocket.rotate(self.last_yaw, 0, 0, 1)   
        self.rocket.rotate(self.last_pitch, 0, 1, 0) 
        self.rocket.rotate(self.last_roll, 1, 0, 0)  


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())