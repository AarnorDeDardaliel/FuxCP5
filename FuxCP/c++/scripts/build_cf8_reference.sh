#!/usr/bin/env bash
# Génère le dossier de référence cf8_reference/ avec le binaire courant (patché),
# en rejouant la matrice de configs de cf8_FINAL, rangé par voix/espèce/vtype.
# Usage : bash build_cf8_reference.sh   (long ; à lancer en arrière-plan)
set -uo pipefail

ROOT=/home/Sacha/Bureau/Mémoire/FuxCP5/FuxCP
BIN="$ROOT/compiled/Generations"
SRC_MANIFEST="$ROOT/results/cf8_FINAL/manifest.csv"
REF="$ROOT/results/cf8_reference"
WORK="$ROOT/results/.cf8_ref_work"
LOG="$REF/build.log"
TIMEOUT=180000
STAG=0
PRESET=bryce

cd "$ROOT"
rm -rf "$REF"            # on régénère l'organisation, mais on CONSERVE le work dir
mkdir -p "$REF" "$WORK"  # pour réutiliser les générations déjà faites

INDEX="$REF/INDEX.md"
{
  echo "# cf8_reference"
  echo ""
  echo "Contrepoints de référence sur le CF n°8 (Do Reference Grave), régénérés avec"
  echo "le binaire **patché** (correctif H2_3 consonance temps faibles 2 voix 3e espèce,"
  echo "réutilisation multivoix — 2026-05-25). Preset \`$PRESET\`, objectif lex pondéré,"
  echo "timeout ${TIMEOUT} ms, stagnation ${STAG} ms."
  echo ""
  echo "Arborescence : \`<N>voices/<espèce>/vt<vtypes>.{txt,mid}\`."
  echo ""
  echo "| Voix | Espèce | vtype | Statut | Coût | #sol | Fichier |"
  echo "|---|---|---|---|---|---|---|"
} > "$INDEX"

echo "=== build cf8_reference $(date '+%F %T') ===" > "$LOG"

# Configs uniques (Nv, especes, vtype) extraites de la matrice cf8_FINAL (_regpush fusionné).
declare -A seen
tail -n +2 "$SRC_MANIFEST" | while IFS=, read -r config voices species vtype status rest; do
  [ -z "$config" ] && continue
  base="${config%_regpush}"
  nv="$voices"
  esp_part=$(printf '%s' "$base" | sed -E 's/.*_sp([0-9-]+)_.*/\1/')
  vt_part=$(printf '%s'  "$base" | sed -E 's/.*_vt([0-9-]+).*/\1/')
  esp_args=$(printf '%s' "$esp_part" | tr '-' ' ')
  vt_args=$(printf '%s'  "$vt_part"  | tr '-' ',')
  spname="$species"
  key="${nv}_${esp_part}_${vt_part}"
  if [ -n "${seen[$key]:-}" ]; then continue; fi
  seen[$key]=1

  sub=".cf8_ref_work/${key}"
  # Réutilise une génération déjà présente (solution cf8_*.txt OU error_*.txt = aucune solution).
  existing=$(find "$ROOT/results/$sub" \( -path "*/txt/cf8_*.txt" -o -path "*/txt/error_*.txt" \) 2>/dev/null | head -1)
  if [ -z "$existing" ]; then
    echo "[$(date '+%T')] RUN  nv=$nv esp='$esp_args' vt='$vt_args' ($spname)" >> "$LOG"
    "$BIN" "$nv" $esp_args -c 8 --preset "$PRESET" -v "$vt_args" \
          -t $TIMEOUT -s $STAG -o "$ROOT/results" --subdir "$sub" >>"$LOG" 2>&1
  else
    echo "[$(date '+%T')] SKIP $key (déjà généré)" >> "$LOG"
  fi

  # La VRAIE solution est sous txt/cf8_*.txt (le préfixe cf8_ exclut error_*.txt et cantus_firmus_cf8.txt).
  txt=$(find "$ROOT/results/$sub" -path "*/txt/cf8_*.txt" 2>/dev/null | head -1)
  destdir="$REF/${nv}voices/${spname}"
  mkdir -p "$destdir"
  label="vt${vt_part}"
  if [ -n "$txt" ]; then
    term=$(grep -aoP 'Terminaison\s*:\s*\K\S+'        "$txt" | head -1)
    score=$(grep -aoP 'Score pondéré\s*:\s*\K\S+'     "$txt" | head -1)
    nsol=$(grep -aoP 'Solutions trouvées\s*:\s*\K\S+' "$txt" | head -1)
    nsol=${nsol:-0}
    cp "$txt" "$destdir/${label}.txt"
    mid=$(find "$ROOT/results/$sub" -name "*.mid" 2>/dev/null | head -1)
    [ -n "$mid" ] && cp "$mid" "$destdir/${label}.mid"
    if [ "${nsol}" -gt 0 ] 2>/dev/null; then st="${term:-?}"; else st="NO_SOLUTION"; fi
    echo "| $nv | $spname | $vt_part | $st | ${score:-—} | $nsol | ${nv}voices/${spname}/${label}.txt |" >> "$INDEX"
    echo "[$(date '+%T')] DONE $key -> $st score=${score:-—} nsol=$nsol" >> "$LOG"
  else
    echo "| $nv | $spname | $vt_part | NO_OUTPUT | — | 0 | — |" >> "$INDEX"
    echo "[$(date '+%T')] DONE $key -> NO_OUTPUT" >> "$LOG"
  fi
done

# work dir conservé (réutilisable pour une ré-organisation rapide ; nettoyage manuel si besoin)
{
  echo ""
  echo "_Build terminé : $(date '+%F %T')._"
} >> "$INDEX"
echo "=== build terminé $(date '+%F %T') ===" >> "$LOG"
