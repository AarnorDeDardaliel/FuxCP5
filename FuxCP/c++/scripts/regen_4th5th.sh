#!/usr/bin/env bash
# Régénère les 4e et 5e espèces (2v + 3v) du dossier cf8_reference avec le binaire
# PATCHÉ (correctif SP5_H6 : consonance/diminution sur les 3 temps faibles d'une
# mesure de 5e fleurie en noires). Parallélise (10 jobs), audite chaque 5e dès
# qu'elle finit, met à jour les lignes 4th/5th de l'INDEX. Timeout 180 s (= ref).
#
# Usage : bash regen_4th5th.sh   (≈12 min ; à lancer en arrière-plan)
set -uo pipefail
export ROOT=/home/Sacha/Bureau/Mémoire/FuxCP5/FuxCP
export BIN="$ROOT/compiled/Generations"
export REF="$ROOT/results/cf8_reference"
export AUD="$ROOT/c++/scripts/audit_fifth.py"
export LOG="$REF/regen45.log"
export RES="$REF/regen45_results.csv"
export TIMEOUT=180000
export STAG=0
export PRESET=bryce

cd "$ROOT"
echo "=== regen 4e/5e $(date '+%F %T') (timeout ${TIMEOUT}ms, patch SP5_H6) ===" > "$LOG"
echo "nv,species,label,status,score,nsol,florid_illegal" > "$RES"

# Matrice : nv;esp(args);vtarg;species;label
JOBS=$(mktemp)
for vt in 1 2 3;       do echo "2;4;$vt;4th;vt$vt"        >> "$JOBS"; done
for vt in 1 2 3 4;     do echo "2;5;$vt;5th;vt$vt"        >> "$JOBS"; done
for a in 1 2 3; do for b in 1 2 3 4; do echo "3;1 4;$a,$b;4th;vt$a-$b" >> "$JOBS"; done; done
for a in 1 2 3; do for b in 1 2 3 4; do echo "3;1 5;$a,$b;5th;vt$a-$b" >> "$JOBS"; done; done

do_job(){
  IFS=';' read -r nv esp vtarg species label <<< "$1"
  local sub=".regen45/${nv}v_${species}_${label}"
  rm -rf "$ROOT/results/$sub"
  "$BIN" "$nv" $esp -c 8 --preset "$PRESET" -v "$vtarg" \
        -t "$TIMEOUT" -s "$STAG" -o "$ROOT/results" --subdir "$sub" >/dev/null 2>&1
  local txt destdir
  txt=$(find "$ROOT/results/$sub" -path "*/txt/cf8_*.txt" 2>/dev/null | head -1)
  destdir="$REF/${nv}voices/${species}"
  mkdir -p "$destdir"
  if [ -n "$txt" ]; then
    local term score nsol mid ill="-"
    term=$(grep -aoP 'Terminaison\s*:\s*\K\S+'        "$txt" | head -1)
    score=$(grep -aoP 'Score pondéré\s*:\s*\K\S+'     "$txt" | head -1)
    nsol=$(grep -aoP 'Solutions trouvées\s*:\s*\K\S+' "$txt" | head -1); nsol=${nsol:-0}
    cp "$txt" "$destdir/${label}.txt"
    mid=$(find "$ROOT/results/$sub" -name "*.mid" 2>/dev/null | head -1)
    [ -n "$mid" ] && cp "$mid" "$destdir/${label}.mid"
    local st="NO_SOLUTION"; [ "${nsol}" -gt 0 ] 2>/dev/null && st="${term:-?}"
    if [ "$species" = "5th" ] && [ "$st" != "NO_SOLUTION" ]; then
      ill=$(python3 "$AUD" "$destdir/${label}.txt" "$nv" 2>/dev/null)
    fi
    (
      flock 9
      echo "${nv},${species},${label},${st},${score:-—},${nsol},${ill}" >> "$RES"
      printf '[%s] %sv %s %-7s -> %-12s score=%-8s nsol=%-3s florid_illegal=%s\n' \
        "$(date '+%T')" "$nv" "$species" "$label" "$st" "${score:-—}" "$nsol" "$ill" >> "$LOG"
      [ "$ill" != "-" ] && [ "$ill" != "0" ] && \
        echo "  !! VIOLATION ${nv}v ${species} ${label} : ${ill} dissonance(s) disjointe(s) en mesure fleurie" >> "$LOG"
    ) 9>"$REF/.regen45.lock"
  else
    ( flock 9
      echo "${nv},${species},${label},NO_OUTPUT,—,0,-" >> "$RES"
      echo "[$(date '+%T')] ${nv}v ${species} ${label} -> NO_OUTPUT" >> "$LOG"
    ) 9>"$REF/.regen45.lock"
  fi
  rm -rf "$ROOT/results/$sub"
}
export -f do_job

xargs -P10 -I{} bash -c 'do_job "$@"' _ {} < "$JOBS"
rm -f "$JOBS"

# --- Mise à jour des lignes 4th/5th de l'INDEX ---
python3 - "$REF/INDEX.md" "$RES" <<'PY'
import sys, csv, re
index, res = sys.argv[1], sys.argv[2]
rows = {}
with open(res) as f:
    for r in csv.DictReader(f):
        vtpart = r['label'][2:]  # "vt1-2" -> "1-2"
        rows[(r['nv'], r['species'], vtpart)] = r
lines = open(index, encoding='utf-8').read().splitlines()
out = []
pat = re.compile(r'^\|\s*(\d)\s*\|\s*(\dth)\s*\|\s*([0-9-]+)\s*\|')
for ln in lines:
    m = pat.match(ln)
    if m and (m.group(1), m.group(2), m.group(3)) in rows:
        r = rows[(m.group(1), m.group(2), m.group(3))]
        if r['status'] in ('NO_SOLUTION', 'NO_OUTPUT'):
            f = '—'
        else:
            f = f"{r['nv']}voices/{r['species']}/{r['label']}.txt"
        out.append(f"| {r['nv']} | {r['species']} | {m.group(3)} | {r['status']} | {r['score']} | {r['nsol']} | {f} |")
    else:
        out.append(ln)
open(index, 'w', encoding='utf-8').write("\n".join(out) + "\n")
print("INDEX mis à jour")
PY

echo "=== regen terminé $(date '+%F %T') ===" >> "$LOG"
# Bilan audit 5e
echo "### BILAN AUDIT 5e ESPÈCE (post-patch) ###" >> "$LOG"
awk -F, 'NR>1 && $2=="5th"{tot++; if($7!="-"&&$7!="0"){bad++; print "  VIOLATION",$1"v",$3,"illegal="$7}} END{print "  5e auditées:",tot,"| avec violation:",bad+0}' "$RES" >> "$LOG"
echo "FIN" >> "$LOG"
