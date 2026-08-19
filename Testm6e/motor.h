/* ================================================================
   motor.h
   Controle du moteur pas-a-pas de l'arche (rotation Theta)
   Meme carte / meme port serie que stm32.c (control_arch_motor_v22_v01_2.ino)
   Protocole moteur (firmware) :
     Aller a un angle absolu : M<angle_deg>D  (ex: "M90D")
     Retour position 0       : H
   Pas de message de fin de course renvoye par le firmware :
   la duree du mouvement est estimee a partir du profil de vitesse
   AccelStepper (setMaxSpeed / setAcceleration) puis attendue (Sleep).
   ================================================================ */
#ifndef MOTOR_H
#define MOTOR_H

#include "common.h"
#include "stm32.h"

/* Doit correspondre exactement au firmware control_arch_motor_v22_v01_2.ino */
#define MOTOR_STEPS_PER_REV   1400     /* stepper.moveTo(angle*1400/360)      */
#define MOTOR_MAX_SPEED_SPS   300.0f   /* stepper.setMaxSpeed(300)            */
#define MOTOR_ACCEL_SPS2      250.0f   /* stepper.setAcceleration(250)        */

/* Deplace le moteur vers un angle absolu (deg) et attend la fin du mouvement */
int   motor_move_to(STM32Info* stm32, float angle_deg);

/* Retour a la position de reference (0 deg) */
int   motor_home(STM32Info* stm32);

/* Derniere position connue (cote PC) apres motor_move_to/motor_home */
float motor_get_angle(void);

#endif /* MOTOR_H */
