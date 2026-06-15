/* ================================================================
   m6e_test_antenna.c
   Test de toutes les antennes de l'arche CAMA25

   Objectif : identifier les antennes defectueuses
   Methode  : pour chaque antenne, lancer TEST_READS_PER_ANT lectures
              a puissance maximale et compter les detections

   Deux fichiers CSV separes :
   - test_theta_YYYYMMDD_HHMMSS.csv  (antennes Theta)
   - test_phi_YYYYMMDD_HHMMSS.csv    (antennes Phi)

   Colonnes CSV :
   Antenne, Axe, Lectures_OK, Lectures_Total, RSSI_moy,
   RSSI_min, RSSI_max, Status, Timestamp
   ================================================================ */
#include "m6e_test_antenna.h"

   /* ================================================================
      get_timestamp
      ================================================================ */
static void get_timestamp(char* buf, int size)
{
    time_t     now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", t);
}

/* ================================================================
   Structure résultat test antenne
   ================================================================ */
typedef struct {
    int     antenna_num;
    char    axe[8];
    int     reads_ok;
    int     reads_total;
    float   rssi_mean;
    int16_t rssi_min;
    int16_t rssi_max;
    char    status[16];   /* "OK", "FAIBLE", "MORTE" */
    char    timestamp[32];
} AntennaTestResult;

/* ================================================================
   open_test_csv — ouvre un CSV de test avec son en-tête
   ================================================================ */
static FILE* open_test_csv(const char* axe_name)
{
    char      fname[256];
    char      date_str[32];
    FILE* fp;
    time_t    now_t = time(NULL);
    struct tm* t = localtime(&now_t);

    /* Generer la date séparément */
    strftime(date_str, sizeof(date_str), "%Y%m%d_%H%M%S", t);

    /* Construire le nom du fichier */
    sprintf(fname, "%stest_%s_%s.csv", CSV_PATH, axe_name, date_str);

    fp = fopen(fname, "w");
    if (!fp)
    {
        printf("[ERREUR] Impossible de creer %s\n", fname);
        return NULL;
    }

    /* En-tete CSV */
    fprintf(fp, "Antenne,Axe,Lectures_OK,Lectures_Total,"
        "RSSI_moy,RSSI_min,RSSI_max,Status,Timestamp\n");

    printf("  CSV cree : %s\n", fname);
    return fp;
}

/* ================================================================
   write_test_row — ecrit une ligne de resultat
   ================================================================ */
static void write_test_row(FILE* fp, const AntennaTestResult* r)
{
    if (!fp) return;
    fprintf(fp, "%d,%s,%d,%d,%.1f,%d,%d,%s,%s\n",
        r->antenna_num,
        r->axe,
        r->reads_ok,
        r->reads_total,
        r->rssi_mean,
        r->rssi_min,
        r->rssi_max,
        r->status,
        r->timestamp);
    fflush(fp);
}

/* ================================================================
   test_one_antenna — teste une antenne N fois
   ================================================================ */
static void test_one_antenna(TMR_Reader* r,
    STM32Info* stm32,
    int                antenna_num,
    AntennaAxe         axe,
    const char* target_epc,
    AntennaTestResult* result)
{
    TMR_Status      ret;
    TMR_ReadPlan    plan;
    TMR_TagReadData trd;
    char            epc_buf[64];
    uint8_t         m6e_port = 1;
    int32_t         power = TEST_POWER_CDBM;
    int             pass;
    int16_t         rssi_samples[TEST_READS_PER_ANT];
    int             reads_ok = 0;
    int16_t         rssi_sum = 0;
    int16_t         rssi_min = 30000;
    int16_t         rssi_max = -30000;
    char            ts[32];

    get_timestamp(ts, sizeof(ts));

    /* Init résultat */
    memset(result, 0, sizeof(AntennaTestResult));
    result->antenna_num = antenna_num;
    result->reads_total = TEST_READS_PER_ANT;
    strncpy(result->axe, axe_to_str(axe), sizeof(result->axe) - 1);
    strncpy(result->timestamp, ts, sizeof(result->timestamp) - 1);

    /* Switch antenne */
    if (stm32->handle != INVALID_HANDLE_VALUE)
    {
        if (!stm32_send_cmd(stm32, antenna_num))
            printf(" [!]");
        Sleep(1500);
    }

    /* Configurer puissance max */
    TMR_paramSet(r, TMR_PARAM_RADIO_READPOWER, &power);

    /* Plan de lecture */
    ret = TMR_RP_init_simple(&plan, 1, &m6e_port,
        TMR_TAG_PROTOCOL_GEN2, 1000);
    if (ret != TMR_SUCCESS)
    {
        strcpy(result->status, "ERREUR");
        return;
    }

    ret = TMR_paramSet(r, TMR_PARAM_READ_PLAN, &plan);
    if (ret != TMR_SUCCESS)
    {
        strcpy(result->status, "ERREUR");
        return;
    }

    /* Boucle de lectures */
    for (pass = 0; pass < TEST_READS_PER_ANT; pass++)
    {
        ret = TMR_read(r, TEST_TIMEOUT_MS, NULL);
        if (ret != TMR_SUCCESS && ret != TMR_ERROR_NO_TAGS_FOUND)
            continue;

        while (TMR_SUCCESS == TMR_hasMoreTags(r))
        {
            ret = TMR_getNextTag(r, &trd);
            if (ret != TMR_SUCCESS) break;

            /* Filtre longueur EPC */
            if ((trd.tag.epcByteCount * 8) != EPC_EXPECTED_BITS)
                continue;

            TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, epc_buf);

            /* Verifier EPC cible si fourni */
            if (target_epc != NULL && strlen(target_epc) > 0)
                if (strcmp(epc_buf, target_epc) != 0) continue;

            rssi_samples[reads_ok] = trd.rssi;
            rssi_sum += trd.rssi;
            if (trd.rssi < rssi_min) rssi_min = trd.rssi;
            if (trd.rssi > rssi_max) rssi_max = trd.rssi;
            reads_ok++;
            break;
        }
    }

    /* Remplir résultat */
    result->reads_ok = reads_ok;

    if (reads_ok > 0)
    {
        result->rssi_mean = (float)rssi_sum / reads_ok;
        result->rssi_min = rssi_min;
        result->rssi_max = rssi_max;
    }
    else
    {
        result->rssi_mean = -999.0f;
        result->rssi_min = -999;
        result->rssi_max = -999;
    }

    /* Status */
    if (reads_ok == 0)
        strcpy(result->status, "MORTE");
    else if (reads_ok < TEST_READS_PER_ANT / 2)
        strcpy(result->status, "FAIBLE");
    else
        strcpy(result->status, "OK");
}

/* ================================================================
   test_axe — teste toutes les antennes d'un axe
   ================================================================ */
static void test_axe(TMR_Reader* r,
    STM32Info* stm32,
    const char* target_epc,
    const int* ant_list,
    int            ant_count,
    AntennaAxe     axe,
    const char* axe_name)
{
    FILE* fp;
    AntennaTestResult result;
    int               i;
    int               count_ok = 0;
    int               count_faible = 0;
    int               count_morte = 0;

    printf("\n===================================\n");
    printf("  TEST AXE %s (%d antennes)\n", axe_name, ant_count);
    printf("===================================\n\n");

    fp = open_test_csv(axe_name);
    if (!fp) return;

    for (i = 0; i < ant_count; i++)
    {
        int ant = ant_list[i];
        printf("  [%02d/%02d] Antenne %2d ... ",
            i + 1, ant_count, ant);
        fflush(stdout);

        test_one_antenna(r, stm32, ant, axe, target_epc, &result);

        /* Affichage résultat */
        if (strcmp(result.status, "OK") == 0)
        {
            printf("OK   | %d/%d lect | RSSI moy=%.1f dBm"
                " [%d .. %d]\n",
                result.reads_ok, result.reads_total,
                result.rssi_mean, result.rssi_min, result.rssi_max);
            count_ok++;
        }
        else if (strcmp(result.status, "FAIBLE") == 0)
        {
            printf("FAIBLE | %d/%d lect | RSSI moy=%.1f dBm\n",
                result.reads_ok, result.reads_total,
                result.rssi_mean);
            count_faible++;
        }
        else
        {
            printf("MORTE  | 0/%d lect\n", result.reads_total);
            count_morte++;
        }

        write_test_row(fp, &result);
    }

    fclose(fp);

    /* Résumé */
    printf("\n-----------------------------------\n");
    printf("  RESUME %s\n", axe_name);
    printf("-----------------------------------\n");
    printf("  OK     : %d/%d\n", count_ok, ant_count);
    printf("  FAIBLE : %d/%d\n", count_faible, ant_count);
    printf("  MORTE  : %d/%d\n", count_morte, ant_count);

    if (count_morte > 0 || count_faible > 0)
    {
        printf("\n  Antennes a verifier :\n");
        /* Re-parcourir pour afficher les defectueuses */
        for (i = 0; i < ant_count; i++)
        {
            /* On relit depuis le CSV — ici on reaffiche juste */
        }
    }
    printf("-----------------------------------\n");
}

/* ================================================================
   m6e_test_antennas — fonction principale
   ================================================================ */
int m6e_test_antennas(TMR_Reader* r,
    STM32Info* stm32,
    const char* target_epc)
{
    int32_t power = TEST_POWER_CDBM;

    print_header("TEST ANTENNES ARCHE CAMA25");
    printf("  EPC cible  : %s\n",
        (target_epc && strlen(target_epc) > 0)
        ? target_epc : "(tous tags)");
    printf("  Puissance  : %.1f dBm\n", power / 100.0f);
    printf("  Lectures   : %d par antenne\n\n", TEST_READS_PER_ANT);

    /* Configurer puissance */
    TMR_paramSet(r, TMR_PARAM_RADIO_READPOWER, &power);

    /* Test axe Theta */
    test_axe(r, stm32, target_epc,
        ANTENNA_THETA, ANT_THETA_COUNT,
        AXE_THETA, "Theta");

    /* Test axe Phi */
    test_axe(r, stm32, target_epc,
        ANTENNA_PHI, ANT_PHI_COUNT,
        AXE_PHI, "Phi");

    printf("\n===================================\n");
    printf("  TEST TERMINE\n");
    printf("  Fichiers CSV dans : %s\n", CSV_PATH);
    printf("===================================\n\n");

    return 1;
}