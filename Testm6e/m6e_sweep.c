/* ================================================================
   m6e_sweep.c
   Sweep frequence x puissance sur une antenne choisie
   Algorithme dichotomie + raffinement pour P_min par frequence

   Algorithme :
   1. Demander le numero d'antenne au clavier
   2. Switch STM32 vers cette antenne
   3. Pour chaque frequence (865 -> 869 MHz, pas 500 kHz) :
      Phase 1 : Dichotomie grossiere (pas 5 dBm) -> intervalle
      Phase 2 : Raffinement (pas 0.5 dBm) -> P_min precise
   4. CSV : Antenne, Freq_kHz, P_min_dBm, RSSI_dBm, Timestamp
   ================================================================ */
#include "m6e_sweep.h"

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
   set_freq — forcer une frequence
   ================================================================ */
static void set_freq(TMR_Reader* r, uint32_t freq_khz)
{
    TMR_uint32List hop;
    uint32_t       hop_one[1];
    uint32_t       hop_time = 3975;

    TMR_paramSet(r, TMR_PARAM_REGION_HOPTIME, &hop_time);
    hop_one[0] = freq_khz;
    hop.list = hop_one;
    hop.max = 1;
    hop.len = 1;
    TMR_paramSet(r, TMR_PARAM_REGION_HOPTABLE, &hop);
}

/* ================================================================
   set_pwr — regler la puissance en centidBm
   ================================================================ */
static void set_pwr(TMR_Reader* r, int32_t power_cdbm)
{
    TMR_paramSet(r, TMR_PARAM_RADIO_READPOWER, &power_cdbm);
}

/* ================================================================
   read_tag_once — une lecture, verifie EPC cible
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
        TMR_TAG_PROTOCOL_GEN2, 1000);
    if (ret != TMR_SUCCESS) return 0;

    ret = TMR_paramSet(r, TMR_PARAM_READ_PLAN, &plan);
    if (ret != TMR_SUCCESS) return 0;

    ret = TMR_read(r, SWEEP_TIMEOUT_MS, NULL);
    if (ret != TMR_SUCCESS && ret != TMR_ERROR_NO_TAGS_FOUND)
        return 0;

    while (TMR_SUCCESS == TMR_hasMoreTags(r))
    {
        ret = TMR_getNextTag(r, &trd);
        if (ret != TMR_SUCCESS) break;

        if ((trd.tag.epcByteCount * 8) != EPC_EXPECTED_BITS)
            continue;

        TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, epc_buf);

        if (strcmp(epc_buf, target_epc) == 0)
        {
            if (rssi_out) *rssi_out = trd.rssi;
            detected = 1;
            break;
        }
    }

    return detected;
}

/* ================================================================
   find_pmin — dichotomie + raffinement pour une frequence donnee
   Retourne P_min en centidBm, -1 si zone morte
   ================================================================ */
static int32_t find_pmin(TMR_Reader* r,
    const char* target_epc,
    uint32_t     freq_khz,
    int16_t* rssi_out)
{
    int32_t p_high = SWEEP_POWER_START;
    int32_t p_low = SWEEP_POWER_STOP;
    int32_t p_mid;
    int32_t p_min = SWEEP_POWER_START;
    int16_t rssi = -999;
    int     detected;

    /* Test zone morte a puissance max */
    set_pwr(r, SWEEP_POWER_START);
    Sleep(100);
    if (!read_tag_once(r, target_epc, NULL))
    {
        printf(" DEAD ZONE\n");
        return -1;
    }

    /* ---- Phase 1 : Dichotomie grossiere (pas 5 dBm) ---- */
    while ((p_high - p_low) > SWEEP_POWER_STEP_COARSE)
    {
        p_mid = ((p_high + p_low) / 2 / 50) * 50;
        set_pwr(r, p_mid);
        Sleep(100);
        detected = read_tag_once(r, target_epc, &rssi);

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

    /* ---- Phase 2 : Raffinement (pas 0.5 dBm) ---- */
    p_min = p_high;
    while (p_high >= p_low)
    {
        set_pwr(r, p_high);
        Sleep(100);
        detected = read_tag_once(r, target_epc, &rssi);

        if (detected)
        {
            p_min = p_high;
            if (rssi_out) *rssi_out = rssi;
            p_high -= SWEEP_POWER_STEP_FINE;
        }
        else
        {
            break;
        }
    }

    return p_min;
}

/* ================================================================
   m6e_sweep — fonction principale
   ================================================================ */
int m6e_sweep(TMR_Reader* r,
    STM32Info* stm32,
    const char* target_epc)
{
    int      antenna_num;
    char     fname[256];
    char     date_str[32];
    FILE* fp;
    uint32_t freq;
    int32_t  p_min;
    int16_t  rssi;
    char     ts[32];
    time_t   now_t;
    struct tm* t_now;

    int freq_count = (SWEEP_FREQ_STOP - SWEEP_FREQ_START)
        / SWEEP_FREQ_STEP + 1;

    /* ---- Affichage infos ---- */
    printf("\n===================================\n");
    printf("  SWEEP P_min = f(Frequence)\n");
    printf("===================================\n");
    printf("  Freq  : %d -> %d kHz (pas %d kHz) = %d pts\n",
        SWEEP_FREQ_START, SWEEP_FREQ_STOP,
        SWEEP_FREQ_STEP, freq_count);
    printf("  Power : %.1f -> %.1f dBm\n",
        SWEEP_POWER_START / 100.0f,
        SWEEP_POWER_STOP / 100.0f);
    printf("  Algo  : Dichotomie + raffinement 0.5 dBm\n");
    printf("  EPC   : %s\n\n", target_epc);

    /* ---- Saisie numero antenne ---- */
    printf("  Entrez le numero d'antenne (1-72) : ");
    fflush(stdout);

    if (scanf("%d", &antenna_num) != 1 ||
        antenna_num < 1 || antenna_num > 72)
    {
        printf("\n  [ERREUR] Numero invalide (1-72)\n");
        return 0;
    }
    /* Vider le buffer stdin */
    while (getchar() != '\n');

    printf("  Antenne choisie : %d\n\n", antenna_num);

    /* ---- Switch antenne ---- */
    if (stm32->handle != INVALID_HANDLE_VALUE)
    {
        printf("  Switch *%d# ...", antenna_num);
        fflush(stdout);
        if (stm32_send_cmd(stm32, antenna_num))
            printf(" OK\n");
        else
            printf(" [!] pas de reponse STM32\n");
        Sleep(1500);
    }

    /* ---- Creer le fichier CSV ---- */
    now_t = time(NULL);
    t_now = localtime(&now_t);
    strftime(date_str, sizeof(date_str), "%Y%m%d_%H%M%S", t_now);
    sprintf(fname, "%ssweep_ant%03d_%s.csv",
        CSV_PATH, antenna_num, date_str);

    fp = fopen(fname, "w");
    if (!fp)
    {
        printf("  [ERREUR] Impossible de creer %s\n", fname);
        return 0;
    }

    /* En-tete CSV */
    fprintf(fp, "Antenne,Freq_kHz,P_min_dBm,RSSI_dBm,Status,EPC,Timestamp\n");
    printf("  CSV : %s\n\n", fname);

    /* ---- Balayage frequences ---- */
    printf("  Balayage...\n\n");
    printf("  %-12s %-12s %-12s\n", "Freq (kHz)", "P_min (dBm)", "RSSI (dBm)");
    printf("  -----------------------------------\n");

    for (freq = SWEEP_FREQ_START;
        freq <= SWEEP_FREQ_STOP;
        freq += SWEEP_FREQ_STEP)
    {
        printf("  %-12u ", freq);
        fflush(stdout);

        /* Forcer la frequence */
        set_freq(r, freq);
        Sleep(100);

        /* Trouver P_min par dichotomie + raffinement */
        rssi = -999;
        p_min = find_pmin(r, target_epc, freq, &rssi);

        /* Affichage et CSV */
        get_timestamp(ts, sizeof(ts));

        if (p_min > 0)
        {
            printf("%-12.1f %-12d\n",
                p_min / 100.0f, rssi);

            fprintf(fp, "%d,%u,%.1f,%d,OK,%s,%s\n",
                antenna_num, freq,
                p_min / 100.0f, rssi,
                target_epc, ts);
        }
        else
        {
            printf("DEAD ZONE    ---\n");

            fprintf(fp, "%d,%u,-1,-999,DEAD_ZONE,%s,%s\n",
                antenna_num, freq,
                target_epc, ts);
        }

        fflush(fp);
    }

    fclose(fp);

    printf("\n===================================\n");
    printf("  SWEEP TERMINE\n");
    printf("  Fichier : %s\n", fname);
    printf("===================================\n\n");

    return 1;
}