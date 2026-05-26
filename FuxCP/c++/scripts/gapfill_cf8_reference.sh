#!/usr/bin/env bash
# Comble les 2 trous 3v/3e (vt1-1, vt2-1) restés sans solution à 180s, avec 480s.
set -uo pipefail
ROOT=/home/Sacha/Bureau/Mémoire/FuxCP5/FuxCP
BIN="$ROOT/compiled/Generations"
REF="$ROOT/results/cf8_reference"
LOG="$REF/gapfill.log"
cd "$ROOT"
echo "=== gapfill $(date '+%F %T') (480s) ===" > "$LOG"
for spec in "1,1:vt1-1" "2,1:vt2-1"; do
  v="${spec%%:*}"; label="${spec##*:}"; sub=".cf8_gapfill/${label}"
  echo "[$(date '+%T')] RUN 3v 3e v=$v" >> "$LOG"
  "$BIN" 3 1 3 -c 8 --preset bryce -v "$v" -t 480000 -s 0 -o "$ROOT/results" --subdir "$sub" >>"$LOG" 2>&1
  txt=$(find "$ROOT/results/$sub" -path "*/txt/cf8_*.txt" 2>/dev/null | head -1)
  if [ -n "$txt" ]; then
    cp "$txt" "$REF/3voices/3rd/${label}.txt"
    mid=$(find "$ROOT/results/$sub" -name "*.mid" 2>/dev/null | head -1); [ -n "$mid" ] && cp "$mid" "$REF/3voices/3rd/${label}.mid"
    sco=$(grep -aoP 'Score pondéré\s*:\s*\K\S+' "$txt" | head -1)
    term=$(grep -aoP 'Terminaison\s*:\s*\K\S+' "$txt" | head -1)
    ill=$(python3 "$ROOT/c++/scripts/audit_one.py" "$REF/3voices/3rd/${label}.txt" 3 4 2>/dev/null)
    echo "[$(date '+%T')] $label -> SOLUTION ($term, score=$sco, audit illegal=$ill)" >> "$LOG"
    # met à jour la ligne INDEX (NO_SOLUTION -> statut réel)
    sed -i "s#| 3 | 3rd | ${label} | NO_SOLUTION | — | 0 | — |#| 3 | 3rd | ${label} | ${term} | ${sco} | sol | 3voices/3rd/${label}.txt |#" "$REF/INDEX.md"
  else
    echo "[$(date '+%T')] $label -> toujours NO_SOLUTION à 480s" >> "$LOG"
  fi
  rm -rf "$ROOT/results/$sub"
done
echo "=== gapfill terminé $(date '+%F %T') ===" >> "$LOG"
