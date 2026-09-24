/* ================================================================
   m6e_scan.c
   Lecture tags sur antenne 1 et antenne 31

   IMPORTANT — Architecture physique :
   - Le M6e DevKit a UN SEUL port antenne physique : port 1
   - La STM32 commute l'arche RF entre position 1 et position 31
   - TMR_RP_init_simple utilise TOUJOURS le port physique 1 du M6e
   - C'est la commande STM32 (*1# ou *31#) qui determine
     quelle antenne de l'arche est active

   FILTRE 1 — ANTI-PHANTOM :
   Accepte uniquement les EPC de EPC_EXPECTED_BITS (96 bits)
   ================================================================ */
#include "m6e_scan.h"

   /* ================================================================
      m6e_read_tags_antenna — lecture N fois sur le port physique M6e
      antenna_param : toujours 1 (port physique unique du M6e DevKit)
      is_ant1       : 1 = stocker dans rssi_a1, 0 = dans rssi_a31
      ================================================================ */
static void m6e_read_tags_antenna(TMR_Reader* r,
    TagProfile* tags,
    int* tag_count,
	int         is_ant1)/*ant1 = 1 -> stocker RSSI dans rssi_a1, sinon dans rssi_a31*/
{
    TMR_Status      ret;
    TMR_ReadPlan    plan;
    TMR_TagReadData trd;
    char            epc_buf[EPC_STR_MAX];
    int             pass, tag_idx;

    /*
     * Port physique M6e = 1 (unique sur le DevKit)
     * La position de l'arche est geree par la STM32
     */
    uint8_t m6e_port = 1;

    ret = TMR_RP_init_simple(&plan, 1, &m6e_port,
        TMR_TAG_PROTOCOL_GEN2, 1000);
    if (ret != TMR_SUCCESS)
    {
        printf("  [ERREUR] RP_init_simple : %s\n", TMR_strerr(r, ret));
        return;
    }

    ret = TMR_paramSet(r, TMR_PARAM_READ_PLAN, &plan);
    if (ret != TMR_SUCCESS)
    {
        printf("  [ERREUR] READ_PLAN : %s\n", TMR_strerr(r, ret));
        return;
    }

	for (pass = 0; pass < READS_PER_ANTENNA; pass++)/*lire plusieurs fois pour chaque position d'antenne afin d'avoir plusieurs mesures RSSI par tag et lisser les variations*/
    {
		ret = TMR_read(r, READ_TIMEOUT_MS, NULL);/*realise une lecture de tags avec un timeout de READ_TIMEOUT_MS ms, les tags lus sont stockés dans le buffer interne du lecteur et peuvent être récupérés avec TMR_getNextTag*/

        if (ret == TMR_ERROR_NO_TAGS_FOUND) continue;
        if (ret != TMR_SUCCESS) continue;

        while (TMR_SUCCESS == TMR_hasMoreTags(r))
        {
            ret = TMR_getNextTag(r, &trd);
            if (ret != TMR_SUCCESS) break;

            /* --- FILTRE 1 : LONGUEUR EPC --- */
            if ((trd.tag.epcByteCount * 8) != EPC_EXPECTED_BITS)
            {
                printf("  [PHANTOM] Tag rejete : %d bits (attendu %d)\n",
                    trd.tag.epcByteCount * 8, EPC_EXPECTED_BITS);
                continue;
            }

            TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, epc_buf);/*parcours le tab et cherche si l epc existe deja */

            tag_idx = find_tag(tags, *tag_count, epc_buf);/*recherche si le tag est déjà présent dans la liste*/
            if (tag_idx == -1)
            {
                if (*tag_count >= MAX_TAGS) continue;
                tag_idx = *tag_count;
                memset(&tags[tag_idx], 0, sizeof(TagProfile));
                strncpy(tags[tag_idx].epc, epc_buf, EPC_STR_MAX - 1);
                tags[tag_idx].epc_bits = trd.tag.epcByteCount * 8;
                (*tag_count)++;
                printf("  [TAG] %s\n", epc_buf);
            }

            if (is_ant1)
            {
                if (tags[tag_idx].count_a1 < READS_PER_ANTENNA)
                    tags[tag_idx].rssi_a1[tags[tag_idx].count_a1++]
                    = trd.rssi;
            }
            else
            {
                if (tags[tag_idx].count_a31 < READS_PER_ANTENNA)
                    tags[tag_idx].rssi_a31[tags[tag_idx].count_a31++]
                    = trd.rssi;
            }
        }
    }
}

/* ================================================================
   m6e_scan_tags
   1. Switch STM32 vers antenne 1 -> lire sur port M6e 1
   2. Switch STM32 vers antenne 31 -> lire sur port M6e 1
   ================================================================ */
int m6e_scan_tags(TMR_Reader* r,
    STM32Info* stm32,
    TagProfile* tags,
    int* tag_count)
{
    *tag_count = 0;

    print_header("ETAPE 4 - SCAN TAGS");

    /* --- Position antenne 1 --- */
    printf("\n Position antenne %d (%d lectures)\n",
        ANTENNA_1, READS_PER_ANTENNA);
    print_sep();
    if (stm32->handle != INVALID_HANDLE_VALUE)
    {
        printf("  Switch STM32 -> position %d ... ", ANTENNA_1);
        printf("%s\n", stm32_send_cmd(stm32, ANTENNA_1)
            ? "OK" : "[!] pas de reponse");
        Sleep(500);
    }

    m6e_read_tags_antenna(r, tags, tag_count, 1);
    printf("  Tags valides : %d\n", *tag_count);

    /* --- Position antenne 31 --- */
    printf("\n Position antenne %d (%d lectures)\n",
        ANTENNA_31, READS_PER_ANTENNA);
    print_sep();

    if (stm32->handle != INVALID_HANDLE_VALUE)
    {
        printf("  Switch STM32 -> position %d ... ", ANTENNA_31);
        printf("%s\n", stm32_send_cmd(stm32, ANTENNA_31)
            ? "OK" : "[!] pas de reponse");
        Sleep(500);
    }
    m6e_read_tags_antenna(r, tags, tag_count, 0);
    printf("  Tags valides : %d\n", *tag_count);

    if (*tag_count == 0)
    {
        printf("\n  [ERREUR] Aucun tag valide.\n");
        printf("  -> EPC attendu : %d bits\n", EPC_EXPECTED_BITS);
        return 0;
    }

    printf("\n===================================\n\n");
    return 1;
}