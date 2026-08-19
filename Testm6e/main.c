/* ================================================================
   main.c
   Point d'entree — Scan RFID M6e + Selection tag + Mesure
   ================================================================ */
#include "m6e_connect.h"
#include "m6e_config.h"
#include "stm32.h"
#include "motor.h"
#include "m6e_scan.h"
#include "m6e_select.h"
#include "m6e_test_antenna.h"
#include "m6e_measure.h"
#include "m6e_sweep.h"
#include "m6e_heatmap.h"

   /*
    * MODE D'EXECUTION :
    * 0 = Test antennes      (identifier antennes defectueuses)
    * 1 = Mesure complete    (balayage freq x puissance 72 antennes)
    * 3 = Sweep freq x puissance sur une antenne choisie (type S21)
    * 4 = Heatmap            (rotation moteur x freq x antennes)
    */
#define RUN_MODE  1


int main(void)
{
    TMR_Reader reader;
    M6eInfo    m6e;
    STM32Info  stm32;
    uint32_t   freq_table[MAX_FREQ];
    int        freq_count = 0;
    int        connected = 0;
    TagProfile tags[MAX_TAGS]; /* Tableau des tags detectes */
    int        tag_count = 0;
    int        best_idx = -1;
    char       target_epc[64];
  

    memset(&reader, 0, sizeof(reader));
    memset(&m6e, 0, sizeof(m6e));
    memset(&stm32, 0, sizeof(stm32));
    memset(tags, 0, sizeof(tags));
    stm32.handle = INVALID_HANDLE_VALUE;

    printf("\n===================================\n");
    printf("   RFID M6e — MESURE ARCHE CAMA25  \n");

    switch (RUN_MODE)
    {
    case 0: printf("   MODE : TEST ANTENNES             \n"); break;
    case 1: printf("   MODE : MESURE COMPLETE           \n"); break;
    case 3: printf("   MODE : SWEEP FREQ x PUISSANCE    \n"); break;
    case 4: printf("   MODE : HEATMAP (ROTATION ARCHE)  \n"); break;
    default: printf("   MODE : INCONNU                  \n"); break;
    }
    printf("===================================\n");

    /* 1. Connexion M6e */
    if (!m6e_connect(&reader, &m6e)) goto end; /* en cas d'echec saut direct au nettoyage */
    connected = 1;

    /* 2. Configuration region + puissance + frequences */
    if (!m6e_config(&reader, freq_table, &freq_count)) goto end;
    
    
    /* 3. Detection STM32 — port reste ouvert — OBLIGATOIRE */
    if (!stm32_detect(m6e.com_port, &stm32))
    {
        printf("  [ERREUR] STM32 obligatoire — mesure impossible\n");
        printf("  -> Verifiez cable USB et firmware STM32\n");
        goto end;
    }

    /* 4. Scan tags — filtre phantom + selection par RSSI */
    if (!m6e_scan_tags(&reader, &stm32, tags, &tag_count)) goto end;
    if (!m6e_select_best_tag(tags, tag_count, &best_idx))  goto end;

    /* Copier l'EPC du tag selectionne */
    strncpy(target_epc, tags[best_idx].epc, sizeof(target_epc) - 1);
    target_epc[sizeof(target_epc) - 1] = '\0';

    printf("===================================\n");
    printf("  Tag retenu : %s\n", target_epc);
    printf("===================================\n\n");

    /* 5. Mode execution */
    switch (RUN_MODE)
    {
    case 0:
        /* Test toutes les antennes Theta puis Phi
         * Genere 2 fichiers CSV : test_Theta_*.csv et test_Phi_*.csv
         * Status : OK / FAIBLE / MORTE */
        m6e_test_antennas(&reader, &stm32, target_epc);
        break;

    case 1:
        /* Mesure balayage complet arche
         * Genere 1 fichier CSV : rfid_*.csv */
        m6e_measure_all(&reader, &stm32,
            freq_table, freq_count,
            target_epc);
        break;

    case 3:
        /* Sweep freq x puissance sur antenne choisie au clavier
         * Genere 1 fichier CSV : sweep_antNNN_*.csv */
        m6e_sweep(&reader, &stm32, target_epc);
        break;

    case 4:
        /* Mesure complete avec rotation moteur de l'arche
         * Genere 1 fichier CSV : heatmap_*.csv (colonne Angle_deg)
         * -> a tracer avec python/heatmap.py */
        m6e_measure_heatmap(&reader, &stm32,
            freq_table, freq_count,
            target_epc);
        break;
    }

end:
    stm32_close(&stm32);       /* Fermer le port serie du STM32 */
    if (connected)
        TMR_destroy(&reader);  /* Libere le lecteur M6e */

    printf("Appuyez sur Entree pour quitter...\n");
    getchar();
    return 0;
}