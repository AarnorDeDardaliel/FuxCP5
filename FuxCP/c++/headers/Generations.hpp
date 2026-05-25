#pragma once


#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <map>
#include <string>
#include <utility>
#include <tuple>
#include <algorithm>
#include <gecode/int.hh>
#include <gecode/search.hh>

#include "Utilities.hpp"
#include "Midi.hpp"
#include "CounterpointProblems/CounterpointUtils.hpp"
#include "CounterpointProblems/CounterpointProblem.hpp"
#include "CounterpointProblems/TwoVoiceCounterpoint.hpp"
#include "CounterpointProblems/ThreeVoiceCounterpoint.hpp"
#include "CounterpointProblems/FourVoiceCounterpoint.hpp"
#include "ConfigLoader.hpp"

// =============================================================
// Internal generation case
// Used by the engine and by the API / CLI.
// =============================================================

struct GenerationCase {
    int cf_id = 1;
    vector<int> cf;
    string cf_name;
    string cf_scale;
    bool use_preset_cf = false;

    int n_voices = 0;
    vector<Species> spList;
    vector<int> v_type;

    string preset_name = "default";
    vector<int> melodic_params;
    vector<int> general_params;
    vector<int> specific_params;
    vector<int> importance;
    int borrow_mode = 0;
    ObjectiveMode obj_mode = OBJECTIVE_PONDERED;

    int timeout_ms = 300000;
    int stagnation_ms = 120000;

    string output_root = "../../results";
    string output_subdir;
};

// =============================================================
// Public result returned by reusable generation entrypoints
// =============================================================

struct GenerationResult {
    // ----- Bench Results -----
    bool success = false;
    string termination = "NONE";

    int nb_solutions = 0;
    int nb_improvements = 0;
    double best_cost = 1e18; // must start above any possible cost, else no solution is ever kept
    double ms_first_solution = 0;
    double ms_last_improve = 0;
    double ms_total = 0;

    Gecode::Search::Statistics stats;

    vector<int> full_solution;
    vector<pair<vector<int>, Species>> generated_voices;

    vector<tuple<int,double,double>> improvements;
    vector<tuple<int,double,double>> checkpoints;
    vector<tuple<int,double,double,string>> solutions_log;

    // ----- General informations -----
    string midi_path;
    string txt_path;
    string csv_path;
    string error_txt_path;

    vector<int> resolved_cf;
    string resolved_cf_name;
    string resolved_cf_scale;
};

// =============================================================
// Generation engine facade
// =============================================================

class Generations {
public:
    Generations() = default;

    // Main reusable engine entrypoint.
    // This is the one both CLI-side wrappers and API-side wrappers should call.
    static GenerationResult run_generation_case(GenerationCase& gc, bool save_outputs = true, bool verbose = true);

};

// Optional convenience helpers if you want them callable outside.
bool resolve_cf(GenerationCase& gc, bool verbose=true);
bool apply_preset(GenerationCase& gc);