#include <TinyWireM.h>
#include <Tiny4kOLED.h>

// Définition des broches
#define BP_GAUCHE  2
#define BP_DROITE  3
#define BUZZER     1

// États du jeu
#define MENU_ACCUEIL   0
#define MENU_PRINCIPAL 1
#define JEU            2

// Types de fruits
const char* typesFruits = "OPMFR";
const int nbTypesFruits = 5;

// Structure pour un fruit
struct Fruit {
  char type;
  int x, y;      
  bool perime;
  bool visible;
};

// Variables globales
Fruit fruits[5];
int nbFruits = 0;
int score = 0;
int niveau = 1;
bool modeAuto = false;
int positionFusil = 64; 
bool tirEnCours = false;
int positionTir = 0;
int hauteurTir = 0;
int etat = MENU_ACCUEIL;

int anciennePositionFusil = 64;
int vitesseFruits = 1;

// --- TIMERS (AJOUT) ---
unsigned long dernierSpawn = 0;
unsigned long dernierMove = 0;
unsigned long dernierTir = 0;

int delaiSpawn = 900;
int delaiMove = 140;
int delaiTir = 80;

// ---------------- INPUT ----------------
uint8_t touche_gauche() {
  int val = analogRead(BP_GAUCHE);
  if (val > 480 && val < 530) return 1;
  if (val > 530 && val < 630) return 2;
  if (val > 630 && val < 700) return 3;
  if (val > 700 && val < 780) return 4;
  return 0;
}

uint8_t touche_droite() {
  int val = analogRead(BP_DROITE);
  if (val > 480 && val < 530) return 1;
  if (val > 530 && val < 630) return 2;
  if (val > 630 && val < 700) return 3;
  if (val > 700 && val < 780) return 4;
  return 0;
}

// ---------------- DEPLACEMENT ----------------
void gererDeplacementManuel() {
  if (touche_droite() == 2) {
    positionFusil += 4;
    if (positionFusil > 120) positionFusil = 120;
  }
  if (touche_droite() == 4) {
    positionFusil -= 4;
    if (positionFusil < 0) positionFusil = 0;
  }
}

// ---------------- GENERATION ----------------
void genererFruit() {
  if (nbFruits >= 5) return;

  int newX;
  bool tropProche;

  do {
    tropProche = false;
    newX = random(5, 123);

    for (int i = 0; i < nbFruits; i++) {
      if (abs(fruits[i].x - newX) < 15) {
        tropProche = true;
        break;
      }
    }
  } while (tropProche);

  int idxType = random(0, nbTypesFruits);
  bool perime = (random(0, 100) < 15);

  fruits[nbFruits].type = typesFruits[idxType];
  fruits[nbFruits].x = newX;
  fruits[nbFruits].y = 0;
  fruits[nbFruits].perime = perime;
  fruits[nbFruits].visible = true;

  nbFruits++;
}

// ---------------- AFFICHAGE ----------------
void afficherFruits() {
  for (int i = 0; i < nbFruits; i++) {
    if (fruits[i].visible) {
      oled.setCursor(fruits[i].x, fruits[i].y / 8);
      oled.print(fruits[i].type);

      if (fruits[i].perime) {
        oled.setCursor(fruits[i].x + 6, fruits[i].y / 8);
        oled.print("X");
      }
    }
  }
}

// ---------------- MOUVEMENT ----------------
void deplacerFruits() {
  if (millis() - dernierMove < delaiMove) return;
  dernierMove = millis();

  for (int i = 0; i < nbFruits; i++) {
    if (fruits[i].visible) {
      fruits[i].y += vitesseFruits;

      if (fruits[i].y >= 64) {
        fruits[i].visible = false;

        for (int j = i; j < nbFruits - 1; j++) {
          fruits[j] = fruits[j + 1];
        }
        nbFruits--;
        i--;
      }
    }
  }
}

// ---------------- TIR ----------------
void effacerTir() {
  if (tirEnCours) {
    oled.setCursor(positionTir, hauteurTir / 8);
    oled.print(" ");
  }
}

void tirer() {
  if (tirEnCours) return;

  tirEnCours = true;
  positionTir = positionFusil;
  hauteurTir = 56;

  tone(BUZZER, 1000, 20);
}

void gererTir() {
  if (!tirEnCours) return;

  if (millis() - dernierTir < delaiTir) return;
  dernierTir = millis();

  effacerTir();

  hauteurTir -= 6;

  if (hauteurTir < 0) {
    tirEnCours = false;
    return;
  }

  oled.setCursor(positionTir, hauteurTir / 8);
  oled.print("*");

  for (int i = 0; i < nbFruits; i++) {
    if (fruits[i].visible &&
        abs(fruits[i].x - positionTir) <= 6 &&
        abs(fruits[i].y - hauteurTir) <= 10) {

      fruits[i].visible = false;
      tirEnCours = false;

      int points = 0;
      switch (fruits[i].type) {
        case 'R': points = 5; break;
        case 'O': points = 1; break;
        case 'P': points = 2; break;
        case 'M': points = 3; break;
        case 'F': points = 4; break;
      }

      if (fruits[i].perime) score -= points;
      else score += points;

      tone(BUZZER, 500, 50);

      for (int j = i; j < nbFruits - 1; j++) {
        fruits[j] = fruits[j + 1];
      }
      nbFruits--;
      break;
    }
  }
}

// ---------------- FUSIL ----------------
void afficherFusil(int position) {
  oled.setCursor(anciennePositionFusil, 7);
  oled.print(" ");

  oled.setCursor(position, 7);
  oled.print("^");

  anciennePositionFusil = position;
}

// ---------------- MENUS ----------------
void afficherAccueil() {
  oled.clear();
  oled.setCursor(4, 2);
  oled.print(F("ECE Paris"));
  oled.setCursor(6, 4);
  oled.print(F("Projet ATTiny85"));
  delay(2000);
  etat = MENU_PRINCIPAL;
}

void afficherMenuPrincipal() {
  oled.clear();
  oled.setCursor(0, 0);
  oled.print(F("Niveau:"));
  oled.setCursor(0, 2);
  oled.print(F("J: Facile"));
  oled.setCursor(0, 3);
  oled.print(F("R: Moyen"));
  oled.setCursor(0, 4);
  oled.print(F("V: Difficile"));
  oled.setCursor(0, 6);
  oled.print(F("Haut pour Start"));

  uint8_t choix = touche_gauche();

  if (choix == 1) { niveau = 1; vitesseFruits = 1; }
  if (choix == 3) { niveau = 2; vitesseFruits = 2; }
  if (choix == 2) { niveau = 3; vitesseFruits = 3; }

  if (touche_droite() == 1) {
    etat = JEU;
    oled.clear();
  }
}

// ---------------- JEU ----------------
void lancerJeu() {

  if (millis() - dernierSpawn > (delaiSpawn - niveau * 120)) {
    dernierSpawn = millis();
    genererFruit();
  }

  deplacerFruits();

  if (touche_droite() == 1 && !tirEnCours) {
    tirer();
  }

  gererTir();
  gererDeplacementManuel();

  oled.clear();

  afficherFruits();
  afficherFusil(positionFusil);

  oled.setCursor(0, 0);
  oled.print(F("S:"));
  oled.print(score);

  oled.setCursor(100, 0);
  oled.print(F("N:"));
  oled.print(niveau);
}

// ---------------- SETUP ----------------
void setup() {
  pinMode(BP_GAUCHE, INPUT);
  pinMode(BP_DROITE, INPUT);
  pinMode(BUZZER, OUTPUT);

  oled.begin(128, 64, sizeof(tiny4koled_init_128x64br), tiny4koled_init_128x64br);
  oled.setFont(FONT6X8);
  oled.on();
  oled.clear();

  randomSeed(analogRead(0));
}

// ---------------- LOOP ----------------
void loop() {
  if (etat == MENU_ACCUEIL) afficherAccueil();
  else if (etat == MENU_PRINCIPAL) afficherMenuPrincipal();
  else if (etat == JEU) lancerJeu();
}
