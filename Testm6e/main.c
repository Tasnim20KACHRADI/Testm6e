/* ================================================================
   main_test_power.c
   Test puissance delivree — mesure avec analyseur de spectre
   868 MHz — emission continue + verification puissance appliquee
   ================================================================ */
#include "m6e_connect.h"

#define TEST_POWER_CDBM   3000    /* 24.0 dBm — changer ici */

int main(void)
{
    TMR_Reader   reader;
    M6eInfo      m6e;
    TMR_Status   ret;
    TMR_ReadPlan plan;
    TMR_Region   region = TMR_REGION_OPEN;
    int32_t      power = TEST_POWER_CDBM;
    int32_t      power_actual = 0;
    uint8_t      port = 1;
    int          connected = 0;

    memset(&reader, 0, sizeof(reader));
    memset(&m6e, 0, sizeof(m6e));

    printf("\n===================================\n");
    printf("   TEST PUISSANCE M6e\n");
    printf("   Puissance demandee : %.1f dBm\n", TEST_POWER_CDBM / 100.0f);
    printf("   Frequence          : 868 MHz\n");
    printf("   Ctrl+C pour arreter\n");
    printf("===================================\n\n");

    /* 1. Connexion M6e */
    if (!m6e_connect(&reader, &m6e)) goto end;
    connected = 1;
    printf("M6e connecte : COM%d @ %u baud\n\n",
        m6e.com_port, m6e.baudrate);

    /* 2. Region OPEN */
    ret = TMR_paramSet(&reader, TMR_PARAM_REGION_ID, &region);
    if (ret != TMR_SUCCESS)
    {
        printf("[ERREUR] Region : %s\n", TMR_strerr(&reader, ret));
        goto end;
    }
    printf("Region : OPEN\n");

    /* 3. Puissance fixe */
    ret = TMR_paramSet(&reader, TMR_PARAM_RADIO_READPOWER, &power);
    if (ret != TMR_SUCCESS)
    {
        printf("[ERREUR] Puissance : %s\n", TMR_strerr(&reader, ret));
        goto end;
    }

    /* 4. Verification puissance reellement appliquee */
    TMR_paramGet(&reader, TMR_PARAM_RADIO_READPOWER, &power_actual);
    printf("Puissance demandee : %.1f dBm\n", power / 100.0f);
    printf("Puissance appliquee: %.1f dBm\n", power_actual / 100.0f);

    if (power_actual != power)
        printf("[!] ATTENTION — puissance differente de la demande !\n");
    else
        printf("Puissance OK\n");

    /* 5. Plan lecture port 1 — GEN2 */
    TMR_RP_init_simple(&plan, 1, &port, TMR_TAG_PROTOCOL_GEN2, 1000);
    TMR_paramSet(&reader, TMR_PARAM_READ_PLAN, &plan);

    /* 6. Emission continue */
    printf("\n===================================\n");
    printf("  EMISSION EN COURS\n");
    printf("  → Analyseur en mode Max Hold\n");
    printf("  → RBW = 1 MHz, VBW = 1 MHz\n");
    printf("  → Center 868 MHz, Span 10 MHz\n");
    printf("  → Attendu sur analyseur : %.1f dBm\n",
        (power_actual / 100.0f) - 30.0f);
    printf("===================================\n\n");

    while (1)
    {
        TMR_read(&reader, 1000, NULL);
    }

end:
    if (connected) TMR_destroy(&reader);
    printf("Appuyez sur Entree pour quitter...\n");
    getchar();
    return 0;
}