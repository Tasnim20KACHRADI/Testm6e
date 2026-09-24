/* ================================================================
   m6e_measure.c
   Algorithme de mesure RFID :
   1. Balayage Theta puis Phi
   2. Pour chaque antenne : test zone morte
   3. Dichotomie grossiere pour trouver l'intervalle
   4. Raffinement dans l'intervalle pour P_min precise
   5. Ecriture CSV ligne par ligne

   API MercuryAPI :
   - TMR_PARAM_REGION_HOPTABLE — forcer une frequence
   - TMR_PARAM_REGION_HOPTIME  — hop time comme URA (3975ms)
   - TMR_PARAM_RADIO_READPOWER — regler la puissance
   - TMR_RP_init_simple
   - TMR_read / TMR_hasMoreTags / TMR_getNextTag
   - TMR_bytesToHex
   ================================================================ */
#include "m6e_measure.h"

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
   set_frequency
   Forcer une seule frequence avec hop time identique a URA
   Doc URA : Hop Time = 3975ms, Hop Table = freq unique
   Si echec -> continue sans forcer (firmware limitation)
   ================================================================ */
static int set_frequency(TMR_Reader* r, uint32_t freq_khz)
{
    TMR_uint32List hop;
    uint32_t       hop_one[1];
    uint32_t       hop_time = 3975; /* ms — identique URA */
    TMR_Status     ret;

    /* Configurer le hop time comme URA */
    TMR_paramSet(r, TMR_PARAM_REGION_HOPTIME, &hop_time);

    /* Forcer une seule frequence */
    hop_one[0] = freq_khz;
    hop.list = hop_one;
    hop.max = 1;
    hop.len = 1;

    ret = TMR_paramSet(r, TMR_PARAM_REGION_HOPTABLE, &hop);
    if (ret != TMR_SUCCESS)
        printf(" [freq non forcee] ");

    return 1; /* toujours continuer */
}

/* ================================================================
   set_power
   ================================================================ */
static int set_power(TMR_Reader* r, int32_t power_cdbm)
{
    TMR_Status ret = TMR_paramSet(r, TMR_PARAM_RADIO_READPOWER,
        &power_cdbm);
    return (ret == TMR_SUCCESS) ? 1 : 0;
}

/* ================================================================
   read_tag_once
   Lecture unique — verifie EPC cible
   Retourne 1 si detecte, 0 sinon
   ================================================================ */
static int read_tag_once(TMR_Reader* r,
    const char* target_epc,
    int16_t* rssi_out)
{
    TMR_Status      ret;
    TMR_ReadPlan    plan;
    TMR_TagReadData trd;
    char            epc_buf[64];
    uint8_t         m6e_port = 1;
    int             detected = 0;

    ret = TMR_RP_init_simple(&plan, 1, &m6e_port,
        TMR_TAG_PROTOCOL_GEN2, 1000);/*configurer plan de lecture*/
    if (ret != TMR_SUCCESS) return 0;

    ret = TMR_paramSet(r, TMR_PARAM_READ_PLAN, &plan);/*envoyer le plan au lecteur*/
    if (ret != TMR_SUCCESS) return 0;

    ret = TMR_read(r, MEASURE_TIMEOUT_MS, NULL);/*lire selon le plan*/
    if (ret != TMR_SUCCESS && ret != TMR_ERROR_NO_TAGS_FOUND)
        return 0;

	while (TMR_SUCCESS == TMR_hasMoreTags(r))/*parcourir les tags lus pour trouver le tag cible et son RSSI, si present*/
    {
        ret = TMR_getNextTag(r, &trd);
        if (ret != TMR_SUCCESS) break;

        if ((trd.tag.epcByteCount * 8) != EPC_EXPECTED_BITS)
            continue;

        TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, epc_buf);/*convertir les octets EPC en chaîne hexadécimale*/

		if (strcmp(epc_buf, target_epc) == 0)/*verifier si l'EPC correspond à la cible*/
        {
            if (rssi_out) *rssi_out = trd.rssi;
            detected = 1;
            break;
        }
    }

    return detected;
}

/* ================================================================
   m6e_test_dead_zone
   ================================================================ */
static int m6e_test_dead_zone(TMR_Reader* r, const char* epc)
{
    if (!set_power(r, POWER_START_CDBM)) return 0;
    Sleep(100);
    return read_tag_once(r, epc, NULL);
}

/* ================================================================
   m6e_find_min_power
   Phase 1 : Dichotomie grossiere (pas 5 dBm) -> trouve intervalle
   Phase 2 : Raffinement (pas 0.5 dBm) -> P_min precise
   ================================================================ */
static int32_t m6e_find_min_power(TMR_Reader* r,
    const char* epc,
    int16_t* rssi_out)
{
    int32_t p_high = POWER_START_CDBM;
    int32_t p_low = POWER_MIN_CDBM;
    int32_t p_mid;
    int32_t p_min = POWER_START_CDBM;
    int16_t rssi = 0;
    int     detected;

    /* ---- PHASE 1 : Dichotomie grossiere ---- */
    printf("    Phase 1 [%.1f .. %.1f dBm] pas 5 dBm\n",
        p_low / 100.0f, p_high / 100.0f);

    while ((p_high - p_low) > POWER_STEP_COARSE)
    {
        p_mid = ((p_high + p_low) / 2 / 50) * 50;

        set_power(r, p_mid);
        Sleep(100);
        detected = read_tag_once(r, epc, &rssi);

        printf("    %.1f dBm -> %s\n",
            p_mid / 100.0f,
            detected ? "detecte" : "non detecte");

        if (detected)
        {
            p_min = p_mid;
            p_high = p_mid;
        }
        else
        {
            p_low = p_mid;
        }
    }

    /* ---- PHASE 2 : Raffinement ---- */
    printf("    Phase 2 [%.1f .. %.1f dBm] pas 0.5 dBm\n",
        p_low / 100.0f, p_high / 100.0f);

    p_min = p_high;
    while (p_high >= p_low)
    {
        set_power(r, p_high);
        Sleep(100);
        detected = read_tag_once(r, epc, &rssi);

        printf("    %.1f dBm -> %s\n",
            p_high / 100.0f,
            detected ? "detecte" : "non detecte");

        if (detected)
        {
            p_min = p_high;
            if (rssi_out) *rssi_out = rssi;
            p_high -= POWER_STEP_FINE;
        }
        else
        {
            break;
        }
    }

    printf("    => P_min = %.1f dBm\n", p_min / 100.0f);
    return p_min;
} 


/* ================================================================
   m6e_measure_antenna
   Mesure complete pour une antenne et une frequence
   ================================================================ */
static void m6e_measure_antenna(TMR_Reader* r,
    STM32Info* stm32,
    int          antenna_num,
    AntennaAxe   axe,
    uint32_t     freq_khz,
    const char* target_epc)
{
    int16_t rssi = 0;
    int32_t p_min = -1;
    CsvRow  row;
    char    ts[32];

    get_timestamp(ts, sizeof(ts));

    printf("  Antenne %d [%s] @ %u kHz\n",
        antenna_num, axe_to_str(axe), freq_khz);

    /* Switch antenne via STM32 */
    if (stm32->handle != INVALID_HANDLE_VALUE)
    {
        printf("    Switch *%d# ...", antenna_num);
        fflush(stdout);
        if (!stm32_send_cmd(stm32, antenna_num))
        printf(" OK\n");
        Sleep(500);
    }

    /* Forcer la frequence */
    set_frequency(r, freq_khz);

    /* Test zone morte */
    printf("    Zone morte (%.1f dBm) ...", POWER_START_CDBM / 100.0f);
    fflush(stdout);

    if (!m6e_test_dead_zone(r, target_epc))
    {
        printf(" DEAD ZONE\n");
        memset(&row, 0, sizeof(row));
        row.freq_khz = freq_khz;
        row.antenna_num = antenna_num;
        row.p_min_dbm = -1.0f;
        row.rssi_dbm = -999.0f;
        strncpy(row.axe, axe_to_str(axe), sizeof(row.axe) - 1);
        strncpy(row.status, "DEAD_ZONE", sizeof(row.status) - 1);
        strncpy(row.epc, target_epc, sizeof(row.epc) - 1);
        strncpy(row.timestamp, ts, sizeof(row.timestamp) - 1);
        csv_write_row(&row);
        return;
    }
    printf(" OK\n");

    /* Mesure P_min */
    p_min = m6e_find_min_power(r, target_epc, &rssi);

    /* Ecrire CSV */
    get_timestamp(ts, sizeof(ts));
    memset(&row, 0, sizeof(row));
    row.freq_khz = freq_khz;
    row.antenna_num = antenna_num;
    row.p_min_dbm = p_min / 100.0f;
    row.rssi_dbm = (float)rssi;
    strncpy(row.axe, axe_to_str(axe), sizeof(row.axe) - 1);
    strncpy(row.status, "OK", sizeof(row.status) - 1);
    strncpy(row.epc, target_epc, sizeof(row.epc) - 1);
    strncpy(row.timestamp, ts, sizeof(row.timestamp) - 1);
    csv_write_row(&row);

    printf("    => RSSI = %d dBm | P_min = %.1f dBm\n\n",
        rssi, p_min / 100.0f);
}

/* ================================================================
   m6e_measure_all
   Boucle principale freq x Theta x Phi
   ================================================================ */
int m6e_measure_all(TMR_Reader* r,
    STM32Info* stm32,
    uint32_t* freq_table,
    int          freq_count,
    const char* target_epc)
{
    int  fi, ai;
    char csv_filename[256];

    print_header("MESURE — BALAYAGE COMPLET ARCHE");
    printf("  EPC        : %s\n", target_epc);
    printf("  Frequences : %d\n", freq_count);
    printf("  Antennes   : %d Theta + %d Phi = %d total\n",
        ANT_THETA_COUNT, ANT_PHI_COUNT, ANT_TOTAL);
    printf("  Algo       : Dichotomie 5dBm + raffinement 0.5dBm\n\n");

    /* Nom CSV avec timestamp */
    {
        time_t     now_t = time(NULL);
        struct tm* tm_now = localtime(&now_t);
        strftime(csv_filename, sizeof(csv_filename),
            CSV_PATH "rfid_%Y%m%d_%H%M%S.csv",
            tm_now);
    }

    printf("  Fichier CSV : %s\n\n", csv_filename);

    if (!csv_open(csv_filename)) return 0;

    /* Boucle frequences */
    for (fi = 0; fi < freq_count; fi++)
    {
        uint32_t freq = freq_table[fi];

        printf("\n===================================\n");
        printf("  FREQUENCE [%d/%d] : %u kHz\n",
            fi + 1, freq_count, freq);
        printf("===================================\n\n");

        /* Balayage Theta */
       printf("--- Balayage THETA (%d antennes) ---\n\n",
            ANT_THETA_COUNT);

        for (ai = 0; ai < ANT_THETA_COUNT; ai++)
        {
            printf("[%d/%d] ",
                ai + 1,
                ANT_THETA_COUNT + ANT_PHI_COUNT);
            fflush(stdout);
            m6e_measure_antenna(r, stm32,
                ANTENNA_THETA[ai], AXE_THETA,
                freq, target_epc);
        }

        /* Balayage Phi */
       printf("--- Balayage PHI (%d antennes) ---\n\n",
            ANT_PHI_COUNT);

        for (ai = 0; ai < ANT_PHI_COUNT; ai++)
        {
            printf("[%d/%d] ",
                ANT_THETA_COUNT + ai + 1,
                ANT_THETA_COUNT + ANT_PHI_COUNT);
            fflush(stdout);
            m6e_measure_antenna(r, stm32,
                ANTENNA_PHI[ai], AXE_PHI,
                freq, target_epc);
        }
        
        printf("\nFrequence %u kHz terminee.\n", freq);
    }

    csv_close();

    printf("\n===================================\n");
    printf("  MESURE TERMINEE\n");
    printf("  Fichier : %s\n", csv_filename);
    printf("===================================\n\n");

    return 1;
}