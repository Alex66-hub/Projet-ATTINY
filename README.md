# Projet-ATTINY

Mini-jeu embarqué sur **ATtiny85** avec affichage **OLED 128×64**, tir au **buzzer**, contrôle **manuel** (boutons) ou **inertiel** (MPU6050 via I2C).

## Fonctionnalités
- Écran d’accueil + écran d’aide
- Menu **mode** : Manuel (boutons) / Gyroscope (MPU6050)
- Menu **niveau** : Facile / Moyen / Difficile
- Gameplay :
  - Déplacement horizontal du fusil
  - Tir vertical (un seul tir à la fois)
  - Fruits aléatoires (O, P, M, F, R) + fruits périmés (marqués `X`)
  - Score + niveau affichés en HUD

## Matériel
- ATtiny85
- OLED 128×64 (I2C)
- MPU6050 (I2C, adresse 0x68)
- Buzzer
- Boutons via **réseau de résistances** (lecture **ADC**)
- Alimentation (pile ou source externe selon montage)

## Bibliothèques
- `TinyWireM`
- `Tiny4kOLED`

## Brochage (code)
- `BP_GAUCHE  = 2`  (PB2 / A1)  → lecture ADC (menu mode + menu niveau)
- `BP_DROITE  = 3`  (PB3 / A3)  → lecture ADC (tir + déplacement)
- `BUZZER     = 6`  (PB1)       → `tone()`

> Remarque : selon ton core ATtiny, la correspondance exacte PBx / Dx peut varier. Le code se base sur les constantes définies ci-dessus.

## Boutons (ADC)
Les boutons sont reconnus par **plages** de valeurs ADC (avec tolérance).  
### BP_GAUCHE (A1)
- `480..530` → code `1` (Jaune)
- `530..630` → code `2` (Vert)
- `630..700` → code `3` (Rouge)
- `700..780` → code `4` (Bleu)

### BP_DROITE (A3)
- `480..530` → code `1` (Haut)
- `530..630` → code `2` (Droit)
- `630..700` → code `3` (Bas)
- `700..780` → code `4` (Gauche)

## Contrôles
### Menus
- **Menu mode** (via `BP_GAUCHE`) :
  - Jaune (1) → Manuel
  - Vert (2) → Gyroscope (MPU) + `mpuInit()`
- **Menu niveau** (via `BP_GAUCHE`) :
  - Jaune (1) → Facile
  - Rouge (3) → Moyen
  - Vert (2) → Difficile
- **Start** : `BP_DROITE` code 1 (Haut)

### En jeu
- **Tir** : `BP_DROITE` code 1 (Haut)
- **Déplacement manuel** :
  - `BP_DROITE` code 2 (Droit) → +6
  - `BP_DROITE` code 4 (Gauche) → -6
- **Déplacement gyro (MPU6050)** :
  - Lecture accel Y (`ACCEL_YOUT = 0x3D`)
  - Si `ay > MPU_SEUIL` → +`MPU_PAS`
  - Si `ay < -MPU_SEUIL` → -`MPU_PAS`

Bornes du fusil : `30 .. 120`.

## Règles du jeu
- Fruits possibles : `O P M F R`
- Fruit périmé : affiché avec un `X`
- Probabilité de fruit périmé : `15%`
- Max fruits simultanés : `5`
- Collision tir–fruit si :
  - `abs(fruit.x - positionTir) <= 6`
  - `abs(fruit.y - hauteurTir) <= 8`

### Score
- Points :
  - `O=1, P=2, M=3, F=4, R=5`
- Fruit périmé : **pénalité** (score -= points)
- Fruit valide : **gain** (score += points)

### Difficulté (niveaux)
Le niveau agit sur la **vitesse de descente** (`vitesseFruits`) :
- Facile : `1`
- Moyen  : `2`
- Difficile : `3`

Le déplacement vertical est cadencé par `INTERVALLE_FRUITS = 150 ms`.  
Le spawn suit une probabilité par boucle : `random < (5 + niveau*3)` (%).

## Machine à états
- `MENU_ACCUEIL` → affiche titre + aide (temporisé), puis `MENU_MODE`
- `MENU_MODE` → choix manuel/MPU, puis `MENU_PRINCIPAL`
- `MENU_PRINCIPAL` → choix niveau + Start, puis `JEU`
- `JEU` → boucle gameplay

## Compilation / Téléversement
1. Installer un core ATtiny compatible dans l’IDE Arduino (selon ton environnement).
2. Installer les bibliothèques `TinyWireM` et `Tiny4kOLED`.
3. Sélectionner le bon **board ATtiny85**, fréquence, et programmateur.
4. Compiler et téléverser.

## Paramètres à ajuster
- `MPU_SEUIL` : sensibilité du contrôle inertiel
- `MPU_PAS` : pas de déplacement en mode gyro
- Plages ADC des boutons (dépend du réseau de résistances)
- `INTERVALLE_FRUITS` et probabilité de spawn (difficulté globale)

## Limitations connues
- Les plages ADC peuvent se chevaucher si les résistances/tolérances varient : prévoir une tolérance (±Δ) et valider au multimètre/OLED.
- L’accueil utilise `delay()` (affichage temporisé).

## Améliorations possibles
- Ajouter une fin de partie (temps limité / vies) + écran score final
- Sauvegarde best score en EEPROM
- Anti-rebond plus robuste (filtrage temporel)
- Optimiser le rendu pour limiter l’effacement/écriture OLED

