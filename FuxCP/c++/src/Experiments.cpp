// =====================================================================
// Experiments.cpp — exploration de coûts dynamiques dans FuxCP
// Auteur : Dorian Genon
// Compile avec : make experiments
// S'arrête avec Ctrl+c, ou si l'arbre de recherche est épuisé, ou si le nombre de solutions atteint target_solution.
// Exporte la dernière solution trouvée en MIDI dans results/experiment.mid
//
// aide de chatGPT.com pour réfléchir au moyen de stopper et exporter la solution lorsuqe bloqué pour la solution suivante
// =====================================================================

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include <signal.h>
#include <atomic>

#include "../headers/Utilities.hpp"
#include "../headers/Midi.hpp"
#include "../headers/CounterpointProblems/CounterpointUtils.hpp"
#include "../headers/CounterpointProblems/CounterpointProblem.hpp"
#include "../headers/CounterpointProblems/TwoVoiceCounterpoint.hpp"
#include "../headers/CostModel.hpp"

// =========================================================
// Gestion de l'interruption avec Ctrl+c
// =========================================================

using namespace Gecode;
using namespace std;

static std::atomic<bool> interrupted(false);

static void signal_handler(int) {
    interrupted.store(true);
}

class InterruptStop : public Gecode::Search::Stop {
public:
    virtual bool stop(const Gecode::Search::Statistics&, const Gecode::Search::Options&) {
        return interrupted.load();
    }
};

int main() {

    // =========================================================
    // 1. CANTUS FIRMUS
    // Séquence de notes MIDI représentant la voix principale. Les contrepoints seront générés à partir de cette séquence.
    // =========================================================
    vector<int> cf = {60, 62, 65, 64, 67, 65, 64, 62, 60};
    int size = cf.size();
    int nMeasures = size - 1; // nombre d'intervalles entre mesures

    // =========================================================
    // 2. ESPÈCES & VOIX
    // spList : liste des espèces pour chaque voix
    // v_type : hauteur de chaque voix par rapport au cf
    //
    // Exemples :
    // 2 voix : spList = {THIRD_SPECIES},                v_type = {2}
    // 3 voix : spList = {THIRD_SPECIES, FIRST_SPECIES},  v_type = {2, 1}
    // 4 voix : spList = {THIRD_SPECIES, FIRST_SPECIES, SECOND_SPECIES}, v_type = {2, 1, -1}
    // =========================================================
    vector<Species> spList = {FOURTH_SPECIES};
    vector<int> v_type     = {2};

    // calcul du nombre d'intervalles mélodiques par voix selon l'espèce
    int nIntervals = 0;
    switch (spList[0]) {
        case FIRST_SPECIES:  nIntervals = size - 1;        break;
        case SECOND_SPECIES: nIntervals = 2 * (size - 1);  break;
        case THIRD_SPECIES:  nIntervals = 4 * (size - 1);  break;
        case FOURTH_SPECIES: nIntervals = 2 * (size - 1);  break;
        case FIFTH_SPECIES:  nIntervals = 4 * (size - 1);  break;
        default:             nIntervals = size - 1;         break;
    }

    // =========================================================
    // 3. MODÈLE DE COÛTS (CostModel)
    //    Le CostModel permet de définir comment chaque coût évolue
    //    au fil de la pièce et par voix, via des "groupes" de coûts.
    
    //    Chaque groupe associe :
    //      - une liste de noms coûts à grouper (indices CostIndex, voir CostModel.hpp)
    //      - une fonction f(s) qui calcule les coûts en fonction d'un
    //        paramètre s ∈ [0,1]
    //      - une shape positionnelle : vecteur de valeurs s,
    //        définissant l'évolution de s au fil de la pièce.
    //        Peut être définie par voix (shapePerVoice)
    //        ou globalement (defaultShape).
    //
    //    Tout coût non groupé utilise sa valeur par défaut de façon constante (defaultCosts) (voir CostModel.hpp)

    //    Fonctions prédéfinies (voir CostModel.hpp et CostModel.cpp) :
    //      steps1(s)     : coûts mélodiques — s=0 favorise conjoints, s=1 favorise sauts
    //      steps2(s)     : coûts mélodiques alternatifs -- s = 0 favorise les consonances parfaites, s=1 favorise les dissonances
    //                                                      consonances imparfaites favorisées dans les deux cas            
    //      harmo(s)      : fifth + octave
    //      perf_cons(s)  : succ + direct
    //      triad_group(s): triad + triad3
    //
    //    Shapes disponibles (voir CostModel.hpp) :
    //      build_constant_zero_shape(n) : s=0 partout
    //      build_constant_one_shape(n)  : s=1 partout
    //      build_linear_shape(n)        : s croît de 0 à 1
    //      build_linear_shape_desc(n)   : s décroît de 1 à 0
    //      build_inverted_v_shape(n)    : s monte puis descend (∧)
    //      build_v_shape(n)             : s descend puis monte (∨)
    //      build_M_shape(n)             : s en forme de M
    // =========================================================
    CostModel costModel;

    // Groupe 1 : fifth + octave en V inversé pour valeur de s
    CostGroup harmonicGroup;
    harmonicGroup.costIndices = {COST_FIFTH, COST_OCTAVE};
    harmonicGroup.fn          = harmo;
    harmonicGroup.defaultShape = build_inverted_v_shape(nMeasures);
    //costModel.addGroup(harmonicGroup); // décommenter pour activer

    // Groupe 2 : succ + direct constants à s=1 
    CostGroup perfConsGroup;
    perfConsGroup.costIndices = {COST_SUCC, COST_DIRECT};
    perfConsGroup.fn          = perf_cons;
    perfConsGroup.defaultShape = build_constant_one_shape(nMeasures);
    //costModel.addGroup(perfConsGroup); // décommenter pour activer

    // Groupe 3 : triad + triad3 linéaire pour valeur de s
    CostGroup triadGrp;
    triadGrp.costIndices = {COST_TRIAD, COST_TRIAD3};
    triadGrp.fn          = triad_group;
    triadGrp.defaultShape = build_linear_shape(nMeasures);
    //costModel.addGroup(triadGrp); // décommenter pour activer

    // Groupe 4 : mélodiques constants à s=0
    CostGroup melodicGroup;
    melodicGroup.costIndices = {COST_MELODIC};
    melodicGroup.fn          = steps1;
    melodicGroup.shapePerVoice = {
        build_constant_zero_shape(nIntervals), 
        // build_v_shape(nIntervals),         
    };
    costModel.addGroup(melodicGroup);

    // =========================================================
    // 4. AFFICHAGE DU MODÈLE DE COÛTS
    // =========================================================

    // Affichage par voix
    cout << "=== Configuration des coûts ===" << endl;
    for (int v = 0; v < (int)spList.size(); ++v) {
        cout << "Voix " << v << " :" << endl;
        for (int c = 0; c < COST_COUNT; ++c) {
            cout << "  coût " << c << " : { ";
            for (int i = 0; i < nMeasures; ++i)
                cout << costModel.getCostAt(c, i, v) << " ";
            cout << "}" << endl;
        }
    }

    // =========================================================
    // 5. VECTEUR D'IMPORTANCE
    // Ordre de priorité des coûts dans l'optimisation choisie
    // 1 = plus important, 14 = moins important
    // Valeurs par défaut définies dans CostModel.hpp
    // =========================================================
    ImportanceVector iv;
    vector<int> importance = iv.toVector();

    // =========================================================
    // 6. PARAMÈTRES DE RECHERCHE
    // =========================================================

    int borrowMode = 0; // 0 = pas d'emprunt, 1 = emprunt autorisé

    ObjectiveMode objMode = OBJECTIVE_LEX; // optimisation lexicographique (par défaut)
    // ObjectiveMode objMode = OBJECTIVE_MINMAX; // optimisation minmax (minimise le pire coût pondéré)
    // ObjectiveMode objMode = OBJECTIVE_WEIGHTED; // optimisation de la somme pondérée 

    
    int    target_solution = 100000; // nombre de solutions à trouver avant d'arrêter la recherche
    string midi_output     = "../../results/experiment.mid"; // chemin du fichier MIDI exporté à la fin de la recherche

    /// =========================================================
    // 7. CRÉATION DU PROBLÈME
    // Active toutes les contraintes et crée le problème à partir du CostModel et des autres paramètres définis
    // =========================================================
    fill(activeConstraints.begin(), activeConstraints.end(), true);

    /**CounterpointProblem* problem = create_problem(
        cf, spList, v_type,
        costModel, 
        importance, borrowMode, objMode
    );**/

    // Remplace temporairement create_problem avec CostModel par l'ancienne API
    vector<int> m_costs = {4, 1, 2, 576, 2, 3, 3, 4};
    vector<int> g_costs = {4, 1, 1, 4, 1, 2, 8, 8};
    vector<int> s_costs = {8, 4, 0, 2, 1, 8, 50};
    vector<int> imp = iv.toVector();

    CounterpointProblem* problem = create_problem(
        cf, spList, v_type,
        m_costs, g_costs, s_costs,
        imp, borrowMode, objMode
    );

    // =========================================================
    // 8. RECHERCHE par Branch and Bound, solutions trouvées croissantes en coût global
    // S'arrête si le nombre de solutions atteint target_solution, si l'utilisateur interrompt, ou si plus de solutions
    // =========================================================
    signal(SIGINT, signal_handler);

    Gecode::Search::Options opts;
    opts.threads = 1;

    InterruptStop stop;
    opts.stop = &stop;

    Search::Base<CounterpointProblem>* e =
        new BAB<CounterpointProblem>(problem, opts);
    int nb_sol = 0;
    vector<int> best_solution;

    ofstream file_time("../../results/graph_time.csv");
    ofstream file_cost("../../results/graph_cost.csv");
    file_time << "solution,time_s\n";
    file_cost << "solution,cost\n";
    auto start = chrono::high_resolution_clock::now();

    cout << "\nCF : ";
    for (int n : cf) cout << n << " ";
    cout << "\nRecherche en cours..." << endl << endl;

    string best_solution_text;

    while (!interrupted) {
        CounterpointProblem* pb = get_next_solution_space(e);
        if (pb == nullptr) break; // si plus de solutions, on arrête la recherche

        nb_sol++;

        auto now = chrono::high_resolution_clock::now();
        double elapsed = chrono::duration<double>(now - start).count();
        double total_cost = pb->getCost();

        file_time << nb_sol << "," << elapsed << endl;
        file_cost << nb_sol << "," << total_cost << endl;

        cout << "[Sol " << nb_sol << "] t=" << elapsed
             << " s | coût=" << total_cost
             << " | détail : " << intVarArgs_to_string(pb->cost()) << endl;
        
        // Sauvegarde de la solution trouvée
        best_solution.clear();
        for (int i = 0; i < pb->getSize(); ++i)
            best_solution.push_back(pb->getSolutionArray()[i].val());

        best_solution_text = pb->to_string();

        if (nb_sol == target_solution) {
            delete pb;
            break;
        }

        delete pb;
    }
    if (interrupted){
        cout << "\nRecherche interrompue — export de la dernière solution." << endl;
    }
    else{
        cout << "\nRecherche terminée — toutes les solutions optimales trouvées." << endl;
    }

    file_time.close();
    file_cost.close();

    // =========================================================
    // 9. EXPORT MIDI de la dernière solution trouvée
    // =========================================================
    if (!best_solution.empty()) {
        cout << "\n--- Dernière solution trouvée ---" << endl;
        cout << "Solution #" << nb_sol << endl;
        cout << best_solution_text << endl;

        cout << "Notes : ";
        for (int n : best_solution) cout << n << " ";
        cout << endl;

        vector<pair<vector<int>, Species>> voices_midi;
        int offset = 0;
        for (int v = 0; v < (int)spList.size(); ++v) {
            int sz = branchingNotesSize(spList[v], cf.size());
            vector<int> voice_notes(best_solution.begin() + offset,
                                    best_solution.begin() + offset + sz);
            voices_midi.push_back({voice_notes, spList[v]});
            offset += sz;
        }
        saveMidiExperiment(midi_output, cf, voices_midi);
        cout << "MIDI écrit : " << midi_output << endl;
    } else {
        cout << "Aucune solution trouvée." << endl;
    }

    cout << "\nTotal solutions trouvées : " << nb_sol << endl;
    delete e;
    return 0;
}