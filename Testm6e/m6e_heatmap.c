/* ================================================================
   m6e_heatmap.c
   Balayage angle (moteur) x frequence x antenne Theta
   Genere 1 fichier CSV : heatmap_*.csv (colonne Angle_deg en plus)
   ================================================================ */
#include "m6e_heatmap.h"

int m6e_measure_heatmap(TMR_Reader* r,
    STM32Info* stm32,
    uint32_t* freq_table,
    int          freq_count,
    const char* target_epc)
{
    float angle;
    int   fi, ai, ant_idx, step_idx, step_total;
    int   ant_num;
    AntennaAxe axe;
    char  csv_filename[256];

    if (!stm32 || stm32->handle == INVALID_HANDLE_VALUE)
    {
        printf("  [ERREUR] Moteur/STM32 non connecte — heatmap impossible\n");
        return 0;
    }

    step_total = (int)((HEATMAP_ANGLE_STOP_DEG - HEATMAP_ANGLE_START_DEG)
        / HEATMAP_ANGLE_STEP_DEG) + 1;

    print_header("MESURE HEATMAP - ROTATION x FREQUENCE x ANTENNE");
    printf("  EPC        : %s\n", target_epc);
    printf("  Angles     : %.0f a %.0f deg (pas %.0f deg, %d positions)\n",
        HEATMAP_ANGLE_START_DEG, HEATMAP_ANGLE_STOP_DEG,
        HEATMAP_ANGLE_STEP_DEG, step_total);
    printf("  Frequences : %d\n", freq_count);
    printf("  Antennes   : %d Theta + %d Phi = %d total\n\n",
        ANT_THETA_COUNT, ANT_PHI_COUNT, ANT_TOTAL);

    /* Nom CSV avec timestamp */
    {
        time_t     now_t = time(NULL);
        struct tm* tm_now = localtime(&now_t);
        strftime(csv_filename, sizeof(csv_filename),
            CSV_PATH "heatmap_%Y%m%d_%H%M%S.csv",
            tm_now);
    }
    printf("  Fichier CSV : %s\n\n", csv_filename);

    if (!csv_open(csv_filename)) return 0;

    /* Position de reference avant de commencer le balayage */
    motor_home(stm32);
    Sleep(300);

    /* ===== BOUCLE PRINCIPALE : Angle → Frequence → Antenne ===== */
    step_idx = 0;
    for (angle = HEATMAP_ANGLE_START_DEG;
        angle <= HEATMAP_ANGLE_STOP_DEG + 0.01f;
        angle += HEATMAP_ANGLE_STEP_DEG)
    {
        step_idx++;
        printf("\n===================================\n");
        printf("  ANGLE [%d/%d] : %.0f deg\n", step_idx, step_total, angle);
        printf("===================================\n");

        /* Moteur tourne a cet angle */
        if (!motor_move_to(stm32, angle))
        {
            printf("  [!] Deplacement moteur echoue — angle ignore\n");
            continue;
        }
        Sleep(300); /* stabilisation mecanique */

        /* Boucle frequences */
        for (fi = 0; fi < freq_count; fi++)
        {
            uint32_t freq = freq_table[fi];
            printf("  --- Freq %u kHz ---\n", freq);

            /* ===== MESURE THETA ===== */
            printf("  [Theta : %d antennes]\n", ANT_THETA_COUNT);
            for (ai = 0; ai < ANT_THETA_COUNT; ai++)
            {
                ant_num = ANTENNA_THETA[ai];
                printf("    [%d/%d] ant %d ", ai + 1, ANT_THETA_COUNT, ant_num);
                fflush(stdout);
                m6e_measure_antenna(r, stm32,
                    ant_num, AXE_THETA,
                    freq, target_epc, angle);
            }

            /* ===== MESURE PHI ===== */
            printf("  [Phi : %d antennes]\n", ANT_PHI_COUNT);
            for (ai = 0; ai < ANT_PHI_COUNT; ai++)
            {
                ant_num = ANTENNA_PHI[ai];
                printf("    [%d/%d] ant %d ", ai + 1, ANT_PHI_COUNT, ant_num);
                fflush(stdout);
                m6e_measure_antenna(r, stm32,
                    ant_num, AXE_PHI,
                    freq, target_epc, angle);
            }
        }
    }

    /* Retour position de reference en fin de mesure */
    motor_home(stm32);

    csv_close();

    printf("\n===================================\n");
    printf("  HEATMAP TERMINEE\n");
    printf("  Fichier : %s\n", csv_filename);
    printf("  -> python python\\heatmap.py \"%s\"\n", csv_filename);
    printf("===================================\n\n");

    return 1;
}
