/**
 * Moteur 17HS4401 + Driver A4988 + Arduino
 *
 * Pinout:
 *   Arduino Pin 8  → A4988 DIR (Direction)
 *   Arduino Pin 9  → A4988 STEP (Pulse)
 *   Arduino GND    → A4988 GND
 *
 * Moteur (4 fils utilisés):
 *   Noir (A+)   → A4988 OUT1
 *   Vert (A-)   → A4988 OUT2
 *   Rouge (B+)  → A4988 OUT3
 *   Bleu (B-)   → A4988 OUT4
 *
 * Alimentation:
 *   +5V    → Arduino + A4988 Logic
 *   +12V   → A4988 VMOT (moteur) - MIN 2A
 *   GND    → Tous les GND
 */

// ===== Configuration des pins =====
#define DIR_PIN 8              // Pin de direction
#define STEP_PIN 9             // Pin de pulse
#define STEPS_PER_REVOLUTION 200  // 17HS4401 = 200 pas pour 1 tour

// ===== Variables globales =====
int currentRPM = 60;           // Vitesse actuelle
bool currentDirection = true;  // true = horaire, false = antihoraire
int motorSpeed = 300000;       // Délai entre pas (micros)

// ===== Configuration =====
void setup() {
  Serial.begin(9600);

  // Configurer les pins
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);

  // État initial sûr
  digitalWrite(DIR_PIN, HIGH);
  digitalWrite(STEP_PIN, LOW);

  // Afficher le menu
  printMenu();
}

// ===== Boucle principale =====
void loop() {
  // Écouter les commandes du port série
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.length() > 0) {
      processCommand(command);
    }
  }
}

// ===== Traitement des commandes =====
void processCommand(String cmd) {
  char action = cmd[0];
  String args = cmd.substring(1);
  args.trim();

  switch (action) {
    case '1': // Test rotation simple
      Serial.println("▶ TEST: Rotation 5 tours horaire...");
      rotateMotor(5, true);
      Serial.println("✓ Terminé\n");
      break;

    case '2': // Rotation antihoraire
      Serial.println("▶ TEST: Rotation 5 tours antihoraire...");
      rotateMotor(5, false);
      Serial.println("✓ Terminé\n");
      break;

    case '3': // Accélération progressive
      Serial.println("▶ TEST: Accélération progressive...");
      testAcceleration();
      Serial.println("✓ Terminé\n");
      break;

    case 'r': // Rotation avec nombre de tours
      if (args.length() > 0) {
        int tours = args.toInt();
        Serial.print("▶ Rotation ");
        Serial.print(tours);
        Serial.println(" tours (CW)...");
        rotateMotor(tours, true);
        Serial.println("✓ Terminé\n");
      }
      break;

    case 'l': // Rotation antihoraire avec nombre
      if (args.length() > 0) {
        int tours = args.toInt();
        Serial.print("▶ Rotation ");
        Serial.print(tours);
        Serial.println(" tours (CCW)...");
        rotateMotor(tours, false);
        Serial.println("✓ Terminé\n");
      }
      break;

    case 's': // Changer la vitesse (RPM)
      if (args.length() > 0) {
        int rpm = args.toInt();
        if (rpm >= 1 && rpm <= 1000) {
          setRPM(rpm);
          Serial.print("✓ Vitesse mise à jour: ");
          Serial.print(rpm);
          Serial.println(" RPM\n");
        } else {
          Serial.println("✗ RPM invalide (1-1000)\n");
        }
      }
      break;

    case 'p': // Rotation nombre exact de pas
      if (args.length() > 0) {
        int steps = args.toInt();
        Serial.print("▶ Rotation ");
        Serial.print(steps);
        Serial.println(" pas...");
        rotateSteps(steps, true);
        Serial.println("✓ Terminé\n");
      }
      break;

    case 'i': // Infos actuelles
      printInfo();
      break;

    case 'h': // Aide
      printMenu();
      break;

    default:
      Serial.println("✗ Commande non reconnue. Tapez 'h' pour l'aide.\n");
  }
}

// ===== Fonctions de mouvement =====

/**
 * Tourner le moteur X tours
 */
void rotateMotor(int tours, bool clockwise) {
  int totalSteps = tours * STEPS_PER_REVOLUTION;
  rotateSteps(totalSteps, clockwise);
}

/**
 * Tourner le moteur X pas
 */
void rotateSteps(int steps, bool clockwise) {
  // Définir la direction
  digitalWrite(DIR_PIN, clockwise ? HIGH : LOW);
  delayMicroseconds(50);  // Attendre que le driver traite

  // Générer les pulses
  for (int i = 0; i < steps; i++) {
    // HIGH pendant ~5μs, puis LOW
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(STEP_PIN, LOW);

    // Délai avant le prochain pas
    delayMicroseconds(motorSpeed);

    // Afficher la progression chaque 50 pas
    if ((i + 1) % 50 == 0) {
      Serial.print(".");
    }
  }
  Serial.println();
}

/**
 * Changer la vitesse (RPM)
 * RPM = (60,000,000) / (STEPS_PER_REVOLUTION * delayMicroseconds)
 */
void setRPM(int rpm) {
  currentRPM = rpm;
  // Calculer le délai correspondant
  motorSpeed = (60000000) / (STEPS_PER_REVOLUTION * rpm);

  // Clamper les valeurs pour sécurité
  if (motorSpeed < 100) motorSpeed = 100;      // Max ~6000 RPM
  if (motorSpeed > 5000000) motorSpeed = 5000000; // Min ~0.2 RPM
}

/**
 * Test d'accélération progressive
 */
void testAcceleration() {
  Serial.println("Phase 1: Accélération...");

  // Accélérer de 30 à 300 RPM en 10 étapes
  for (int rpm = 30; rpm <= 300; rpm += 30) {
    Serial.print("  RPM: ");
    Serial.print(rpm);
    Serial.print(" - ");

    setRPM(rpm);
    rotateMotor(1, true);  // 1 tour à chaque vitesse

    delay(200);  // Pause entre les accélérations
  }

  Serial.println("\nPhase 2: Décélération...");

  // Décélérer de 300 à 30 RPM
  for (int rpm = 300; rpm >= 30; rpm -= 30) {
    Serial.print("  RPM: ");
    Serial.print(rpm);
    Serial.print(" - ");

    setRPM(rpm);
    rotateMotor(1, true);

    delay(200);
  }

  // Revenir à la vitesse par défaut
  setRPM(60);
}

// ===== Affichage =====

void printMenu() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║  17HS4401 + A4988 + Arduino  v1.0     ║");
  Serial.println("╚════════════════════════════════════════╝\n");

  Serial.println("COMMANDES RAPIDES:");
  Serial.println("  1     - Test rotation 5 tours (CW)");
  Serial.println("  2     - Test rotation 5 tours (CCW)");
  Serial.println("  3     - Test accélération progressive\n");

  Serial.println("COMMANDES PERSONNALISÉES:");
  Serial.println("  r[n]  - Rotation n tours (CW)  ex: r10");
  Serial.println("  l[n]  - Rotation n tours (CCW) ex: l10");
  Serial.println("  p[n]  - Rotation n pas         ex: p400");
  Serial.println("  s[n]  - Vitesse n RPM          ex: s100\n");

  Serial.println("UTILITAIRES:");
  Serial.println("  i     - Afficher les infos actuelles");
  Serial.println("  h     - Afficher ce menu\n");

  printInfo();
}

void printInfo() {
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║  CONFIGURATION ACTUELLE                ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.print("║ Vitesse (RPM):        ");
  Serial.print(currentRPM);
  Serial.println(" ");

  Serial.print("║ Délai (μs):           ");
  Serial.print(motorSpeed);
  Serial.println(" ");

  Serial.println("║ Moteur:               17HS4401 (NEMA17)");
  Serial.println("║ Driver:               A4988");
  Serial.println("║ Pas/tour:             200");
  Serial.println("║ Angle/pas:            1.8°");
  Serial.println("║ Alimentation:         12V (2A min)");
  Serial.println("╚════════════════════════════════════════╝\n");
}

// ===== Fonction debug =====
void debugPulse() {
  Serial.println("▶ DEBUG: Envoi d'une seule pulse...");
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(STEP_PIN, LOW);
  Serial.println("✓ Pulse envoyée (vérifier avec oscilloscope)\n");
}
