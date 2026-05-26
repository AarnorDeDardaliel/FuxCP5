#!/usr/bin/env bash
# Audit "fail-fast" pendant le build cf8_reference : dès qu'un cas 2e/3e espèce
# AVEC solution apparaît, contrôle les dissonances disjointes sur temps faibles.
# - 1 violation  -> sort en code 2 (notification immédiate) pour couper le build.
# - build fini sans violation -> sort en code 0 (récap propre).
set -uo pipefail
ROOT=/home/Sacha/Bureau/Mémoire/FuxCP5/FuxCP
REF="$ROOT/results/cf8_reference"
AUD="$ROOT/c++/scripts/audit_one.py"
LOG="$REF/audit_live.log"
shopt -s nullglob
declare -A seen
echo "=== audit live démarré $(date '+%F %T') ===" > "$LOG"

for it in $(seq 1 320); do
  for f in "$REF"/2voices/2nd/*.txt "$REF"/2voices/3rd/*.txt \
           "$REF"/3voices/2nd/*.txt "$REF"/3voices/3rd/*.txt; do
    [ -n "${seen[$f]:-}" ] && continue
    seen[$f]=1
    case "$f" in *2voices*) nv=2;; *) nv=3;; esac
    case "$f" in *2nd*)     npm=2;; *) npm=4;; esac
    ill=$(python3 "$AUD" "$f" "$nv" "$npm" 2>>"$LOG")
    echo "[$(date '+%T')] $(basename $(dirname $(dirname $f)))/$(basename $(dirname $f))/$(basename $f) -> illegal=$ill" >> "$LOG"
    if [ "${ill:-0}" -gt 0 ] 2>/dev/null; then
      echo "### VIOLATION : $f (disjointes illégales = $ill)"
      tail -25 "$LOG"
      exit 2
    fi
  done
  if grep -q "build terminé" "$REF/build.log" 2>/dev/null; then
    echo "### BUILD TERMINÉ — aucun cas 2e/3e espèce illégal"
    echo "Audités OK (illegal=0) : $(grep -c 'illegal=0' "$LOG")"
    echo "Total audités           : $(grep -c 'illegal=' "$LOG")"
    exit 0
  fi
  sleep 60
done
echo "### cap temps atteint sans fin de build"; exit 1
