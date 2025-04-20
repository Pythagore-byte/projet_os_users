# Sherlock 13 – Version réseau SDL2

Ce dépôt propose une adaptation réseau (TCP) et graphique (SDL2) du jeu de déduction **Sherlock 13**. Il contient :

| Fichier        | Rôle                                                                          |
|----------------|-------------------------------------------------------------------------------|
| `server.c`     | Serveur TCP : gère la distribution des cartes, les questions, les accusations. |
| `sh13.c`       | Client SDL2 : interface graphique + communication avec le serveur.            |
| `SH13_*.png`   | Cartes personnages.                                                           |
| `SH13_*_120x120.png` | Icônes des objets.                                                      |
| `gobutton.png` / `connectbutton.png` | Boutons interface.                                      |
| `sans.ttf`     | Police TrueType utilisée dans l'interface.                                    |

---

## 1. Pré‑requis

| Système | Testé sur | Notes |
|---------|-----------|-------|
| Linux / WSL (Ubuntu) | 22.04 | SDL2 + dev packages installés |

```bash
sudo apt update
sudo apt install build-essential pkg-config \
                 libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
```

---

## 2. Compilation

### Serveur

```bash
gcc server.c -o server
```

### Client

```bash
gcc sh13.c -o sh13 \
    $(pkg-config --cflags --libs sdl2 SDL2_image SDL2_ttf) \
    -pthread
```

---

## 3. Lancer une partie locale (4 fenêtres sur le même PC)

1. **Démarrer le serveur** sur le port 8080 :

   ```bash
   ./server 8080
   ```

2. **Ouvrir quatre terminaux** et lancer :

   ```bash
   # terminal 1
   ./sh13 127.0.0.1 8080 127.0.0.1 4000 Alice

   # terminal 2
   ./sh13 127.0.0.1 8080 127.0.0.1 4001 Bob

   # terminal 3
   ./sh13 127.0.0.1 8080 127.0.0.1 4002 Carol

   # terminal 4
   ./sh13 127.0.0.1 8080 127.0.0.1 4003 Dave
   ```

Chaque port client doit être différent, et chaque nom de joueur unique.

---

## 4. Lancer en réseau LAN

* Serveur : `./server 8080` sur la machine A (IP, par ex. 192.168.1.42).
* Chaque joueur sur sa propre machine B/C/D/E :

  ```bash
  ./sh13 192.168.1.42 8080 <IP_B> <PORT_B> <Nom>
  ```

Le port client (`<PORT_B>`) doit être ouvert en TCP sur la machine cliente pour recevoir les messages du serveur.

---

## 5. Contrôles dans l’interface SDL

| Action | Clics requis |
|--------|--------------|
| **Se connecter** | Bouton *CONNECT* en haut‑gauche. |
| **Question globale** | Sélectionner un **objet** (icône) puis *GO*. |
| **Question ciblée** | Sélectionner un **joueur** à gauche, un **objet**, puis *GO*. |
| **Accusation** | Cliquer le **nom du suspect** (devient bleu), puis *GO*. |
| **Noter une hypothèse** | Clic dans la petite colonne croix (250‑300 px) à côté du suspect. |

Le bouton **GO** n’apparaît que pour le joueur dont c’est le tour.

---

## 6. Fin de partie

* **Accusation correcte** : le serveur envoie `W <id> <nom>`, affiche « Victoire : <nom> », ferme la connexion ; chaque client affiche une boîte SDL « Fin de partie » avec le gagnant.
* **Accusation incorrecte** : le tour passe simplement au joueur suivant.

*(La variante « tout le monde a raté → dernier survivant gagne » n’est pas encore codée.)*

---

## 7. Détails techniques

### Messages TCP

| Code | Émetteur → Récepteur | Signification |
|------|----------------------|---------------|
| `C ip port name` | Client → Serveur | connexion. |
| `I id` | Serveur → Client (individuel) | Id attribué. |
| `L n1 n2 n3 n4` | Serveur → Tous | Liste joueurs. |
| `D c1 c2 c3` | Serveur → Client | Ses 3 cartes. |
| `V i j v` | Serveur → Clients | Valeur (ou `100` = *). |
| `M id` | Serveur → Tous | Prochain joueur à jouer. |
| `O id obj` | Client → Serveur | Question globale. |
| `S id joueur obj` | Client → Serveur | Question ciblée. |
| `G id suspect` | Client → Serveur | Accusation. |
| `W id name` | Serveur → Tous | Victoire. |

### Threading

* Le client lance un petit thread TCP pour écouter son port (`gClientPort`) et poste les données dans `gbuffer`.
* Le thread principal SDL consomme `gbuffer` (via `synchro`).

### Ajouts récents

* Affichage du **nom du vainqueur** (`W`) + popup SDL.
* Bouton **GO** uniquement désactivé quand une action valide part réellement.
* Variable globale `gWindow` / `gRenderer` pour accès unifié aux appels SDL.
* Ligne de séparation superflue supprimée (trait en‑dessous d’Irène Adler).

---

## 8. To‑do / idées d’amélioration

- Implémenter la condition *« tous ont raté une accusation → dernier joueur gagne »*.
- Sauvegarde / chargement d’une partie.
- Musiques et SFX.
- Animations d’apparition de cartes/icônes avec *SDL_Fade*.
- Packaging *.deb* et script `run.sh` automatique.

---

© 2024 – Adaptation réseau & SDL par <TOURE SEKOUBA>. Basé sur le jeu **Sherlock 13** de Hope S. Hwang.

