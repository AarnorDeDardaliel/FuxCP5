// =====================================================================
// GenerateCounterpoint.cpp — point d'entrée unifié de génération.
// Format aligné sur Generations.cpp : utilise GenerationCase + bench BAB
// séparé. Toutes les données autrefois hardcodées (registre CF, presets de
// paramètres, campagnes) sont chargées depuis FuxCP/config/*.csv.
//
// Usage CLI :
//   ./GenerateCounterpoint <nb_voix> <especes...> [options]
//
// Modes spéciaux :
//   ./GenerateCounterpoint --campaign <id>   (lance toutes les lignes)
//   ./GenerateCounterpoint --list-cf
//   ./GenerateCounterpoint --list-presets
//   ./GenerateCounterpoint --list-campaigns
//
// Options :
//   --cf-notes CF_NOTES (no default, format is : 60,62,65,64,67,65,64,62,60 ),
//   -c CF_ID            (défaut : 1)
//   --preset NAME       (défaut : default)
//   -t timeout_ms       (défaut : 300000)
//   -s stagnation_ms    (défaut : 120000)
//   -o output_root      (défaut : ../results)
//   --subdir NAME       (sous-dossier sous output_root)
//   -m lex|total|mixed  (défaut : lex)
//   -v vt1,vt2,...      (v_types des voix de contrepoint, séparés par ',')
// =====================================================================

#include "../headers/Generations.hpp"

using namespace std;
using namespace std::chrono;
using namespace Gecode;

class StagnationStop : public Search::Stop {
    Search::TimeStop maxStop;
    steady_clock::time_point lastImprovement;
    int stagnation_ms;
    bool stagnation_triggered = false;
    bool max_triggered = false;
    
public:
    StagnationStop(int max_ms, int stag_ms)
        : maxStop(max_ms), stagnation_ms(stag_ms),
          lastImprovement(steady_clock::now()) {}

    void notifyImprovement() { lastImprovement = steady_clock::now(); }

    bool stop(const Search::Statistics& s, const Search::Options& o) {
        if (maxStop.stop(s, o)) { max_triggered = true; return true; }
        if (stagnation_ms > 0) {
            auto el = duration_cast<milliseconds>(steady_clock::now() - lastImprovement).count();
            if (el > stagnation_ms) { stagnation_triggered = true; return true; }
        }
        return false;
    }
    string stopReason() const {
        if (stagnation_triggered) return "STAGNATION";
        if (max_triggered)        return "TIMEOUT_MAX";
        return "EXHAUSTIVE";
    }
};

// =============================================================
// Helpers
// =============================================================

static const int CHECKPOINT_INTERVAL = 1000;

static string species_tag_for(const GenerationCase& gc) {
    string tag = to_string(gc.n_voices) + "voices_1";  // CF en première position
    for (int s : gc.spList) tag += "_" + to_string(s+1);
    return tag;
}

static void mkdir_p(const string& path) {
    string cmd = "mkdir -p '" + path + "'";
    int r = system(cmd.c_str());
    (void)r;
}


// =============================================================
// Construction d'un GenerationCase à partir des CSV + arguments
// =============================================================

bool resolve_cf(GenerationCase& gc, bool verbose) {
    string detected_scale_name;
    if (gc.use_preset_cf){
        string config_dir = ConfigLoader::find_config_dir();
        auto cfs = ConfigLoader::load_cantus_firmus(config_dir);

        auto it = cfs.find(gc.cf_id);
        if (it == cfs.end()) {
            cerr << "CF inconnu : " << gc.cf_id << endl;
            return false;
        }
        gc.cf = it->second.notes;
        if (gc.cf.empty()) {
            cerr << "CF id=" << gc.cf_id << " : aucune note dans cantus_firmus.csv" << endl;
            return false;
        }
        gc.cf_name  = it->second.name;
        gc.cf_scale = it->second.scale;
        detected_scale_name = detect_scale_name_for_cf(gc.cf);
    }
    else { // Direct cf
        if (gc.cf.empty()) {
            cerr << "Direct CF provided but empty." << endl;
            return false;
        }

        detected_scale_name = detect_scale_name_for_cf(gc.cf);
        if (gc.cf_scale.empty()) { gc.cf_scale = detected_scale_name; }
        if (gc.cf_name.empty()){ gc.cf_name  = "direct_cf_" + gc.cf_scale; }
    }

    if (verbose){
        cout << "Cantus firmus -> gamme détectée : "
        << detected_scale_name << " (tonique = "
        << noteNames[((gc.cf[0] % 12) + 12) % 12] << ")"
        << " -> gamme spécifiée : " << gc.cf_scale << endl;
    }
    
    return true;
}

bool apply_preset(GenerationCase& gc) {
    string config_dir = ConfigLoader::find_config_dir();
    auto presets = ConfigLoader::load_presets(config_dir);

    auto it = presets.find(gc.preset_name);
    if (it == presets.end()) {
        cerr << "Preset inconnu : " << gc.preset_name << endl;
        return false;
    }
    const auto& p = it->second;
    gc.melodic_params  = p.melodic_params;
    gc.general_params  = p.general_params;
    gc.specific_params = p.specific_params;
    gc.importance      = p.importance;
    gc.borrow_mode     = p.borrow_mode;
    return true;
}

// =============================================================
// Logging (txt + csv) — une seule génération
// =============================================================

static void write_txt_report(const string& path, const GenerationCase& gc,
                             const GenerationResult& gr) {
    ofstream r(path);
    r << "========================================\n"
      << "  RÉSULTATS : " << species_tag_for(gc) << "\n"
      << "========================================\n\n";
    r << "--- Configuration ---\n";
    if (gc.use_preset_cf) r << "CF                : " << gc.cf_id << " (" << gc.cf_name << ")\n";
    else                  r << "CF                : " << gc.cf_name << "\n";
    r  << "Gamme indicative  : " << gc.cf_scale << "\n"
      << "Preset            : " << gc.preset_name << "\n"
      << "Borrow mode       : " << gc.borrow_mode << "\n"
      << "Mode d'objectif   : " << obj_mode_label(gc.obj_mode) << "\n"
      << "Nombre de voix    : " << gc.n_voices << "\n"
      << "Espèces (incl CF) : 1";
    for (int s : gc.spList) r << " " << s;
    r << "\nv_type            :";
    for (int v : gc.v_type) r << " " << v;
    r << "\nTimeout max (ms)  : " << gc.timeout_ms << "\n"
      << "Stagnation (ms)   : " << gc.stagnation_ms << "\n"
      << "Cantus firmus     :";
    for (int n : gc.cf) r << " " << n;
    r << "\n                   ";
    for (int n : gc.cf) r << " " << midi_to_french(n);
    r << "\n\n";

    r << "--- Métriques de recherche ---\n"
      << "Temps total (ms)            : " << fixed << setprecision(1) << gr.ms_total << "\n"
      << "Temps 1ère solution (ms)    : " << fixed << setprecision(1) << gr.ms_first_solution << "\n"
      << "Temps dernière amélio. (ms) : " << fixed << setprecision(1) << gr.ms_last_improve << "\n"
      << "Solutions trouvées          : " << gr.nb_solutions << "\n"
      << "Améliorations de coût       : " << gr.nb_improvements << "\n"
      << cost_label(gc.obj_mode) << "           : " << gr.best_cost << "\n"
      << "Terminaison                 : " << gr.termination;
    if (gr.termination == "EXHAUSTIVE") r << " (optimal)";
    r << "\n\n";

    r << "--- Statistiques Gecode ---\n"
      << "Nœuds      : " << gr.stats.node      << "\n"
      << "Échecs     : " << gr.stats.fail      << "\n"
      << "Restarts   : " << gr.stats.restart   << "\n"
      << "Propagat.  : " << gr.stats.propagate << "\n"
      << "Profondeur : " << gr.stats.depth     << "\n\n";

    if (!gr.improvements.empty()) {
        r << "--- Améliorations ---\n"
          << setw(8) << "Sol#" << setw(14) << "Temps (ms)" << setw(12) << "Coût" << "\n";
        for (auto& t : gr.improvements) {
            r << setw(8) << get<0>(t)
              << setw(14) << fixed << setprecision(1) << get<1>(t)
              << setw(12) << fixed << setprecision(1) << get<2>(t) << "\n";
        }
        r << "\n";
    }
    if (!gr.checkpoints.empty()) {
        r << "--- Checkpoints (" << CHECKPOINT_INTERVAL << " sol) ---\n"
          << setw(8) << "Sol#" << setw(14) << "Temps (ms)" << setw(14) << "Best cost" << "\n";
        for (auto& t : gr.checkpoints) {
            r << setw(8) << get<0>(t)
              << setw(14) << fixed << setprecision(1) << get<1>(t)
              << setw(14) << fixed << setprecision(1) << get<2>(t) << "\n";
        }
        r << "\n";
    }
    if (!gr.solutions_log.empty()) {
        r << "--- \u00c9volution du vecteur de co\u00fbts (toutes les solutions BAB) ---\n"
          << setw(8) << "Sol#" << setw(14) << "Temps (ms)"
          << setw(12) << "Cost" << "  Vecteur lex\n";
        for (auto& t : gr.solutions_log) {
            r << setw(8) << get<0>(t)
              << setw(14) << fixed << setprecision(1) << get<1>(t)
              << setw(12) << fixed << setprecision(1) << get<2>(t)
              << "  " << get<3>(t) << "\n";
        }
        r << "\n";
    }
    if (!gr.full_solution.empty()) {
        r << "--- Solution (notes MIDI) ---\n";
        for (int n : gr.full_solution) r << n << " ";
        r << "\n";
    }
    r.close();
}

static void write_csv_report(const string& path, const GenerationCase& gc,
                             const GenerationResult& gr) {
    ofstream c(path);
    
    c << "cf_id,cf_name,preset,borrow_mode,nb_voix,especes,v_type,"
         "timeout_ms,stagnation_ms,temps_total_ms,temps_premiere_solution_ms,"
         "temps_derniere_amelioration_ms,nb_solutions,nb_ameliorations,cout_final,"
         "terminaison,noeuds,echecs,redemarrages,propagations,profondeur_max\n";
    if (gc.use_preset_cf) c << gc.cf_id ;
    else                  c << "/" << "\n";
    c << "," << gc.cf_name << "," << gc.preset_name << ","
      << gc.borrow_mode << "," << gc.n_voices << ",";
    c << "1";
    for (int s : gc.spList) c << "-" << s;
    c << ",";
    for (size_t i = 0; i < gc.v_type.size(); ++i) {
        if (i > 0) c << "-";
        c << gc.v_type[i];
    }
    c << "," << gc.timeout_ms << "," << gc.stagnation_ms
      << "," << fixed << setprecision(1) << gr.ms_total
      << "," << fixed << setprecision(1) << gr.ms_first_solution
      << "," << fixed << setprecision(1) << gr.ms_last_improve
      << "," << gr.nb_solutions
      << "," << gr.nb_improvements
      << "," << fixed << setprecision(1) << gr.best_cost
      << "," << gr.termination
      << "," << gr.stats.node
      << "," << gr.stats.fail
      << "," << gr.stats.restart
      << "," << gr.stats.propagate
      << "," << gr.stats.depth << "\n";

    if (!gr.improvements.empty()) {
        c << "\n# Améliorations\nsolution_num,temps_ms,cout\n";
        for (auto& t : gr.improvements)
            c << get<0>(t) << "," << fixed << setprecision(1) << get<1>(t)
              << "," << fixed << setprecision(1) << get<2>(t) << "\n";
    }
    if (!gr.checkpoints.empty()) {
        c << "\n# Checkpoints\nsolution_num,temps_ms,meilleur_cout\n";
        for (auto& t : gr.checkpoints)
            c << get<0>(t) << "," << fixed << setprecision(1) << get<1>(t)
              << "," << fixed << setprecision(1) << get<2>(t) << "\n";
    }
    if (!gr.solutions_log.empty()) {
        c << "\n# Evolution couts\nsolution_num,temps_ms,cout_scalaire,vecteur_lex\n";
        for (auto& t : gr.solutions_log) {
            string lex = get<3>(t);
            // CSV-safe : remplacer virgules par points-virgules dans le vecteur lex
            for (char& ch : lex) if (ch == ',') ch = ';';
            c << get<0>(t) << "," << fixed << setprecision(1) << get<1>(t)
              << "," << fixed << setprecision(1) << get<2>(t)
              << ",\"" << lex << "\"\n";
        }
    }
    c.close();
}

static void write_error_txt(const string& path, const GenerationCase& gc,
                            const GenerationResult& gr) {
    ofstream e(path);
    e << "========================================\n"
      << "  AUCUNE SOLUTION : " << species_tag_for(gc) << "\n"
      << "========================================\n\n";
    if (gc.use_preset_cf) e << "CF                : " << gc.cf_id << " (" << gc.cf_name << ")\n";
    else                  e << "CF                : " << gc.cf_name << "\n";
    e  << "Preset         : " << gc.preset_name << "\n"
      << "Nb voix        : " << gc.n_voices << "\n"
      << "Espèces        : 1";
    for (int s : gc.spList) e << " " << s;
    e << "\nTimeout (ms)   : " << gc.timeout_ms << "\n"
      << "Stagnation (ms): " << gc.stagnation_ms << "\n"
      << "Terminaison    : " << gr.termination << "\n"
      << "Temps écoulé   : " << fixed << setprecision(1) << gr.ms_total << " ms\n"
      << "Nœuds          : " << gr.stats.node << "\n"
      << "Échecs         : " << gr.stats.fail << "\n";
    e.close();
}

// =============================================================
// Helper : retourne true si l'itération n doit afficher du logging progressif
// Affiche à : 1, 5, 10, 20, 30, 40, 50, 100, 200, 500, 1000, 1500, ...
// =============================================================
static bool should_log_iteration(int n) {
    if (n <= 50) {
        return n == 1 || n == 5 || n == 10 || n == 20 || n == 30 || n == 40 || n == 50;
    }
    if (n <= 500) return n % 100 == 0;
    if (n <= 2000) return n % 500 == 0;
    return n % 1000 == 0;
}

// =============================================================
// Bench BAB (équivalent generation_BAB_bench de Generations.cpp)
// =============================================================

static GenerationResult run_bench(CounterpointProblem* problem, GenerationCase& gc, bool verbose=true) {
    GenerationResult result;
    result.resolved_cf = gc.cf;
    result.resolved_cf_name = gc.cf_name;
    result.resolved_cf_scale = gc.cf_scale;

    StagnationStop stopObj(gc.timeout_ms, gc.stagnation_ms);
    Search::Options opt;
    opt.stop = &stopObj;

    auto t0 = steady_clock::now();
    auto ms_since = [&]() {
        return duration<double, milli>(steady_clock::now() - t0).count();
    };

    BAB<CounterpointProblem> e(problem, opt);
    CounterpointProblem* best = nullptr;
    int iteration = 0;

    while (CounterpointProblem* s = e.next()) {
        stopObj.notifyImprovement();
        iteration++;
        result.nb_solutions++;
        double now = ms_since();
        double cost = s->getCost();
        string lex = intVarArgs_to_string(s->cost());

        // Trace live : on voit l'évolution des coûts en suivant le log.
        // En mode lex on affiche le vecteur complet (le plus parlant) ;
        // en mode total/mixed/pond on affiche le scalaire optimisé directement.
        if (verbose) {
            if (gc.obj_mode == OBJECTIVE_LEX) {
                cout << "  [Sol " << result.nb_solutions
                    << " | t=" << fixed << setprecision(0) << now << "ms"
                    << " | somme=" << fixed << setprecision(1) << cost
                    << " | lex=" << lex << "]" << endl;
            } else {
                cout << "  [Sol " << result.nb_solutions
                    << " | t=" << fixed << setprecision(0) << now << "ms"
                    << " | " << cost_label(gc.obj_mode) << "="
                    << fixed << setprecision(1) << cost << "]" << endl;
            }
        }
        
        // Logging progressif des itérations (affiche tous les 1, 5, 10, 20, 50, 100, 500, 1000...)
        if (should_log_iteration(iteration)) {
            // Log even without verbose to check the algorithm is progressing
            cout << "    -> Itération " << iteration << " | t=" 
            << fixed << setprecision(0) << now << "ms | meilleur="
            << fixed << setprecision(1) << result.best_cost << endl;            
        }


        result.solutions_log.emplace_back(result.nb_solutions, now, cost, lex);

        if (result.nb_solutions == 1) result.ms_first_solution = now;

        if (cost < result.best_cost) {
            result.nb_improvements++;
            delete best;
            best = s;
            result.best_cost = cost;
            result.ms_last_improve = now;
            result.improvements.emplace_back(result.nb_solutions, now, cost);
            stopObj.notifyImprovement();
        } else {
            delete s;
        }
        if (result.nb_solutions % CHECKPOINT_INTERVAL == 0) {
            result.checkpoints.emplace_back(result.nb_solutions, now, result.best_cost);
        }
    }

    bool stopped = e.stopped();
    result.termination = stopped ? stopObj.stopReason() : "EXHAUSTIVE";
    result.stats = e.statistics();
    result.ms_total = ms_since();

    if (best) {
        IntVarArray sa = best->getSolutionArray();
        result.full_solution.reserve(sa.size());
        result.success = true;
        for (int i = 0; i < sa.size(); ++i) result.full_solution.push_back(sa[i].val());
        delete best;
    } else { 
        result.success = false; 
    }
    return result;
}

// =============================================================
// Exécution d'un GenerationCase complet (problème + bench + sauvegarde)
// =============================================================

GenerationResult Generations::run_generation_case(GenerationCase& gc, bool save_outputs, bool verbose) {
    string species_tag = species_tag_for(gc);
    string cf_prefix = gc.use_preset_cf ? ("cf" + to_string(gc.cf_id)) : gc.cf_name;
    string default_filenames = cf_prefix + "_" + species_tag;

    string base_dir = gc.output_root;
    if (!gc.output_subdir.empty()) base_dir += "/" + gc.output_subdir;
    base_dir += "/" + to_string(gc.n_voices) + "voices";
    string txt_dir  = base_dir + "/txt";
    string csv_dir  = base_dir + "/csv";
    string midi_dir = base_dir + "/midi";
    mkdir_p(txt_dir); mkdir_p(csv_dir); mkdir_p(midi_dir);

    // Description du CF (créée 1 seule fois)
    if (save_outputs){
        string cf_desc = base_dir + "/cantus_firmus_" + cf_prefix + ".txt";
        if (!ifstream(cf_desc).good()) {
            ofstream f(cf_desc);
            for (size_t i = 0; i < gc.cf.size(); ++i) { if (i) f << " "; f << gc.cf[i]; }
            f << "\n";
            for (size_t i = 0; i < gc.cf.size(); ++i) { if (i) f << " "; f << midi_to_french(gc.cf[i]); }
            f << "\n" << gc.cf_name << " (gamme indicative : " << gc.cf_scale << ")\n";
        }
    } 

    if (verbose){
        if (gc.use_preset_cf) cout << "\n=== Génération : cf" << gc.cf_id << " (" << gc.cf_name << ") | ";
        else                  cout << "\n=== Génération : cf" << gc.cf_name << " | ";
        cout     << species_tag << " | preset=" << gc.preset_name
            << " | obj=" << obj_mode_short(gc.obj_mode)
            << " | timeout=" << gc.timeout_ms << "ms | stagn=" << gc.stagnation_ms << "ms ===" << endl;
    }
    
    // Activation des contraintes musicales (par défaut toutes actives)
    fill(activeConstraints.begin(), activeConstraints.end(), true);

    // Création du problème via create_problem (méthode unifiée, cf Generations.cpp)
    CounterpointProblem* problem = create_problem(
        gc.cf, gc.spList, gc.v_type,
        gc.melodic_params, gc.general_params, gc.specific_params,
        gc.importance, gc.borrow_mode, gc.obj_mode);

    GenerationResult result = run_bench(problem, gc, verbose);
    delete problem;

    if (!result.full_solution.empty()) {
        // Découpage des voix pour MIDI
        vector<pair<vector<int>, Species>> voices;
        int offset = 0;
        for (int v = 0; v < (int)gc.spList.size(); ++v) {
            int sz = branchingNotesSize(gc.spList[v], gc.cf.size());
            vector<int> notes(result.full_solution.begin() + offset,
                              result.full_solution.begin() + offset + sz);
            voices.emplace_back(notes, gc.spList[v]);
            offset += sz;
        }
        result.generated_voices = voices;

        if (save_outputs){
            string midi_path = midi_dir + "/" + default_filenames + ".mid";
            string txt_path = txt_dir + "/" + default_filenames + ".txt";
            string csv_path = csv_dir + "/" + default_filenames + ".csv";

            saveMidiMultiVoice(midi_path, gc.cf, voices);
            write_txt_report(txt_dir + "/" + default_filenames + ".txt", gc, result);
            write_csv_report(csv_dir + "/" + default_filenames + ".csv", gc, result);

            result.midi_path = midi_path;
            result.txt_path = txt_path;
            result.csv_path = csv_path;
        }
    
        if (verbose){
            cout << "  -> Solutions=" << result.nb_solutions
             << " Améliorations=" << result.nb_improvements
             << " " << cost_label(gc.obj_mode) << "=" << result.best_cost
             << " Temps=" << fixed << setprecision(0) << result.ms_total << "ms"
             << " (" << result.termination << ")" << "\n"
             << "  -> Best = " << int_vector_to_string(result.full_solution) << endl;
        }
    } else {

        if (save_outputs){
            string error_path = txt_dir + "/error_" + default_filenames + ".txt";
            write_error_txt(error_path, gc, result);
            result.error_txt_path = error_path;
        }

        if (verbose){
            cout << "  -> AUCUNE SOLUTION (" << result.termination << ", "
             << fixed << setprecision(0) << result.ms_total << " ms)" << endl;
        }
    }

    // Fill GenerationResult
    return result;
}
