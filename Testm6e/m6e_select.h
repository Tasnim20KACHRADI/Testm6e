/* ================================================================
   m6e_select.h
   Selection du meilleur tag par delta RSSI
   ================================================================ */
#ifndef M6E_SELECT_H
#define M6E_SELECT_H

#include "common.h"

int m6e_select_best_tag(TagProfile* tags,
                         int         tag_count,
                         int*        best_idx);

#endif
