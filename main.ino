#include <TinyWireM.h>
#include <Tiny4kOLED.h>

// Définition des broches
#define BP_GAUCHE  2  // Broche PB2 (A1)
#define BP_DROITE  3  // Broche PB3 (A3)
#define BUZZER     1  // Broche PB1

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

// Variables pour éviter le scintillement
int anciennePositionFusil = 64;

// Vitesse des fruits
int vitesseFruits = 1;

uint8_t touche_gauche() {
  int val = analogRead(BP_GAUCHE);
  if (val > 480 && val < 530) return 1;  // Bouton Jaune
  if (val > 530 && val < 630) return 2;  // Bouton Vert
  if (val > 630 && val < 700) return 3;  // Bouton Rouge
  if (val > 700 && val < 780) return 4;  // Bouton Bleu
  return 0;
}

uint8_t touche_droite() {
  int val = analogRead(BP_DROITE);
  if (val > 480 && val < 530) return 1;  // Bouton Haut
  if (val > 530 && val < 630) return 2;  // Bouton Droit
  if (val > 630 && val < 700) return 3;  // Bouton Bas
  if (val > 700 && val < 780) return 4;  // Bouton Gauche
  return 0;
}

void gererDeplacementManuel() {
  if (touche_droite() == 2) {  // Bouton Droit
    positionFusil += 4;
    if (positionFusil > 120) positionFusil = 120;
  }
  if (touche_droite() == 4) {  // Bouton Gauche
    positionFusil -= 4;
    if (positionFusil < 0) positionFusil = 0;
  }
}

void genererFruit() {
  if (nbFruits >= 5) return;
  int idxType = random(0, nbTypesFruits);
  bool perime = (random(0, 100) < 15);
  fruits[nbFruits].type = typesFruits[idxType];
  fruits[nbFruits].x = random(5, 123);  // Position aléatoire en x (5-123 pour éviter les bords)
  fruits[nbFruits].y = 0;               // Position initiale en haut
  fruits[nbFruits].perime = perime;
  fruits[nbFruits].visible = true;
  nbFruits++;
}

void afficherFruits() {
  for (int i = 0; i < nbFruits; i++) {
    if (fruits[i].visible) {
      oled.setCursor(fruits[i].x, fruits[i].y / 8);  // Conversion pixels -> caractères (1x8)
      oled.print(fruits[i].type);
      if (fruits[i].perime) {
        oled.setCursor((fruits[i].x + 6), fruits[i].y / 8);
        oled.print("X");
      }
    }
  }
}

void deplacerFruits() {
  for (int i = 0; i < nbFruits; i++) {
    if (fruits[i].visible) {
      fruits[i].y += vitesseFruits;  // Vitesse ajustable
      // delay(100);
      if (fruits[i].y >= 64) {       // Si le fruit atteint le bas de l'écran
        fruits[i].visible = false;
        for (int j = i; j < nbFruits - 1; j++) {
          fruits[j] = fruits[j + 1];
        }
        nbFruits--;
      }
    }
  }
}

void effacerTir() {
  if (tirEnCours && hauteurTir >= 0 && hauteurTir < 64) {
    oled.setCursor(positionTir, hauteurTir);
    oled.print(" ");
  }
}

void tirer() {
  if (tirEnCours) return;
  tirEnCours = true;
  positionTir = positionFusil;
  hauteurTir = 56;  // Position initiale en bas de l'écran
  tone(BUZZER, 1000, 20);
}

void gererTir() {
  if (!tirEnCours) return;

  effacerTir();  // Efface l'ancienne position du tir

  hauteurTir -= 4;  // Vitesse du tir
  if (hauteurTir < 0) {
    tirEnCours = false;
    return;
  }

  oled.setCursor(positionTir, hauteurTir/3);
  oled.print("*");

  for (int i = 0; i < nbFruits; i++) {
    if (fruits[i].visible &&
        abs(fruits[i].x - positionTir) <= 6 &&  // Tolérance en x
        abs(fruits[i].y - hauteurTir) <= 8) {  // Tolérance en y
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

void afficherFusil(int position) {
  // Efface l'ancienne position du fusil
  oled.setCursor(anciennePositionFusil, 7);
  oled.print(" ");

  // Dessine le fusil à la nouvelle position
  oled.setCursor(position, 7);
  oled.print("^");

  anciennePositionFusil = position;
}

void afficherAccueil() {
  oled.clear();
  oled.setCursor(4, 2);
  oled.print(F("ECE Paris"));
  oled.setCursor(6, 4);
  oled.print(F("Projet ATTiny85"));
  delay(2000);
  etat = MENU_PRINCIPAL;
}

int B = 0;

void afficherMenuPrincipal() {
  if (B==1){
    oled.clear();
  }
  // oled.clear();
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
  if (choix == 1) {
    niveau = 1;
    vitesseFruits = 1;
    B = 1;
  }
  if (choix == 3) {
    niveau = 2;
    vitesseFruits = 1;
    B = 1;
  }
  if (choix == 2) {
    niveau = 3;
    vitesseFruits = 3;
    B = 1;
  }

  // Bouton Haut pour démarrer
  if (touche_droite() == 1) { 
    etat = JEU;
    B = 1;
    // oled.clear();
  }
}

void lancerJeu() {
  if (random(0, 100) < (5 + niveau * 3)){
    // delay(50);
    genererFruit();  // Génération aléatoire des fruits
  }
 
  deplacerFruits();

  if (touche_droite() == 1 && !tirEnCours) {  // Bouton Haut pour tirer
    tirer();
    delay(150);  // Anti-rebond
  }

  gererTir();
  gererDeplacementManuel();
  afficherFruits();
  afficherFusil(positionFusil);

  // Affichage du score et du niveau
  oled.setCursor(0, 0);
  oled.print(F("S:"));
  oled.print(score);
  oled.setCursor(100, 0);
  oled.print(F("N:"));
  oled.print(niveau);
}

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

void loop() {
  if (etat == MENU_ACCUEIL) afficherAccueil();
  else if (etat == MENU_PRINCIPAL) afficherMenuPrincipal();
  else if (etat == JEU) lancerJeu();
}
