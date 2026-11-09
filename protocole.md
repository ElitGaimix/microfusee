# Protocole radio

## Format

```text
[AA] [longueur payload] [ID] [payload...] [CRC haut] [CRC bas]
```

Syncword, longueur et ID : **1 octet chacun**. Payload : **0 à 64 octets avec RocketLRS** (le champ longueur est codé sur 1 octet).
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
| `0x02` | 21 octets, voir ci-dessous | Télémétrie GPS | NLR1 → sol, environ 1 Hz |
| `0x03` | cmd | Commands | Prévu, payload à définir (`0x00` pour la commande d'armement) |
| `0x04` | `[ID commande] [résultat...]` | Acquittement | Réponse à une commande (ex. `SERVO` → `[0x06] [angle]`) |
| `0x05` | `[0x00/0x01]` | LED | Allume / éteint la LED |
| `0x06` | `[angle 0..180]` | Servo | Consigne absolue |
| `0xFE` | vide | DFU | Passage en mode bootloader pour le stm32 |
| `0xF0` | 8 octets, voir ci-dessous | Rapport de liaison | Envoyé 1x/s par le TX et par le RX |
| `0xFF` | `[ID] [code]` | Erreur | Paquet inconnu ou invalide |

### `0x02` — télémétrie GPS

NLR1 émet ce paquet vers le sol pour chaque réponse UBX NAV-PVT complète
dont le checksum est valide. Une réponse est demandée toutes les **1 s**.
Aucun paquet n'est émis si le GPS ne répond pas ou si sa trame est corrompue.
Une réponse sans fix est transmise également, pour suivre l'acquisition.

Payload fixe de **21 octets**, soit **26 octets** avec l'en-tête et le CRC.
Offsets comptés depuis le début du payload. Les quatre entiers de 32 bits
sont **signés, en complément à deux, little-endian** (octet faible d'abord).
Le CRC reste envoyé octet fort d'abord, comme pour les autres messages.

| Offset | Taille | Champ | Format / unité |
|---|---|---|---|
| 0 | 1 | Heure UTC | `uint8`, 0..23 |
| 1 | 1 | Minute UTC | `uint8`, 0..59 |
| 2 | 1 | Seconde UTC | `uint8`, 0..60 (seconde intercalaire) |
| 3 | 1 | Type de fix | `uint8`, valeur UBX `fixType` |
| 4 | 1 | Validité | bit 0 : heure UTC valide ; bit 1 : `gnssFixOK` ; bits 2..7 à zéro |
| 5 | 4 | Longitude | `int32`, degrés × 10⁷ ; négatif à l'ouest |
| 9 | 4 | Latitude | `int32`, degrés × 10⁷ ; négatif au sud |
| 13 | 4 | Altitude au niveau moyen de la mer | `int32`, millimètres (`hMSL`) |
| 17 | 4 | Vitesse au sol 2D | `int32`, millimètres/seconde (`gSpeed`) |

Types de fix : `0` aucun fix, `1` estimation seule (dead reckoning),
`2` fix 2D, `3` fix 3D, `4` GNSS + estimation, `5` heure seule.
Le type de fix seul ne garantit pas une solution valide : vérifier également
le bit 1 de validité. L'altitude nécessite un fix 3D valide (type `3` ou `4`).
Le bit 0 doit être vérifié avant d'afficher l'heure. Ces indicateurs décrivent
la mesure reçue ; le logiciel au sol doit aussi suivre l'âge du dernier paquet.

Conversions : coordonnées / `10 000 000` → degrés ; altitude / `1 000` → m ;
vitesse / `1 000` → m/s, ou × `0,0036` → km/h. Aucune conversion flottante
n'est faite à bord. L'altitude est celle de la mer, pas la hauteur au-dessus
du sol ; la vitesse est horizontale, pas une vitesse verticale ou 3D.

Source des champs et unités : [spécification UBX NAV-PVT u-blox](https://content.u-blox.com/sites/default/files/products/documents/u-blox7-V14_ReceiverDescriptionProtocolSpec_%28GPS.G7-SW-12001%29_Public.pdf).

### `0xF0` — rapport de liaison

Le module radio injecte ce paquet une fois par seconde dans l'UART local :
le TX l'envoie donc à Java, le RX au STM32. Les champs montants sont
**toujours mesurés par le module qui émet** ; les champs descendants viennent
de la mesure du **module d'en face**, transmise par radio dans son propre
`0xF0`.

| Octet | Champ | Unité | Source |
|---|---|---|---|
| 0 | RSSI montant | magnitude positive, donc `-dBm` (0 = pas de mesure) | mesure locale (RX → TX) |
| 1 | SNR montant | signé, en dB entiers | mesure locale |
| 2 | Qualité de lien montante | % | mesure locale |
| 3 | RSSI descendant | magnitude positive, donc `-dBm` (0 = pas de mesure) | mesure du module d'en face (TX → RX) |
| 4 | SNR descendant | signé, en dB entiers | mesure du module d'en face |
| 5 | Qualité de lien descendante | % | mesure du module d'en face |
| 6 | Puissance TX | niveau ELRS | TX |
| 7 | Mode RF | index du débit radio | local |

Un récepteur ne mesure que le sens montant : s'il n'a pas reçu de `0xF0` du
TX depuis 3 s, il laisse les octets 3 à 5 à zéro plutôt que d'annoncer un
descendant périmé.

Exemple (`AA 08 F0 48 FB 62 3C 07 5B 03 08 <CRC>` : 72 → -72 dBm, SNR -5 dB).


Exemple de heartbeat et sa réponse, CRC inclus :

```text
Java  -> STM32 : AA 04 00 54 45 53 54 C9 24
STM32 -> Java  : AA 00 00 B6 C1
```
