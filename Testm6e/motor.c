/* ================================================================
   motor.c
   Controle du moteur pas-a-pas de l'arche — meme handle COM que stm32.c
   (le port est deja ouvert par stm32_detect(), on ne le reouvre pas :
   pas de reset DTR, pas de conflit d'acces)

   Commandes envoyees telles que lues par le firmware Arduino
   control_arch_motor_v22_v01_2.ino :
     'M' <chiffres angle> 'D'  -> move_motor(angle) -> moveTo(angle*1400/360)
     'H'                       -> home()            -> moveTo(0)
   ================================================================ */
#include "motor.h"

static float g_current_angle_deg = 0.0f;

/* ================================================================
   motor_write_bytes — envoie caractere par caractere, comme stm32.c,
   pour laisser le temps au buffer serie du firmware
   ================================================================ */
static void motor_write_bytes(HANDLE h, const char* buf, int len)
{
    int   i;
    DWORD w;

    for (i = 0; i < len; i++)
    {
        WriteFile(h, &buf[i], 1, &w, NULL);
        Sleep(20);
    }
    FlushFileBuffers(h);
}

/* ================================================================
   motor_travel_time_ms
   Estime la duree (ms) d'un deplacement de |delta_deg| degres
   a partir du profil trapezoidal AccelStepper (accel/plateau/decel),
   avec une marge de securite pour le jitter serie/boucle Arduino.
   ================================================================ */
static DWORD motor_travel_time_ms(float delta_deg)
{
    double steps   = fabs((double)delta_deg) * MOTOR_STEPS_PER_REV / 360.0;
    double t_accel = MOTOR_MAX_SPEED_SPS / MOTOR_ACCEL_SPS2;
    double d_accel = 0.5 * MOTOR_ACCEL_SPS2 * t_accel * t_accel;
    double t_total;

    if (steps < 1.0) return 100; /* marge mini pour reception commande */

    if (2.0 * d_accel <= steps)
    {
        /* profil trapezoidal : montee, plateau a vitesse max, descente */
        double d_plateau = steps - 2.0 * d_accel;
        t_total = 2.0 * t_accel + d_plateau / MOTOR_MAX_SPEED_SPS;
    }
    else
    {
        /* deplacement trop court pour atteindre la vitesse max (triangle) */
        double t_peak = sqrt(steps / MOTOR_ACCEL_SPS2);
        t_total = 2.0 * t_peak;
    }

    /* +400 ms de marge (parsing serie, boucle Arduino, inertie) */
    return (DWORD)(t_total * 1000.0) + 400;
}

/* ================================================================
   motor_move_to — envoie "M<angle>D" et attend la fin du mouvement
   ================================================================ */
int motor_move_to(STM32Info* stm32, float angle_deg)
{
    char  cmd[32];
    int   len;
    DWORD wait_ms;

    if (!stm32 || stm32->handle == INVALID_HANDLE_VALUE) return 0;

    wait_ms = motor_travel_time_ms(angle_deg - g_current_angle_deg);
    len = sprintf(cmd, "M%dD", (int)angle_deg);

    printf("    Moteur -> %.1f deg (%s) ...", angle_deg, cmd);
    fflush(stdout);

    PurgeComm(stm32->handle, PURGE_RXABORT | PURGE_RXCLEAR |
        PURGE_TXABORT | PURGE_TXCLEAR);
    Sleep(30);

    motor_write_bytes(stm32->handle, cmd, len);
    Sleep(wait_ms);

    g_current_angle_deg = angle_deg;
    printf(" OK (%lu ms)\n", (unsigned long)wait_ms);
    return 1;
}

/* ================================================================
   motor_home — envoie "H" (retour position 0) et attend l'arrivee
   ================================================================ */
int motor_home(STM32Info* stm32)
{
    DWORD wait_ms;

    if (!stm32 || stm32->handle == INVALID_HANDLE_VALUE) return 0;

    wait_ms = motor_travel_time_ms(g_current_angle_deg);

    printf("    Moteur -> HOME (H) ...");
    fflush(stdout);

    PurgeComm(stm32->handle, PURGE_RXABORT | PURGE_RXCLEAR |
        PURGE_TXABORT | PURGE_TXCLEAR);
    Sleep(30);

    motor_write_bytes(stm32->handle, "H", 1);
    Sleep(wait_ms);

    g_current_angle_deg = 0.0f;
    printf(" OK (%lu ms)\n", (unsigned long)wait_ms);
    return 1;
}

float motor_get_angle(void)
{
    return g_current_angle_deg;
}
