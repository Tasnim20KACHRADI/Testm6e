# 🎯 Guide d'utilisation : motor.c / motor.h

## Vue d'ensemble

Le module `motor.c/h` pilote le **moteur pas-à-pas de l'arche CAMA25** via la même **connexion série STM32** que les commandes antennes.

### Architecture
```
PC (Testm6e.exe)
    ↓
Port série COM (115200 baud)
    ↓
STM32 Nucleo (firmware : control_arch_motor_v22_v01_2.ino)
    ├─ Antennes (switch_port via *N#)
    └─ Moteur (move_motor via M<angle>D, home via H)
```

---

## 📋 Fichiers

| Fichier | Rôle |
|---------|------|
| `motor.h` | Déclarations publiques |
| `motor.c` | Implémentation |

---

## 🔧 Configuration (motor.h)

Vérifiez que ces valeurs correspondent **exactement** à votre firmware Arduino :

```c
#define MOTOR_STEPS_PER_REV   1400      // stepper.moveTo(angle*1400/360)
#define MOTOR_MAX_SPEED_SPS   300.0f    // stepper.setMaxSpeed(300)
#define MOTOR_ACCEL_SPS2      250.0f    // stepper.setAcceleration(250)
```

**⚠️ Important :** Si ces valeurs sont fausses, les délais d'attente seront incorrects !

---

## 📝 API Publique

### 1. **motor_move_to()** — Déplacer à un angle absolu

```c
int motor_move_to(STM32Info* stm32, float angle_deg);
```

**Paramètres :**
- `stm32` : struct STM32Info (déjà ouvert par stm32_detect())
- `angle_deg` : angle cible en degrés (0.0 — 360.0)

**Retour :**
- `1` : succès
- `0` : erreur (handle invalide)

**Exemple :**
```c
motor_move_to(&stm32, 45.0);   // Tourne à 45°
motor_move_to(&stm32, 180.0);  // Tourne à 180°
```

**Ce qu'il fait :**
1. Envoie `M45D` ou `M180D` via série
2. Calcule la durée d'attente (profil trapézoïdal AccelStepper)
3. Attend avec Sleep()
4. Mémorise la position locale (`g_current_angle_deg`)

---

### 2. **motor_home()** — Retour à 0°

```c
int motor_home(STM32Info* stm32);
```

**Retour :**
- `1` : succès
- `0` : erreur

**Exemple :**
```c
motor_home(&stm32);  // Retour position 0°
```

**Ce qu'il fait :**
1. Envoie `H` via série
2. Attend le retour à 0°
3. Reset `g_current_angle_deg = 0.0f`

---

### 3. **motor_get_angle()** — Lire la position locale

```c
float motor_get_angle(void);
```

**Retour :** Angle actuel (deg), mémorisé côté PC

**Exemple :**
```c
float current = motor_get_angle();
printf("Position moteur : %.1f deg\n", current);
```

**⚠️ Note :** C'est la **position PC**, pas la position réelle du moteur !

---

## 🔌 Intégration avec stm32.c

Le port série est **partagé** :

```c
/* stm32_detect() l'ouvre UNE FOIS */
STM32Info stm32;
if (!stm32_detect(m6e.com_port, &stm32)) {
    printf("[ERREUR] STM32 non detectée\n");
    return 0;
}

/* Maintenant les deux modules peuvent l'utiliser */
stm32_send_cmd(&stm32, 15);      // Antenne 15
motor_move_to(&stm32, 90.0);     // Moteur 90°
stm32_send_cmd(&stm32, 20);      // Antenne 20
motor_home(&stm32);              // Retour HOME
```

**✅ Pas de conflit :** Chaque commande purge le buffer avant d'envoyer (voir `motor_write_bytes()`).

---

## ⏱️ Calcul de durée

La fonction `motor_travel_time_ms()` estime la durée du mouvement :

```
Entrée  : delta_deg (angle à parcourir)
Calcul  : profil trapézoïdal (accel → plateau → decel)
Sortie  : durée (ms) + 400ms de marge
```

**Exemple :**
- 90° → ~2700 ms (~2.7 sec)
- 10° → ~800 ms
- 1° → ~300 ms

---

## 🔄 Protocole série (firmware)

| Commande | Format | Exemple | Effet |
|----------|--------|---------|-------|
| Moteur | `M<angle>D` | `M90D` | Va à 90° |
| Home | `H` | `H` | Retour à 0° |
| Antenne | `*N#` | `*15#` | Active antenne 15 |

**⚠️ Important :** Le firmware **n'envoie PAS d'accusé de réception** !
- Seule l'exécution est garantie par délai d'attente
- D'où la marge de +400ms dans `motor_travel_time_ms()`

---

## 📊 Exemple complet (RUN_MODE 4 - Heatmap)

```c
int m6e_measure_heatmap(TMR_Reader* r, STM32Info* stm32, 
                        uint32_t* freq_table, int freq_count,
                        const char* target_epc)
{
    float angle;
    int ai, fi;
    
    /* Position de référence */
    motor_home(stm32);
    Sleep(300);
    
    /* Boucle angles */
    for (angle = 0.0f; angle <= 350.0f; angle += 10.0f) {
        
        /* Tourne à cet angle */
        if (!motor_move_to(stm32, angle)) {
            printf("Erreur moteur\n");
            continue;
        }
        Sleep(200);  /* stabilisation mécanique */
        
        /* Mesure toutes antennes à cet angle */
        for (fi = 0; fi < freq_count; fi++) {
            for (ai = 0; ai < ANT_THETA_COUNT; ai++) {
                m6e_measure_antenna(r, stm32,
                    ANTENNA_THETA[ai], AXE_THETA,
                    freq_table[fi], target_epc, angle);
            }
        }
    }
    
    /* Retour HOME */
    motor_home(stm32);
    return 1;
}
```

---

## 🐛 Dépannage

| Problème | Cause | Solution |
|----------|-------|----------|
| Moteur ne tourne pas | Handle invalide | Vérifier `stm32_detect()` avant `motor_move_to()` |
| Moteur tourne trop lent/rapide | MOTOR_MAX_SPEED_SPS faux | Ajuster la valeur dans motor.h |
| Mouvement trop court/long | MOTOR_STEPS_PER_REV faux | Vérifier firmware Arduino |
| CSV angle toujours 0.0 | angle_deg pas passé à m6e_measure_antenna() | Vérifier appels dans m6e_heatmap.c |

---

## ✅ Checklist avant test

- [ ] Firmware STM32 chargé : `control_arch_motor_v22_v01_2.ino`
- [ ] `MOTOR_STEPS_PER_REV` = **1400**
- [ ] `MOTOR_MAX_SPEED_SPS` = **300.0**
- [ ] `MOTOR_ACCEL_SPS2` = **250.0**
- [ ] `stm32_detect()` appelé avant `motor_move_to()`
- [ ] Moteur branché sur la STM32
- [ ] Port série COM disponible

---

## 📚 Références

- **Firmware Arduino** : `control_arch_motor_v22_v01_2.ino`
- **STM32 Interface** : `stm32.c/h`
- **Mesure** : `m6e_measure.c/h`
- **Heatmap** : `m6e_heatmap.c/h`
