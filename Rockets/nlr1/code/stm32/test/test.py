
import serial
import time
from threading import Event, Thread

PORT = "COM4"
BAUDRATE = 9600

SYNCWORD = 0xAA
PACKET_REQUEST = 0x00
PACKET_ACK = 0x03


class Packet:
    def __init__(self):
        self.reset()

    def reset(self):
        self.syncword = 0
        self.payload_len = 0
        self.packet_id = 0
        self.payload = bytearray()
        self.checksum = 0
        self.actual_byte = 0

    def append_byte(self, byte: int) -> bool:
        # Recherche du début de paquet
        if self.actual_byte == 0:
            if byte != SYNCWORD:
                return False
            self.syncword = byte

        elif self.actual_byte == 1:
            self.payload_len = byte

        elif self.actual_byte == 2:
            self.packet_id = byte

        elif self.actual_byte < 3 + self.payload_len:
            self.payload.append(byte)

        else:
            self.checksum = byte
            self.actual_byte += 1
            return True

        self.actual_byte += 1
        return False

    def to_bytes(self) -> bytes:
        return bytes([
            self.syncword,
            self.payload_len,
            self.packet_id
        ]) + bytes(self.payload) + bytes([self.checksum])


def build_packet(packet_id: int, payload: bytes = b"") -> bytes:
    if len(payload) > 255:
        raise ValueError("Payload trop long")

    # Checksum simulé : toujours 0x00
    return bytes([
        SYNCWORD,
        len(payload),
        packet_id
    ]) + payload + b"\x00"


def on_packet_received(packet: Packet):
    print("\n<< Paquet reçu")
    print(f"   Syncword     : 0x{packet.syncword:02X}")
    print(f"   Payload len  : {packet.payload_len}")
    print(f"   Packet ID    : 0x{packet.packet_id:02X}")
    print(f"   Payload      : {packet.payload.hex(' ') or '(vide)'}")
    print(f"   Checksum     : 0x{packet.checksum:02X}")

    if packet.packet_id == PACKET_ACK:
        print("   ACK reçu : requête acquittée")
    else:
        print("   Type de paquet inconnu")


def lire_port(ser: serial.Serial, arret: Event):
    packet = Packet()

    while not arret.is_set():
        try:
            donnees = ser.read(1)
        except serial.SerialException as erreur:
            print(f"Erreur de lecture série : {erreur}")
            arret.set()
            return

        if not donnees:
            continue

        if packet.append_byte(donnees[0]):
            on_packet_received(packet)
            packet.reset()


def main():
    arret = Event()
    lecteur = None

    try:
        with serial.Serial(
            port=PORT,
            baudrate=BAUDRATE,
            timeout=0.1,
            write_timeout=2
        ) as ser:

            print(f"Port connecté : {PORT} à {BAUDRATE} bauds")
            print("Envoi des requêtes... (Ctrl+C pour arrêter)\n")

            lecteur = Thread(
                target=lire_port,
                args=(ser, arret),
                daemon=True
            )
            lecteur.start()

            prochain_envoi = time.monotonic()

            while not arret.is_set():
                maintenant = time.monotonic()

                if maintenant >= prochain_envoi:
                    paquet = build_packet(PACKET_REQUEST,b"TEST")

                    ser.write(paquet)
                    ser.flush()

                    print(f">> Requête envoyée : {paquet.hex(' ')}")

                    prochain_envoi = maintenant + 2

                arret.wait(
                    max(0, prochain_envoi - time.monotonic())
                )

    except serial.SerialException as erreur:
        print(f"Erreur série : {erreur}")

    except KeyboardInterrupt:
        print("\nTest arrêté.")

    finally:
        arret.set()
        if lecteur is not None:
            lecteur.join(timeout=1)


if __name__ == "__main__":
    main()
