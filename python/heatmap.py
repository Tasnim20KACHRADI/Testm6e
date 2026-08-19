#!/usr/bin/env python3
"""
heatmap.py - genere une heatmap Angle x Antenne a partir du CSV
produit par le mode RUN_MODE=4 (m6e_heatmap.c) du projet Testm6e.

Le CSV contient une colonne "Angle_deg" ajoutee par le firmware
moteur (control_arch_motor_v22_v01_2.ino) via motor.c/motor.h.

Usage :
    python heatmap.py heatmap_20260819_153000.csv
    python heatmap.py heatmap_20260819_153000.csv --value pmin
    python heatmap.py heatmap_20260819_153000.csv --freq 865700
    python heatmap.py heatmap_20260819_153000.csv --axe Theta --out out.png
"""
import argparse
import os
import sys
from typing import Optional

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

REQUIRED_COLUMNS = {
    "Freq_kHz", "Antenne", "Axe", "P_min_dBm",
    "RSSI_dBm", "Status", "EPC", "Timestamp", "Angle_deg",
}


def load_csv(path: str) -> pd.DataFrame:
    df = pd.read_csv(path)
    missing = REQUIRED_COLUMNS - set(df.columns)
    if missing:
        sys.exit(f"[ERREUR] colonnes manquantes dans le CSV : {missing}\n"
                  f"         (verifiez que le .ino/firmware genere bien Angle_deg)")
    return df


def build_pivot(df: pd.DataFrame, value_col: str,
                 freq_khz: Optional[int], axe: Optional[str]) -> pd.DataFrame:
    data = df.copy()

    if axe is not None:
        data = data[data["Axe"].str.lower() == axe.lower()]
        if data.empty:
            sys.exit(f"[ERREUR] aucune ligne pour Axe={axe}")

    if freq_khz is not None:
        data = data[data["Freq_kHz"] == freq_khz]
        if data.empty:
            sys.exit(f"[ERREUR] aucune ligne pour Freq_kHz={freq_khz}")

    # DEAD_ZONE -> NaN pour ne pas fausser l'echelle de couleur
    data.loc[data["Status"] == "DEAD_ZONE", value_col] = np.nan

    pivot = data.pivot_table(index="Angle_deg", columns="Antenne",
                              values=value_col, aggfunc="mean")
    pivot = pivot.sort_index().reindex(sorted(pivot.columns), axis=1)
    return pivot


def plot_heatmap(pivot: pd.DataFrame, value_col: str,
                  freq_label: str, out_path: str, show: bool):
    width = max(8.0, 0.28 * len(pivot.columns))
    height = max(5.0, 0.35 * len(pivot.index))
    fig, ax = plt.subplots(figsize=(width, height))

    masked = np.ma.masked_invalid(pivot.values)
    cmap = plt.get_cmap("viridis").copy()
    cmap.set_bad(color="lightgray")

    im = ax.imshow(masked, aspect="auto", cmap=cmap, origin="lower")

    ax.set_xticks(range(len(pivot.columns)))
    ax.set_xticklabels(pivot.columns, rotation=90, fontsize=7)
    ax.set_yticks(range(len(pivot.index)))
    ax.set_yticklabels([f"{a:g}" for a in pivot.index], fontsize=8)

    ax.set_xlabel("Antenne")
    ax.set_ylabel("Angle moteur (deg)")
    label = "RSSI (dBm)" if value_col == "RSSI_dBm" else "P_min (dBm)"
    ax.set_title(f"Heatmap arche - {label}{freq_label}")

    cbar = fig.colorbar(im, ax=ax)
    cbar.set_label(label)

    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    print(f"Heatmap enregistree : {out_path}")

    if show:
        plt.show()


def main():
    parser = argparse.ArgumentParser(
        description="Heatmap Angle x Antenne (arche RFID Testm6e)")
    parser.add_argument("csv_path",
        help="fichier CSV genere par le mode heatmap (RUN_MODE=4)")
    parser.add_argument("--value", choices=["rssi", "pmin"], default="rssi",
        help="grandeur affichee (defaut: rssi)")
    parser.add_argument("--freq", type=int, default=None,
        help="filtrer sur une frequence precise en kHz "
             "(sans cet argument : moyenne sur toutes les frequences)")
    parser.add_argument("--axe", choices=["Theta", "Phi"], default=None,
        help="filtrer sur un axe d'antenne")
    parser.add_argument("--out", default=None, help="chemin du PNG de sortie")
    parser.add_argument("--no-show", action="store_true",
        help="ne pas ouvrir de fenetre, juste enregistrer le PNG")
    args = parser.parse_args()

    df = load_csv(args.csv_path)
    value_col = "RSSI_dBm" if args.value == "rssi" else "P_min_dBm"

    pivot = build_pivot(df, value_col, args.freq, args.axe)

    freq_label = f" @ {args.freq} kHz" if args.freq is not None else " (moyenne toutes frequences)"
    out_path = args.out or (
        os.path.splitext(args.csv_path)[0] + f"_heatmap_{args.value}.png")

    plot_heatmap(pivot, value_col, freq_label, out_path, show=not args.no_show)


if __name__ == "__main__":
    main()
