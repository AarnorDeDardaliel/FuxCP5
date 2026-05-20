#include "../headers/Generations.hpp"

// =============================================================
// Listing helpers
// =============================================================

static void cmd_list_cf(const map<int, CFEntry>& cfs) {
    cout << "Cantus firmus disponibles :" << endl;
    cout << "  ID  | Nom                  | Gamme indicative   | Description" << endl;
    cout << "  ----+----------------------+--------------------+-----------------------------" << endl;
    for (auto& kv : cfs) {
        cout << "  " << setw(3) << kv.first << " | "
             << setw(20) << left << kv.second.name << " | "
             << setw(18) << left << kv.second.scale << " | "
             << kv.second.description << endl;
    }
}

static void cmd_list_presets(const map<string, PresetEntry>& presets) {
    cout << "Presets disponibles :" << endl;
    for (auto& kv : presets) {
        cout << "  " << setw(15) << left << kv.first
             << " borrow=" << kv.second.borrow_mode
             << "  - " << kv.second.description << endl;
    }
}

static void cmd_list_campaigns(const vector<CampaignRow>& camps) {
    cout << "Campagnes disponibles :" << endl;
    map<string,int> counts;
    map<string,string> first_desc;
    for (auto& c : camps) {
        counts[c.campaign_id]++;
        if (first_desc.find(c.campaign_id) == first_desc.end()) {
            first_desc[c.campaign_id] = c.description;
        }
    }
    for (auto& kv : counts) {
        cout << "  " << setw(20) << left << kv.first
             << " (" << kv.second << " configs) - " << first_desc[kv.first] << endl;
    }
}


// =============================================================
// CLI
// =============================================================

struct CliArgs {
    int    nb_voix       = 0;
    vector<int> sp_input;          // espèces des voix de contrepoint
    vector<int>    cf;
    int    timeout_ms    = 300000; // 5min
    int    stagnation_ms = 120000; // 2min
    string output_root   = "../../results";
    string output_subdir = "";
    ObjectiveMode obj_mode = OBJECTIVE_PONDERED;
    vector<int> v_types_override;

    int cf_id            = 1;
    bool use_preset_cf   = false;
    string preset_name   = "default";

    string campaign_id;
    bool list_cf = false, list_presets = false, list_campaigns = false;
};

static void print_usage() {
    cout << "Usage : ./GenerateCounterpoint <nb_voix> <especes...> [options]\n"
         << "        ./GenerateCounterpoint --campaign <id>\n"
         << "        ./GenerateCounterpoint --list-cf | --list-presets | --list-campaigns\n\n"
         << "Options :\n"
         << "  --cf-notes n1,n2,... raw cantus firmus MIDI notes\n"
         << "  -c CF_ID            id du cantus firmus (cf cantus_firmus.csv)\n"
         << "  --preset NAME       preset de paramètres (cf presets.csv)\n"
         << "  -t timeout_ms       timeout total maximum\n"
         << "  -s stagnation_ms    arrêt si pas d'amélioration depuis N ms (0 = off)\n"
         << "  -o output_root      racine des résultats (défaut ../results)\n"
         << "  --subdir NAME       sous-dossier sous output_root\n"
         << "  -m lex|total|mixed|pond  mode d'objectif\n"
         << "  -v vt1,vt2,...      v_types des voix de contrepoint\n";
}

static bool parse_cli(int argc, char* argv[], CliArgs& a) {
    int i = 1;
    if (i < argc) {
        string s = argv[i];
        if (s == "--list-cf")        { a.list_cf        = true; return true; }
        if (s == "--list-presets")   { a.list_presets   = true; return true; }
        if (s == "--list-campaigns") { a.list_campaigns = true; return true; }
        if (s == "--campaign" && i + 1 < argc) { a.campaign_id = argv[i+1]; return true; }
    }
    if (argc < 3) { print_usage(); return false; }
    a.nb_voix = atoi(argv[1]);
    if (a.nb_voix < 2 || a.nb_voix > 4) {
        cerr << "nb_voix doit être 2, 3 ou 4." << endl;
        return false;
    }
    int n_sp = a.nb_voix - 1;
    if (argc < 2 + n_sp) {
        cerr << "Il faut " << n_sp << " espèces (hors CF)." << endl;
        return false;
    }
    for (int k = 0; k < n_sp; ++k) {
        int sp = atoi(argv[2 + k]);
        if (sp < 1 || sp > 5) { cerr << "Espèce invalide : " << sp << endl; return false; }
        a.sp_input.push_back(sp);
    }
    int j = 2 + n_sp;
    while (j < argc) {
        string s = argv[j];
        auto need = [&](int extra) { return j + extra < argc; };
        if (s == "--cf-notes" && need(1)) {
            string raw = argv[++j];
            a.cf.clear();
            for (auto& part : ConfigLoader::split(raw, ',')) {
                string p = ConfigLoader::trim(part);
                if (!p.empty()) a.cf.push_back(atoi(p.c_str()));
            }
        }
        else if      (s == "-c" && need(1))        { 
            a.cf_id = atoi(argv[++j]); 
            a.use_preset_cf = true;
        }
        else if (s == "--preset" && need(1))  { a.preset_name = argv[++j]; }
        else if (s == "-t" && need(1))        { a.timeout_ms = atoi(argv[++j]); }
        else if (s == "-s" && need(1))        { a.stagnation_ms = atoi(argv[++j]); }
        else if (s == "-o" && need(1))        { a.output_root = argv[++j]; }
        else if (s == "--subdir" && need(1))  { a.output_subdir = argv[++j]; }
        else if (s == "-m" && need(1))        { a.obj_mode = parse_obj_mode(argv[++j]); }
        else if (s == "-v" && need(1)) {
            string v = argv[++j];
            for (auto& part : ConfigLoader::split(v, ',')) {
                string p = ConfigLoader::trim(part);
                if (!p.empty()) a.v_types_override.push_back(atoi(p.c_str()));
            }
        }
        else { cerr << "Argument inconnu : " << s << endl; return false; }
        ++j;
    }
    return true;
}

static GenerationCase build_gc_from_cli(const CliArgs& a) {
    GenerationCase gc;
    gc.cf = a.cf;
    if (a.use_preset_cf){
        gc.cf_id         = a.cf_id;
        gc.use_preset_cf = true;
    }
    gc.n_voices       = a.nb_voix;
    gc.preset_name    = a.preset_name;
    gc.timeout_ms     = a.timeout_ms;
    gc.stagnation_ms  = a.stagnation_ms;
    gc.output_root    = a.output_root;
    gc.output_subdir  = a.output_subdir;
    gc.obj_mode       = a.obj_mode;
    for (int s : a.sp_input) gc.spList.push_back(int_to_species(s));
    if (!a.v_types_override.empty()) gc.v_type = a.v_types_override;
    else gc.v_type = vector<int>(a.nb_voix - 1, 0);
    return gc;
}

static GenerationCase build_gc_from_campaign(const CampaignRow& row, int cf_id, const string& output_root,
                                      ObjectiveMode obj_override) {
    GenerationCase gc;
    gc.cf_id          = cf_id;
    gc.use_preset_cf = true;
    gc.n_voices       = row.n_voices;
    gc.preset_name    = row.preset.empty() ? "default" : row.preset;
    gc.timeout_ms     = row.timeout_ms;
    gc.stagnation_ms  = row.stagnation_ms;
    gc.output_root    = output_root;
    gc.output_subdir  = row.output_subdir;
    gc.obj_mode       = obj_override;
    for (int s : row.species) gc.spList.push_back(int_to_species(s));
    if (!row.v_types.empty()) gc.v_type = row.v_types;
    else gc.v_type = vector<int>(row.n_voices - 1, 0);
    return gc;
}


// =============================================================
// main
// =============================================================

int main(int argc, char* argv[]) {
    string config_dir = ConfigLoader::find_config_dir();
    auto cfs      = ConfigLoader::load_cantus_firmus(config_dir);
    auto presets  = ConfigLoader::load_presets(config_dir);
    auto camps    = ConfigLoader::load_campaigns(config_dir);

    CliArgs a;
    if (!parse_cli(argc, argv, a)) return 1;

    if (a.list_cf)        { cmd_list_cf(cfs); return 0; }
    if (a.list_presets)   { cmd_list_presets(presets); return 0; }
    if (a.list_campaigns) { cmd_list_campaigns(camps); return 0; }

    // === Mode campagne ===
    if (!a.campaign_id.empty()) {
        vector<CampaignRow> selected;
        for (auto& c : camps) if (c.campaign_id == a.campaign_id) selected.push_back(c);
        if (selected.empty()) {
            cerr << "Aucune ligne pour la campagne : " << a.campaign_id << endl;
            return 1;
        }
        cout << "=== Campagne " << a.campaign_id << " : " << selected.size()
             << " configs × CF associés ===" << endl;
        int total = 0, done = 0;
        for (auto& row : selected) total += row.cf_ids.size();
        for (auto& row : selected) {
            for (int cf_id : row.cf_ids) {
                done++;
                cout << "\n[" << done << "/" << total << "] " << row.description << endl;
                GenerationCase gc = build_gc_from_campaign(row, cf_id, a.output_root, a.obj_mode);
                if (!resolve_cf(gc))      { cerr << "  CF non résolu, skip." << endl; continue; }
                if (!apply_preset(gc)){ cerr << "  Preset non résolu, skip." << endl; continue; }
                GenerationResult res = Generations::run_generation_case(gc, true, true);
            }
        }
        cout << "\n=== Campagne terminée (" << done << "/" << total << ") ===" << endl;
        return 0;
    }

    // === Mode unique ===
    GenerationCase gc = build_gc_from_cli(a);
    cout << gc.spList.size() << endl;
    if (!resolve_cf(gc))       return 1;
    if (!apply_preset(gc)) return 1;
    GenerationResult res = Generations::run_generation_case(gc, true, true);
    return res.success ? 0 : 1;
}

