# Protocole radio

## Format

```text
[AA] [longueur payload] [ID] [payload...] [CRC haut] [CRC bas]
```

Syncword, longueur et ID : **1 octet chacun**. Payload : **0 à 255 octets**.
Taille totale : **payload + 5 octets**.

## CRC-16/CCITT-FALSE

- Polynôme : `0x1021`, valeur initiale : `0xFFFF`.
- Sans réflexion ni XOR final ; octet fort envoyé en premier.
- Calcul sur l'en-tête et le payload, sans les deux octets du CRC.
- Un paquet dont le CRC est incorrect est rejeté.

Pour chaque octet : XOR avec le registre sur ses 8 bits de poids fort,
puis 8 décalages à gauche avec XOR `0x1021` si le bit sortant vaut 1.
Vérification : `123456789` en ASCII donne `0x29B1`.

## Messages

| ID | Payload | Rôle | État |
|---|---|---|---|
| `0x00` | Mode : `0x00`: Safe, `0x01` : Armed, `0x02` : GND (réserver pour l'éméteur au sol)| Heartbeat | Java envoie `TEST`, le STM32 répond sans payload avec le même ID |
| `0x01` |todo| Informations de vol | Prévu, payload à définir |
| `0x02` |todo| Position GPS | Prévu, payload à définir |
| `0x03` | cmd | Commands | Prévu, payload à définir (`0x00` pour la commande d'armement) |

Exemple de heartbeat et sa réponse, CRC inclus :

```text
Java  -> STM32 : AA 04 00 54 45 53 54 C9 24
STM32 -> Java  : AA 00 00 B6 C1
```
