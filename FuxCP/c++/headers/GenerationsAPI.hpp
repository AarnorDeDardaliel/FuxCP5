#pragma once

#include "Generations.hpp"

// =====================================================================
// GenerationAPI.hpp — simplified public entrypoint for generation.
//
// This API is meant for external callers that want to generate a
// counterpoint without depending on the CLI or on the internal workflow
// of the generation engine.
//
// This interface is intentionally lighter than the CLI: the cantus firmus 
// is always provided directly, no campaign logic, no list commands, no 
// testing-specific options
//
// Typical usage:
//   1. Fill a GenerationInput structure
//   2. Call generate_counterpoint(input)
//   3. Read the returned GenerationResult
// 
// For custom metaparameters (melodic_params, general_params, specific_params and importance) :
//   - Add a line in config > presets.csv
//   - set 'preset_name' to the name of your new preset
// 
// For example of uses, see "QUICK TEST on API" part in Tests/main.cpp
// 
// =====================================================================


// Input of the public generation API.
struct GenerationInput {
    // ----- Required -----

    // MIDI notes of the cantus firmus. The first note also acts as the tonal reference for scale detection.
    // Example: {60, 62, 65, 64, 67, 65, 64, 62, 60}
    vector<int> cf_notes;

    // Species, of the generated counterpoint voices only.
    // Example: {FIRST_SPECIES, THIRD_SPECIES}
    // This means: 1 cantus firmus, 1 first-species generated voice, 1 third-species generated voice
    vector<Species> species;

    // Voice-position / register types of the generated counterpoint voices.
    // Possible value of counterpoint notes are (cf_notes[0] + v_type * 6) +/- PERFECT_OCTAVE, with PERFECT_OCTAVE = 12
    // Example: {0, 2}
    // This means : first counterpoint centered around cf_notes[0], second counterpoint around cf_notes[0] + 2 * 6
    vector<int> v_type;

    
    // ----- Optionnal -----
    // If not specified, the engine should fall back to its default preset behavior

    // Name of the preset to load from presets.csv. 
    string preset_name = "";

    // Maximum total runtime of the search, in milliseconds.
    // Setting a value of 0 disables timeout max
    int timeout_ms = -1;

    // Maximum allowed stagnation time, in milliseconds. Search stops if no improvement is found during this delay.
    // Setting a value of 0 disables stagnation
    int stagnation_ms = -1;

    // Objective mode used by the solver.
    //
    // Possible values:
    //   OBJECTIVE_LEX 
    //   OBJECTIVE_TOTAL 
    //   OBJECTIVE_MIXED
    //   OBJECTIVE_PONDERED
    ObjectiveMode obj_mode = OBJECTIVE_PONDERED;


    // Root directory where result files should be written.
    string output_root = "";

    // Optional subdirectory added under output_root. Useful for grouping several generations in the same campaign or test.
    string output_subdir = "";

    // If true, the engine writes output files (txt / csv / midi when relevant). If false, generation is run without saving any files.
    bool save_outputs = true;

    // If true, the engine log  (txt / csv / midi when relevant). If false, generation will only print errors and checkpoints.
    bool verbose = false;
};

// Launches one generation from a fully specified GenerationInput.
//
// The returned GenerationResult contains:
//   - search statistics
//   - termination status
//   - the full solution
//   - generated voices split by species
//   - resolved cantus firmus metadata
//   - output file paths when saving is enabled
GenerationResult generate_counterpoint(const GenerationInput& input);

GenerationResult generate_counterpoint(vector<int> cf_notes, vector<Species> species, vector<int> v_type);