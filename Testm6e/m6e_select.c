/* ================================================================
   m6e_select.c
   FILTRE 2 — Selection par delta RSSI

   Pour chaque tag valide (apres filtre phantom) :
   1. Calculer RSSI moyen antenne 1
   2. Calculer RSSI moyen antenne 31
   3. Delta = |mean_a1 - mean_a31|

   Criteres :
   1. Tag detecte sur les DEUX antennes (obligatoire)
   2. Delta le plus GRAND -> tag le mieux positionne dans l'arche
   3. A delta egal -> RSSI global le plus fort
   ================================================================ */
#include "m6e_select.h"

int m6e_select_best_tag(TagProfile* tags,
                         int         tag_count,
                         int*        best_idx)
{
    int   i;
    float best_delta = -1.0f;
    float best_mean  = -9999.0f;
    int   found      = 0;

    print_header("ETAPE 5 - SELECTION PAR RSSI");

    /* Calcul et affichage du profil de chaque tag */
    for (i = 0; i < tag_count; i++)
    {
        TagProfile* t = &tags[i];

        t->mean_a1   = compute_mean(t->rssi_a1,  t->count_a1);/*calculer la moyenne des RSSI pour l'antenne 1,t(tableau de rssi)]*/
        t->mean_a31  = compute_mean(t->rssi_a31, t->count_a31);
        t->seen_both = (t->count_a1 > 0 && t->count_a31 > 0);
        t->delta     = t->seen_both
                       ? fabsf(t->mean_a1 - t->mean_a31)
                       : -1.0f;

        printf("Tag [%d] : %s\n", i + 1, t->epc);
        printf("  Ant%-3d : %d lect. | moy = %.1f dBm\n",
               ANTENNA_1,  t->count_a1,  t->mean_a1);
        printf("  Ant%-3d : %d lect. | moy = %.1f dBm\n",
               ANTENNA_31, t->count_a31, t->mean_a31);

        if (t->seen_both)
            printf("  Delta  : %.1f dBm\n\n", t->delta);
        else
            printf("  [!] Une seule antenne\n\n");
    }

    print_sep();
    printf(" SELECTION — Delta RSSI maximum\n");
    print_sep();

    /* Chercher le tag avec le plus grand delta */
    for (i = 0; i < tag_count; i++)
    {
        TagProfile* t = &tags[i];
        if (!t->seen_both) continue;

        float global = (t->mean_a1 + t->mean_a31) / 2.0f;

        if (t->delta > best_delta ||
           (fabsf(t->delta - best_delta) < 0.1f &&
            global > best_mean))
        {
            best_delta = t->delta;
            best_mean  = global;
            *best_idx  = i;
            found      = 1;
        }
    }

    
    /* Pas de fallback — les deux antennes sont obligatoires */
    if (!found)
    {
        printf("  [ERREUR] Aucun tag detecte sur les 2 antennes.\n");
        printf("  -> Verifiez positionnement du tag dans l'arche\n");
        printf("  -> Verifiez connexion STM32 et switch antennes\n");
        return 0;
    }

    printf("  => EPC     : %s\n", tags[*best_idx].epc);
    printf("  => Ant%-3d  : %.1f dBm\n",
        ANTENNA_1, tags[*best_idx].mean_a1);
    printf("  => Ant%-3d  : %.1f dBm\n",
        ANTENNA_31, tags[*best_idx].mean_a31);
    printf("  => Delta   : %.1f dBm\n", tags[*best_idx].delta);
    printf("===================================\n\n");
    return 1;
}
