# Sherlock 13 – Version réseau SDL2

**Auteur : TOURE SEKOUBA – 2024**

Ce dépôt contient une adaptation réseau (TCP) et graphique (SDL2) du jeu de déduction **Sherlock 13** réalisée dans le cadre du module *Systèmes concurrents*.

| Fichier | Rôle |
|---------|------|
| `server.c` | Serveur TCP : gère la distribution des cartes, les questions et les accusations. |
| `sh13.c` | Client SDL2 : interface graphique + communication réseau (1 processus par joueur). |
| `Makefile` | Compilation automatique (`make`, `make clean`). |
| `SH13_*.png` | Cartes personnages. |
| `SH13_*_120x120.png` | Icônes des objets. |
| `gobutton.png`, `connectbutton.png` | Boutons de l’interface. |
| `sans.ttf` | Police utilisée dans l’interface. |
| `rapport_SH13.pdf` | Rapport synthétique expliquant la démarche. |

---

## 1. Prérequis

```bash
sudo apt update
sudo apt install build-essential pkg-config \
                 libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
```

Testé sous Ubuntu 22.04 / WSL.

---

## 2. Compilation

```bash
make          # construit   server  et  sh13
make clean    # supprime les exécutables
```

---

## 3. Lancer une partie locale (4 fenêtres)

```bash
./server 8080        # terminal A

# dans 4 autres terminaux
./sh13 127.0.0.1 8080 127.0.0.1 4000 Sekou
./sh13 127.0.0.1 8080 127.0.0.1 4001 Bob
./sh13 127.0.0.1 8080 127.0.0.1 4002 Thiery
./sh13 127.0.0.1 8080 127.0.0.1 4003 Mamadou
```

Chaque joueur se connecte avec un **port client différent**.

---

## 4. Contrôles dans l’interface SDL

| Action | Clics |
|--------|-------|
| Connexion | Bouton *CONNECT* en haut‑gauche. |
| Question globale | Cliquer un **objet** puis *GO*. |
| Question ciblée | Cliquer un **joueur**, puis un **objet**, puis *GO*. |
| Accusation | Cliquer le **suspect** dans la liste du bas (devient bleu), puis *GO*. |
| Hypothèse personnelle | Cliquer la petite case à cocher à droite du suspect. |

Le bouton **GO** n’apparaît que pour le joueur dont c’est le tour.

---

## 5. Fin de partie

La partie se termine à la **première accusation correcte** ; le serveur affiche le numéro du joueur gagnant et quitte. (La diffusion du nom avec un message `W` et la boîte de dialogue côté client ne sont **pas** implémentées dans cette version.)

---

## 6. Aspects techniques mis en œuvre

* **Sockets TCP** : communication client/serveur via un protocole ASCII minimal (messages `C`, `I`, `L`, `D`, `V`, `M`, `O`, `S`, `G`).
* **Processus multiples** : 1 serveur + 1 client par joueur.
* **Threads** : chaque client possède un thread réseau dédié qui publie les messages reçus dans un buffer partagé.
* **Mutex & synchronisation** : protège le buffer (`pthread_mutex_t`) et flag `volatile int synchro` pour signaler un message prêt.
* **SDL2** : rendu des textures, gestion de la souris, multi‑fenêtres (plusieurs instances).

---

## 7. Idées d’amélioration

- Diffuser le nom du vainqueur aux clients et afficher une boîte de dialogue.
- Implémenter la règle « dernier joueur n’ayant pas raté d’accusation gagne ».
- IA simple pour parties solo.
- Chat réseau intégré.

---

© 2024 – TOURE SEKOUBA. Basé sur le jeu **Sherlock 13** de Hope S. Hwang.

