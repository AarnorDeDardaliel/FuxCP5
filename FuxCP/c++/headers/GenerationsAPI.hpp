#include "Generations.hpp"

// For external use
struct GenerationInput {
    vector<int> cf_notes; // The cantus firmus notes

    vector<Species> species;
    vector<int> v_type;

    string preset_name = "";
    int timeout_ms = -1;
    int stagnation_ms = -1;
    ObjectiveMode obj_mode = OBJECTIVE_PONDERED;

    string output_root = "";
    string output_subdir = "";
    bool save_outputs = true;
    bool verbose = false;
};

GenerationResult generate_counterpoint(const GenerationInput& input);