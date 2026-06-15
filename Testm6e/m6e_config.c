/* ================================================================
   m6e_config.c
   Configuration region OPEN + hop table EU + puissance max

   API MercuryAPI :
   - TMR_paramSet(REGION_ID)        — TMR_REGION_OPEN
   - TMR_paramSet(RADIO_READPOWER)  — 3150 = 31.5 dBm
   - TMR_paramGet(REGION_HOPTABLE)  — liste frequences kHz
   ================================================================ */
#include "m6e_config.h"

int m6e_config(TMR_Reader* r, uint32_t* freq_table, int* freq_count)
{
    TMR_Status     ret;
    TMR_Region     region = TMR_REGION_OPEN;
    int32_t        power  = POWER_MAX_CDBM;
    TMR_uint32List hop;/*liste pour stocker les frequences du hop table*/
    uint32_t       hop_buf[MAX_FREQ];/*buffer pour stocker les frequences du hop table*/
    int            i;
    
    TMR_paramSet(r, TMR_PARAM_RADIO_READPOWER, &power);
    print_header("ETAPE 2 - CONFIGURATION");

    /* Region OPEN */
    ret = TMR_paramSet(r, TMR_PARAM_REGION_ID, &region);
    if (ret != TMR_SUCCESS)/*ret contient code de retour de tmr paramset , tmr success operation reussie */
    {
        printf("  [ERREUR] Region : %s\n", TMR_strerr(r, ret));/*tmr_strerr fonction SDK pour convertir le code d'erreur en message lisible */
        return 0;
    }
    printf("  Region    : OPEN\n");
	/*si tmr success retourne erreur return  et main   goto end*/
    /* Puissance max */
    ret = TMR_paramSet(r, TMR_PARAM_RADIO_READPOWER, &power);
    if (ret != TMR_SUCCESS)
    {
        printf("  [ERREUR] Puissance : %s\n", TMR_strerr(r, ret));
        return 0;
    }
    printf("  Puissance : %.1f dBm\n\n", power / 100.0f);

    /* Hop table */
    hop.list = hop_buf;
    hop.max  = MAX_FREQ;
    hop.len  = 0;
    ret = TMR_paramGet(r, TMR_PARAM_REGION_HOPTABLE, &hop);
    if (ret != TMR_SUCCESS)
    {
        printf("  [ERREUR] Hop table : %s\n", TMR_strerr(r, ret));
        return 0;
    }

    /* Filtrer 865-869 MHz*/
    
    *freq_count = 0;
    for (i = 0; i < (int)hop.len; i++)
        if (hop_buf[i] >= 865000 && hop_buf[i] <= 869000)
            freq_table[(*freq_count)++] = hop_buf[i];

    print_sep();
    printf("  Frequences EU (865-869 MHz) : %d\n", *freq_count);
    for (i = 0; i < *freq_count; i++)
        printf("  [%02d] %u kHz\n", i + 1, freq_table[i]);
    print_sep();
    return 1;
	/*fonction utiliser pour indexer les frequences dans freq_table et compter le nombre de frequences dans freq_count*/
}
