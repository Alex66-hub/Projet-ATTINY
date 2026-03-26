#include <TinyWireM.h>
#include <Tiny4kOLED.h>

// Définition des broches
#define BP_GAUCHE  2   // PB2 / A1
#define BP_DROITE  3   // PB3 / A3
#define BUZZER     6   // PB1

#define MPU_ADDR        0x68
#define MPU_PWR_MGMT_1  0x6B

// ── MODIFIÉ : on lit l'axe Y (Pitch) = registres 0x3D/0x3E ──────────────────
//    Axe X (Roll)  = 0x3B  → inclinaison sur le côté  (pencher l'épaule)
//    Axe Y (Pitch) = 0x3D  → inclinaison avant/arrière (pencher en avant/arrière)
#define MPU_ACCEL_YOUT  0x3D
#define MPU_SEUIL  1500   // LSB (±2g → 16384 LSB/g). Ajustez si trop/pas assez sensible
#define MPU_PAS    4      

#define MENU_ACCUEIL    0
#define MENU_PRINCIPAL  1
#define MENU_MODE       2
#define JEU             3

// Types de fruits
const char* typesFruits  = "OPMFR";
const int   nbTypesFruits = 5;

// Structure pour un fruit
struct Fruit {
  char type;
  int  x, y;
  bool perime;
  bool visible;
};

// Variables globales
Fruit fruits[5];
int  nbFruits    = 0;
int  score       = 0;
int  niveau      = 1;
bool modeAuto    = false;

int  positionFusil         = 72;
// Variables pour éviter le scintillement
int  anciennePositionFusil = 72;

bool tirEnCours  = false;
int  positionTir = 0;
int  hauteurTir  = 0;

int  etat          = MENU_ACCUEIL;
int  vitesseFruits = 1;

#define INTERVALLE_FRUITS 150
unsigned long dernierDeplacementFruits = 0;

// Fonction d'initalisation du MPU6050
void mpuInit() {
  TinyWireM.beginTransmission(MPU_ADDR);
  TinyWireM.write(MPU_PWR_MGMT_1);
  TinyWireM.write(0x00);  // wake up
  TinyWireM.endTransmission();
}

// Fonction qui lit l'inclinaison du MPU6050 suivant l'axe Y
int16_t mpuLireAccelY() {
  TinyWireM.beginTransmission(MPU_ADDR);
  TinyWireM.write(MPU_ACCEL_YOUT);   // registre 0x3D
  TinyWireM.endTransmission();
  TinyWireM.requestFrom(MPU_ADDR, 2);
  uint8_t hi = TinyWireM.read();
  uint8_t lo = TinyWireM.read();
  return (int16_t)((hi << 8) | lo);
}


uint8_t touche_gauche() {
  int val = analogRead(BP_GAUCHE);
  if (val > 480 && val < 530) return 1;  // Jaune
  if (val > 530 && val < 630) return 2;  // Vert
  if (val > 630 && val < 700) return 3;  // Rouge
  if (val > 700 && val < 780) return 4;  // Bleu
  return 0;
}

uint8_t touche_droite() {
  int val = analogRead(BP_DROITE);
  if (val > 480 && val < 530) return 1;  // Haut
  if (val > 530 && val < 630) return 2;  // Droit
  if (val > 630 && val < 700) return 3;  // Bas
  if (val > 700 && val < 780) return 4;  // Gauche
  return 0;
}


void gererDeplacementManuel() {
  if (touche_droite() == 2) {
    positionFusil += 6;
    if (positionFusil > 120) positionFusil = 120;
  }
  if (touche_droite() == 4) {
    positionFusil -= 6;
    if (positionFusil < 30) positionFusil = 30;
  }
}

void gererDeplacementAuto() {
  int16_t ay = mpuLireAccelY();
  if (ay > MPU_SEUIL) {
    positionFusil += MPU_PAS;
    if (positionFusil > 120) positionFusil = 120;
  } else if (ay < -MPU_SEUIL) {
    positionFusil -= MPU_PAS;
    if (positionFusil < 30) positionFusil = 30;
  }
}

void genererFruit() {
  if (nbFruits >= 5) return;
  int idxType = random(0, nbTypesFruits);
  bool perime = (random(0, 100) < 15);
  fruits[nbFruits].type    = typesFruits[idxType];
  fruits[nbFruits].x       = random(28, 123);
  fruits[nbFruits].y       = 0;
  fruits[nbFruits].perime  = perime;
  fruits[nbFruits].visible = true;
  nbFruits++;
}

void effacerFruit(int i) {
  oled.setCursor(fruits[i].x, fruits[i].y / 8);
  oled.print("  ");
}

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

void deplacerFruits() {
  for (int i = 0; i < nbFruits; i++) {
    if (fruits[i].visible) {
      effacerFruit(i);
      fruits[i].y += vitesseFruits;
      if (fruits[i].y >= 64) {
        fruits[i].visible = false;
        for (int j = i; j < nbFruits - 1; j++) fruits[j] = fruits[j + 1];
        nbFruits--;
        i--;
      }
    }
  }
}


void effacerTir() {
  if (hauteurTir >= 0 && hauteurTir < 64) {
    oled.setCursor(positionTir, hauteurTir / 8);
    oled.print(" ");
  }
}

void tirer() {
  if (tirEnCours) return;
  tirEnCours  = true;
  positionTir = positionFusil;
  hauteurTir  = 56;
  tone(BUZZER, 1000, 20);
}

void gererTir() {
  if (!tirEnCours) return;

  effacerTir();

  hauteurTir -= 4;
  if (hauteurTir < 0) {
    tirEnCours = false;
    return;
  }

  for (int i = 0; i < nbFruits; i++) {
    if (fruits[i].visible &&
        abs(fruits[i].x - positionTir) <= 6 &&
        abs(fruits[i].y - hauteurTir)  <= 8) {

      effacerFruit(i);
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
      else                  score += points;

      tone(BUZZER, 500, 50);

      for (int j = i; j < nbFruits - 1; j++) fruits[j] = fruits[j + 1];
      nbFruits--;
      return;
    }
  }

  oled.setCursor(positionTir, hauteurTir / 8);
  oled.print("*");
}


void afficherFusil(int position) {
  oled.setCursor(anciennePositionFusil, 7);
  oled.print(" ");
  oled.setCursor(position, 7);
  oled.print("^");
  anciennePositionFusil = position;
}


void afficherAccueil() {
  oled.clear();
  oled.setCursor(4, 2);
  oled.print(F("JEU ATTiny85"));
  oled.setCursor(6, 4);
  oled.print(F("ECE Paris"));
  delay(2000);

  oled.clear();
  oled.setCursor(0, 1);
  oled.print(F("AIDE"));
  oled.setCursor(0, 3);
  oled.print(F("Dep: BP noir <- ->"));
  oled.setCursor(0, 5);
  oled.print(F("Tir: BP noir Haut"));
  delay(2500);

  etat = MENU_MODE;
}

void afficherMenuMode() {
  oled.clear();
  oled.setCursor(0, 0);
  oled.print(F("Mode deplacement:"));
  oled.setCursor(0, 2);
  oled.print(F("J: Manuel (BP)"));
  oled.setCursor(0, 4);
  oled.print(F("V: Gyroscope (MPU)"));

  uint8_t choix = touche_gauche();
  if (choix == 1) {
    modeAuto = false;
    etat = MENU_PRINCIPAL;
    oled.clear();
  }
  if (choix == 2) {
    modeAuto = true;
    mpuInit();
    etat = MENU_PRINCIPAL;
    oled.clear();
  }
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


//  BOUCLE DE JEU
void lancerJeu() {
  if (random(0, 100) < (5 + niveau * 3)) genererFruit();

  unsigned long maintenant = millis();
  if (maintenant - dernierDeplacementFruits >= INTERVALLE_FRUITS) {
    dernierDeplacementFruits = maintenant;
    deplacerFruits();
  }

  if (touche_droite() == 1 && !tirEnCours) {
    tirer();
    delay(150);
  }

  gererTir();

  if (modeAuto) gererDeplacementAuto();
  else          gererDeplacementManuel();

  afficherFruits();
  afficherFusil(positionFusil);

  oled.setCursor(0, 0);
  oled.print(F("S:"));
  oled.print(score);
  oled.setCursor(0, 1);
  oled.print(F("N:"));
  oled.print(niveau);

  for (int i = 0; i < 8; i++) {
    oled.setCursor(25, i);
    oled.print("|");
  }
}


void setup() {
  pinMode(BP_GAUCHE, INPUT);
  pinMode(BP_DROITE, INPUT);
  pinMode(BUZZER, OUTPUT);

  TinyWireM.begin();

  oled.begin(128, 64, sizeof(tiny4koled_init_128x64br), tiny4koled_init_128x64br);
  oled.setFont(FONT6X8);
  oled.on();
  oled.clear();

  uint32_t graine = 0;
  for (uint8_t i = 0; i < 16; i++) {
    graine ^= (uint32_t)analogRead(0) << (i % 16);
    graine += analogRead(1);
  }
  randomSeed(graine);
}

void loop() {
  if      (etat == MENU_ACCUEIL)   afficherAccueil();
  else if (etat == MENU_MODE)      afficherMenuMode();
  else if (etat == MENU_PRINCIPAL) afficherMenuPrincipal();
  else if (etat == JEU)            lancerJeu();
}
