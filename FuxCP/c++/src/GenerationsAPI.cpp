// =====================================================================
// GenerationAPI.cpp — public API wrapper around the generation engine.
//
// Role:
//   This file exposes a simple programmatic entrypoint for other parts of
//   the project (or external integrations) that need to generate a
//   counterpoint without knowing the internal generation pipeline.
//
// Design:
//   - The actual generation logic stays in Generations.cpp.
//   - This API only converts a clean external input (GenerationInput)
//     into the internal generation case format (GenCase), then delegates
//     execution to the reusable engine function.
//
// Main entrypoint:
//   GenerationResult generate_counterpoint(const GenerationInput& input)
//
// Expected use:
//   - plugin / UI / other project modules
//   - direct generation from a user-provided cantus firmus
//   - integration without exposing CLI-specific options or internals
//
// Not handled here:
//   - command-line parsing
//   - campaign execution
//   - listing presets / cantus firmi / campaigns
//   - advanced testing workflows
//
// Notes:
//   - The cantus firmus can be provided only as MIDI notes.
//   - Presets and configuration data are still resolved through the
//     shared generation engine.
// =====================================================================

#include "../headers/GenerationsAPI.hpp"

static GenerationCase build_gc_from_api_input(const GenerationInput& input) {
    GenerationCase gc;
    gc.cf = input.cf_notes;
    gc.use_preset_cf = false;
    if (!resolve_cf(gc, input.verbose)) { cerr << "  Error in cf resolution !" << endl; }

    gc.n_voices = input.species.size()+1;
    gc.spList = input.species;
    gc.v_type = input.v_type;

    if (input.preset_name != "") gc.preset_name = input.preset_name;
    if (input.timeout_ms != -1) gc.timeout_ms = input.timeout_ms;
    if (input.stagnation_ms != -1) gc.stagnation_ms = input.stagnation_ms;
    gc.obj_mode = input.obj_mode;
    if (input.output_root != "") gc.output_root = input.output_root;
    if (input.output_subdir != "") gc.output_subdir = input.output_subdir;

    if (!apply_preset(gc)){ cerr << "  Error in presets resolution !" << endl; }

    return gc;
}

GenerationResult generate_counterpoint(const GenerationInput& input) {
    GenerationCase gc = build_gc_from_api_input(input);
    return Generations::run_generation_case(gc, input.save_outputs, input.verbose);
}