#!/usr/bin/env bash
# Showcase generation pool — CF7 (C major reference), 12 min/run, no stagnation,
# production branching (afc+rnd), objective = pond. Runs SEQUENTIALLY so the
# time-vs-cost figures stay clean (no CPU contention).
#
# Scenario set: all feasible CF-lowest cases from FuxCP/config/GenData/{classic,multi}.csv
#   - negative v_types excluded (CF must be the lowest voice)
#   - classic 4v transposed to positive registers {0,1,2}
#   - 4v uniform sp4/sp5 omitted (below "No result" in multi.csv = infeasible)
#
# Each scenario writes to its own subdir so same-species/different-register runs
# do not collide. A pool_summary.csv row is appended per run.
set -u

BIN="/home/Sacha/Bureau/Mémoire/FuxCP5/FuxCP/compiled/Generations"
ROOT="/home/Sacha/Bureau/Mémoire/FuxCP5/FuxCP/results"
POOL="showcase_pool"
CF=7
TIMEOUT=720000   # 12 min
STAGN=0          # no stagnation
PRESET="default"
OBJ="pond"

SUMMARY="$ROOT/$POOL/pool_summary.csv"
mkdir -p "$ROOT/$POOL"
echo "combo_id,nb_voix,species,v_type,best_cost,termination,ms_total,ms_first,nb_solutions" > "$SUMMARY"

# Each line: "<nb_voix>|<species space-sep>|<v_type comma-sep>"
SCENARIOS=(
  # ---- 2 voices (classic, single CP) ----
  "2|1|2"  "2|1|1"  "2|1|0"
  "2|2|2"  "2|2|1"  "2|2|0"
  "2|3|2"  "2|3|1"  "2|3|0"
  "2|4|2"  "2|4|0"
  "2|5|2"  "2|5|1"  "2|5|0"
  # ---- 3 voices (classic uniform) ----
  "3|1 1|4,2"  "3|1 1|0,2"  "3|1 1|2,0"
  "3|2 2|4,2"  "3|2 2|0,2"  "3|2 2|2,0"  "3|2 2|1,2"
  "3|3 3|4,2"  "3|3 3|0,2"  "3|3 3|2,0"  "3|3 3|1,0"
  "3|4 4|4,2"  "3|4 4|0,2"  "3|4 4|2,0"
  "3|5 5|4,2"  "3|5 5|0,2"  "3|5 5|2,0"
  # ---- 3 voices (multi mixed) ----
  "3|2 3|1,2"  "3|2 3|0,2"  "3|2 3|2,0"
  "3|3 5|2,0"
  # ---- 4 voices (transposed classic + multi, positive registers) ----
  "4|1 1 1|0,1,2"
  "4|2 2 2|0,1,2"
  "4|3 3 3|0,1,2"
  "4|2 3 4|2,1,0"
)

total=${#SCENARIOS[@]}
i=0
t0=$(date +%s)
for sc in "${SCENARIOS[@]}"; do
  i=$((i+1))
  nv="${sc%%|*}"
  rest="${sc#*|}"
  sp="${rest%%|*}"
  vt="${rest#*|}"

  sp_slug="${sp// /-}"
  vt_slug="${vt//,/-}"
  combo="cf${CF}_${nv}v_sp${sp_slug}_vt${vt_slug}"

  echo "[$i/$total] $combo (nv=$nv sp=[$sp] vt=$vt)"
  "$BIN" "$nv" $sp -c "$CF" --preset "$PRESET" -t "$TIMEOUT" -s "$STAGN" \
        -m "$OBJ" -v "$vt" --subdir "$POOL/$combo" -o "$ROOT" \
        > "$ROOT/$POOL/${combo}.log" 2>&1

  # Parse the per-run CSV (single data row) for the summary.
  csv=$(find "$ROOT/$POOL/$combo" -name "*.csv" -path "*/csv/*" 2>/dev/null | head -1)
  if [ -n "$csv" ]; then
    row=$(sed -n '2p' "$csv")
    # CSV cols: cf_id,cf_name,preset,borrow_mode,nb_voix,especes,v_type,timeout_ms,
    #           stagnation_ms,temps_total_ms,temps_premiere_solution_ms,
    #           temps_derniere_amelioration_ms,nb_solutions,nb_ameliorations,cout_final,
    #           terminaison,...
    best_cost=$(echo "$row" | cut -d',' -f15)
    term=$(echo "$row" | cut -d',' -f16)
    ms_total=$(echo "$row" | cut -d',' -f10)
    ms_first=$(echo "$row" | cut -d',' -f11)
    nb_sol=$(echo "$row" | cut -d',' -f13)
  else
    best_cost="NA"; term="NO_CSV"; ms_total="NA"; ms_first="NA"; nb_sol="NA"
  fi
  echo "$combo,$nv,\"$sp\",\"$vt\",$best_cost,$term,$ms_total,$ms_first,$nb_sol" >> "$SUMMARY"
  echo "    -> best_cost=$best_cost term=$term ms_total=$ms_total nb_sol=$nb_sol"
done
elapsed=$(( $(date +%s) - t0 ))
echo "=== Pool terminé : $total runs en ${elapsed}s. Summary : $SUMMARY ==="
