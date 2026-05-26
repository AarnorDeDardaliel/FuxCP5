#!/usr/bin/env bash
# Régénère les 16 configs de 5e espèce (2v vt1-4 ; 3v vt{1,2,3}-{1,2,3,4}) avec le binaire
# FINAL (patch SP5_H6 + dump isThirdSpeciesArray), puis AUDITE uniquement les vraies cellules
# 3e (drapeau=1) : temps faibles régis par SP5_H6, temps forts par H1. Met à jour l'INDEX.
# Les 4e espèces ne sont PAS touchées (inchangées par le patch ; déjà régénérées).
set -uo pipefail
export ROOT=/home/Sacha/Bureau/Mémoire/FuxCP5/FuxCP
export BIN="$ROOT/compiled/Generations"
export REF="$ROOT/results/cf8_reference"
export AUD="$ROOT/c++/scripts/audit_fifth.py"
export LOG="$REF/regen5_verify.log"
export RES="$REF/regen5_verify_results.csv"
export TIMEOUT=180000

cd "$ROOT"
echo "=== regen+audit 5e $(date '+%F %T') (timeout ${TIMEOUT}ms, patch SP5_H6 + dump) ===" > "$LOG"
echo "nv,label,status,score,nsol,third_cells,phantom_cells,weak_illegal,downbeat_illegal" > "$RES"

JOBS=$(mktemp)
for vt in 1 2 3 4; do echo "2;5;$vt;vt$vt" >> "$JOBS"; done
for a in 1 2 3; do for b in 1 2 3 4; do echo "3;1 5;$a,$b;vt$a-$b" >> "$JOBS"; done; done

do_job(){
  IFS=';' read -r nv esp vtarg label <<< "$1"
  local sub=".regen5/${nv}v_${label}"
  rm -rf "$ROOT/results/$sub"
  "$BIN" "$nv" $esp -c 8 --preset bryce -v "$vtarg" -t "$TIMEOUT" -s 0 \
        -o "$ROOT/results" --subdir "$sub" >/dev/null 2>&1
  local txt destdir="$REF/${nv}voices/5th"
  txt=$(find "$ROOT/results/$sub" -path "*/txt/cf8_*.txt" 2>/dev/null | head -1)
  mkdir -p "$destdir"
  if [ -n "$txt" ]; then
    local term score nsol mid det wk db tc ph
    term=$(grep -aoP 'Terminaison\s*:\s*\K\S+'        "$txt" | head -1)
    score=$(grep -aoP 'Score pondéré\s*:\s*\K\S+'     "$txt" | head -1)
    nsol=$(grep -aoP 'Solutions trouvées\s*:\s*\K\S+' "$txt" | head -1); nsol=${nsol:-0}
    cp "$txt" "$destdir/${label}.txt"
    mid=$(find "$ROOT/results/$sub" -name "*.mid" 2>/dev/null | head -1)
    [ -n "$mid" ] && cp "$mid" "$destdir/${label}.mid"
    local st="NO_SOLUTION"; [ "${nsol}" -gt 0 ] 2>/dev/null && st="${term:-?}"
    wk="-"; db="-"; tc="-"; ph="-"
    if [ "$st" != "NO_SOLUTION" ]; then
      wk=$(python3 "$AUD" "$destdir/${label}.txt" "$nv" 2>/dev/null)
      det=$(python3 "$AUD" "$destdir/${label}.txt" "$nv" 2>&1 1>/dev/null | head -1)
      tc=$(echo "$det" | grep -oP '3rd-cells=\K\d+'); ph=$(echo "$det" | grep -oP 'phantom-cells=\K\d+')
      db=$(echo "$det" | grep -oP 'DOWNBEAT illegal\(H1\)=\K\d+')
    fi
    ( flock 9
      echo "${nv},${label},${st},${score:-—},${nsol},${tc:--},${ph:--},${wk},${db:--}" >> "$RES"
      printf '[%s] %sv 5e %-7s -> %-12s score=%-8s third=%-3s phantom=%-3s WEAK_ill=%s DOWN_ill=%s\n' \
        "$(date '+%T')" "$nv" "$label" "$st" "${score:-—}" "${tc:--}" "${ph:--}" "$wk" "${db:--}" >> "$LOG"
      [ "$wk" != "-" ] && [ "$wk" != "0" ] && echo "  !! SP5_H6 VIOLATION ${nv}v ${label} : $wk temps faible(s) 3e illégaux" >> "$LOG"
    ) 9>"$REF/.regen5.lock"
  else
    ( flock 9
      echo "${nv},${label},NO_OUTPUT,—,0,-,-,-,-" >> "$RES"
      echo "[$(date '+%T')] ${nv}v 5e ${label} -> NO_OUTPUT" >> "$LOG"
    ) 9>"$REF/.regen5.lock"
  fi
  rm -rf "$ROOT/results/$sub"
}
export -f do_job
xargs -P10 -I{} bash -c 'do_job "$@"' _ {} < "$JOBS"
rm -f "$JOBS"

# MAJ des lignes 5th de l'INDEX (statut/score/nsol/fichier)
python3 - "$REF/INDEX.md" "$RES" <<'PY'
import sys, csv, re
index, res = sys.argv[1], sys.argv[2]
rows = {}
with open(res) as f:
    for r in csv.DictReader(f):
        rows[(r['nv'], r['label'][2:])] = r
out = []
pat = re.compile(r'^\|\s*(\d)\s*\|\s*5th\s*\|\s*([0-9-]+)\s*\|')
for ln in open(index, encoding='utf-8').read().splitlines():
    m = pat.match(ln)
    if m and (m.group(1), m.group(2)) in rows:
        r = rows[(m.group(1), m.group(2))]
        f = '—' if r['status'] in ('NO_SOLUTION','NO_OUTPUT') else f"{r['nv']}voices/5th/vt{m.group(2)}.txt"
        out.append(f"| {r['nv']} | 5th | {m.group(2)} | {r['status']} | {r['score']} | {r['nsol']} | {f} |")
    else:
        out.append(ln)
open(index,'w',encoding='utf-8').write("\n".join(out)+"\n")
print("INDEX 5th mis à jour")
PY

echo "=== BILAN 5e (post-patch, audit sur cellules THIRD réelles) ===" >> "$LOG"
awk -F, 'NR>1{tot++; if($8!="-"&&$8!="0")bad++; if($9!="-"&&$9!="0")dbad++}
  END{print "  configs 5e avec solution:",tot,"| SP5_H6 violations:",bad+0,"| H1 downbeat violations:",dbad+0}' "$RES" >> "$LOG"
echo "FIN5" >> "$LOG"
