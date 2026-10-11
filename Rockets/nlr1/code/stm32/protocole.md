# Documentation du protocole de communication radio

## Forme du packet envoyer : 
```mermaid
packet-beta
title Forme du packet (Bits par bits)
0-15: "Sync Word (2 octets)"
16-31: "Payload Length (2 octets)"
32-39: "Packet ID (1 octet)"
40-103: "Payload / Données (taille variante, 8 octets max)"
104-119: "Checksum CRC16 (2 octets)"
```

## Type de packets : 

### 0x00 : Heartbeat
    Permet de s'assurer que la communication est encore ouverte, envoyer seulement en mode 0 càd quand la fusée est au sol désarmée et qu'elle n'envoie pas de données
    
    Le payload fait 0 octets

### 0x01 : Information
    Envoie les information de vol (voltage de la batterie, pression, orientation du gyroscope etc...) Attention : la taille des truc est faite au pif, changera plus tard
```mermaid
packet-beta
title Forme du payload (Bits par bits)
0-7: "Voltage Batterie (1 octet)"
8-23: "Pression (2 octets)"
24-63: "Gyroscope (5 octets x y z et un truc au pif) "
```
### 0x02 : GPS
    Envoie la position gps