import serial
import threading
import time

# --- CONFIGURATION ---
# Remplace 'COM3' par le vrai port de ton Micro TX V2 (regarde dans le Gestionnaire de périphériques)
SERIAL_PORT = 'COM3' 
BAUD_RATE = 115200  # Doit être identique à la vitesse configurée dans l'ESP32 et le Crossfire

def lecture_telemetrie(ser):
    """Fonction qui tourne en arrière-plan pour lire la télémétrie"""
    print("[Système] Écoute de la télémétrie activée...")
    while ser.is_open:
        try:
            if ser.in_waiting > 0:
                # Lecture de la ligne envoyée par la fusée
                ligne = ser.readline().decode('utf-8', errors='ignore').strip()
                
                # Si c'est notre paquet de télémétrie
                if ligne.startswith("$TELEMETRIE"):
                    # On sépare les données grâce aux virgules
                    donnees = ligne.split(',')
                    if len(donnees) >= 3:
                        altitude = donnees[1]
                        batterie = donnees[2]
                        print(f"\n[TÉLÉMÉTRIE] Altitude: {altitude} m | Batterie: {batterie} V")
                        print("Entrez une commande (BOOM / RESET / QUIT) : ", end="", flush=True)
                
                # Si c'est un message de statut (ex: confirmation du parachute)
                elif ligne.startswith("STATUT"):
                    print(f"\n[ALERTE STATUT] {ligne}")
                    print("Entrez une commande (BOOM / RESET / QUIT) : ", end="", flush=True)
                    
        except Exception as e:
            print(f"\n[Erreur Lecture] {e}")
            break
        time.sleep(0.05) # Petite pause pour ne pas surcharger le processeur

def main():
    try:
        # Ouverture du port série du Micro TX
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"Connecté au Micro TX sur le port {SERIAL_PORT}")
        print("Commandes disponibles :")
        print(" - 'BOOM'  : Déclencher le parachute")
        print(" - 'RESET' : Éteindre la LED / Réinitialiser")
        print(" - 'QUIT'  : Fermer le programme")
        print("-" * 40)

        # Lancement du thread de lecture en arrière-plan
        thread_ecoute = threading.Thread(target=lecture_telemetrie, args=(ser,), daemon=True)
        thread_ecoute.start()

        # Boucle principale : attente des commandes de l'utilisateur
        while True:
            commande = input("Entrez une commande (BOOM / RESET / QUIT) : ").strip()

            if commande.upper() == "QUIT":
                print("Fermeture du programme...")
                break
            elif commande.upper() in ["BOOM", "RESET"]:
                # On envoie la commande suivie d'un saut de ligne '\n' car l'ESP32 utilise readStringUntil('\n')
                paquet = commande.upper() + "\n"
                ser.write(paquet.encode('utf-8'))
                print(f"[Ordre] '{commande.upper()}' envoyé à la fusée.")
            else:
                print("[Erreur] Commande inconnue. Utilisez BOOM, RESET ou QUIT.")

    except serial.SerialException as e:
        print(f"Impossible d'ouvrir le port {SERIAL_PORT}. Vérifie le branchement et le numéro de port. ({e})")
    except KeyboardInterrupt:
        print("\nProgramme interrompu.")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Connexion série fermée proprement.")

if __name__ == "__main__":
    main()