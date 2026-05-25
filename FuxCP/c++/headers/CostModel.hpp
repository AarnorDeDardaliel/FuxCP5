// =====================================================================
// CostModel.hpp — Paramètres de coûts pour FuxCP
// Auteur : Dorian Genon
// Ce fichier a pour objectif de mettre en place tous les paramètres de coûts,
// ainsi que les différentes options de dynamic sliders pour les expériences.
// Tout ceci pouvant être facilement modifié par l'utilisateur, selon les effets 
// qu'il souhaite explorer. 

// aide de claude.ai pour réfléchir à la structure pour généraliser les dynamic sliders à tous les coûts,
// et pas juste aux melodics costs
// =====================================================================

#pragma once
#include <vector>
#include <string>
#include <functional>
#include <cmath>
#include <algorithm>

// =====================================================================
// INDICES DES COÛTS — correspond à l'ordre du vecteur importance
// =====================================================================
enum CostIndex {
    COST_BORROW      = 0,
    COST_FIFTH       = 1,
    COST_OCTAVE      = 2,
    COST_SUCC        = 3,
    COST_VARIETY     = 4,
    COST_TRIAD       = 5,
    COST_DIRECT      = 6,
    COST_MOTION      = 7,
    COST_PENULT      = 8,
    COST_CAMBIATA    = 9,
    COST_TRIAD3      = 10,
    COST_M2          = 11,
    COST_SYNCOPATION = 12,
    COST_MELODIC     = 13,
    COST_COUNT        = 14
};


// =====================================================================
// DYNAMIC SLIDERS — difféntes shapes pour décider l'évolution des coûts en fonction
// de la position.
// Ces fonctions donnent à chaque position une valeur de s entre 0 et 1, cette valeur
// étant ensuite utilisée pour calculer les coûts.
// =====================================================================
std::vector<double> build_constant_zero_shape(int n); // s=0 partout
std::vector<double> build_constant_one_shape(int n); // s=1 partout
std::vector<double> build_linear_shape(int n); // s croît linéairement de 0 à 1
std::vector<double> build_linear_shape_desc(int n); // s décroît linéairement de 1 à 0
std::vector<double> build_inverted_v_shape(int n); // s fait 0->1->0 en formant un ∧
std::vector<double> build_v_shape(int n); // s fait 1->0->1 en formant un ∨
std::vector<double> build_M_shape(int n); // s fait 0->1->0->1->0 en formant un M


// =====================================================================
// TYPE DE FONCTION DE GROUPE : Prend s in [0,1], retourne un vecteur de coûts (un par coût du groupe)
// =====================================================================
using CostGroupFn = std::function<std::vector<int>(double)>;


// =====================================================================
// COST GROUP — groupe de coûts variant selon un même slider s
//
// Un groupe associe :
//   - une liste de coûts (indices CostIndex)
//   - une fonction f(s) -> {cout1, cout2, ...}
//   - une shape positionnelle : s varie selon la position dans la pièce
//
// Exemple : grouper fifth et octave avec harmo(s) et une shape en V inversé
//   group.costIndices = {COST_FIFTH, COST_OCTAVE}
//   group.fn          = harmo
//   group.shape       = build_inverted_v_melodic_shape(nbr_measures)
// =====================================================================
struct CostGroup {
    std::vector<int> costIndices;  // indices des coûts dans ce groupe
    CostGroupFn      fn;           // fonction qui en recevant s en entrée associe des coûts à ce groupe
    std::vector<std::vector<double>> shapePerVoice; // shape positionnelle par voix
    std::vector<double> defaultShape;     // utilisée si voix non définie

    // Retourne la shape à utiliser pour une voix donnée (shape spécifique ou defaultShape)
    const std::vector<double>& getShapeForVoice(int voiceIndex) const {
        if (voiceIndex < (int)shapePerVoice.size() &&
            !shapePerVoice[voiceIndex].empty())
            return shapePerVoice[voiceIndex];
        return defaultShape;
    }

    // Retourne les coûts à la position pos
    std::vector<int> getCostsAt(int pos, int voiceIndex = 0) const {
        const auto& shape = getShapeForVoice(voiceIndex);
        double s = (pos >= 0 && pos < (int)shape.size()) ? shape[pos] : 1.0;
        return fn(s);
    }

    // Retourne le coût du coût costIndex à la position pos
    // Retourne -1 si costIndex n'est pas dans ce groupe
    int getCostAt(int costIndex, int pos, int voiceIndex = 0) const {
        for (int i = 0; i < (int)costIndices.size(); ++i) {
            if (costIndices[i] == costIndex) {
                auto costs = getCostsAt(pos, voiceIndex);
                return (i < (int)costs.size()) ? costs[i] : -1;
            }
        }
        return -1;
    }
};


// =====================================================================
// COST MODEL — modèle complet de coûts
//
// Contient :
//   - les groupes de coûts (définissent comment les coûts varient)
//   - les valeurs par défaut (utilisées si un coût n'est dans aucun groupe)
//
// Tout coût non groupé utilise sa valeur par défaut comme shape constante.
// =====================================================================
struct CostModel {

    // Groupes de coûts définis par l'utilisateur
    std::vector<CostGroup> groups;

    // Valeurs par défaut pour chaque coût (si non groupé)
    std::vector<int> defaultCosts = {
        4,   // borrow
        1,   // fifth
        1,   // octave
        4,   // succ
        1,   // variety
        2,   // triad
        8,   // direct
        2,   // motion (0=contraire, 1=oblique, 2=parallèle — hardcodé)
        8,   // penult
        4,   // cambiata
        2,   // triad3
        1,   // m2
        8,   // syncopation
        4    // melodic (géré séparément par steps())
    };

    // Ajoute un groupe de coûts au modèle
    void addGroup(CostGroup g) {
        groups.push_back(g);
    }

    // Vérifie si un coût est dans un groupe
    bool isGrouped(int costIndex, int voiceIndex = 0) const {
        for (const auto& g : groups) {
            for (int idx : g.costIndices) {
                if (idx == costIndex) {
                    // Vérifie qu'il y a une shape pour cette voix
                    if (!g.getShapeForVoice(voiceIndex).empty())
                        return true;
                }
            }
        }
        return false;
    }

    // Retourne le coût à la position pos pour une voix donnée
    // Utilise le groupe si défini, sinon la valeur par défaut
    int getCostAt(int costIndex, int pos, int voiceIndex = 0) const {
        // Priorité 1 : groupe avec shape explicite pour cette voix
        for (const auto& g : groups) {
            if (voiceIndex < (int)g.shapePerVoice.size() &&
                !g.shapePerVoice[voiceIndex].empty()) {
                int v = g.getCostAt(costIndex, pos, voiceIndex);
                if (v >= 0) return v;
            }
        }
        // Priorité 2 : groupe avec defaultShape
        for (const auto& g : groups) {
            if (g.shapePerVoice.empty() || voiceIndex >= (int)g.shapePerVoice.size() ||
                g.shapePerVoice[voiceIndex].empty()) {
                int v = g.getCostAt(costIndex, pos, voiceIndex);
                if (v >= 0) return v;
            }
        }
        // Priorité 3 : valeur par défaut
        if (costIndex >= 0 && costIndex < COST_COUNT)
            return defaultCosts[costIndex];
        return 0;
    }

    // retourne la valuer de s à la position pos pour une voix donnée
    double getShapeAt(int costIndex, int pos, int voiceIndex = 0) const {
        for (const auto& g : groups) {
            for (int idx : g.costIndices) {
                if (idx == costIndex) {
                    const auto& shape = g.getShapeForVoice(voiceIndex);
                    if (!shape.empty() && pos < (int)shape.size())
                        return shape[pos];
                }
            }
        }
        return 1.0; // valeur par défaut
    }

    // Retourne le vecteur complet des coûts d'un groupe à la position pos
    std::vector<int> getGroupCostsAt(int costIndex, int pos, int voiceIndex = 0) const {
        // Priorité 1 : shape explicite pour cette voix
        for (const auto& g : groups) {
            if (voiceIndex < (int)g.shapePerVoice.size() &&
                !g.shapePerVoice[voiceIndex].empty()) {
                for (int idx : g.costIndices) {
                    if (idx == costIndex)
                        return g.getCostsAt(pos, voiceIndex);
                }
            }
        }
        // Priorité 2 : defaultShape
        for (const auto& g : groups) {
            if (g.shapePerVoice.empty() || voiceIndex >= (int)g.shapePerVoice.size() ||
                g.shapePerVoice[voiceIndex].empty()) {
                for (int idx : g.costIndices) {
                    if (idx == costIndex)
                        return g.getCostsAt(pos, voiceIndex);
                }
            }
        }
        return {};
    }

    // Retourne le profil complet d'un coût sur nPositions
    std::vector<int> getProfile(int costIndex, int nPositions) const {
        std::vector<int> profile(nPositions);
        for (int i = 0; i < nPositions; ++i)
            profile[i] = getCostAt(costIndex, i);
        return profile;
    }

    // Retourne le vecteur g_costs à la position pos
    // (borrow, fifth, octave, succ, variety, triad, direct, penult)
    std::vector<int> getGeneralCostsAt(int pos, int voiceIndex = 0) const {
        return {
            getCostAt(COST_BORROW,  pos, voiceIndex),
            getCostAt(COST_FIFTH,   pos, voiceIndex),
            getCostAt(COST_OCTAVE,  pos, voiceIndex),
            getCostAt(COST_SUCC,    pos, voiceIndex),
            getCostAt(COST_VARIETY, pos, voiceIndex),
            getCostAt(COST_TRIAD,   pos, voiceIndex),
            getCostAt(COST_DIRECT,  pos, voiceIndex),
            0 // non positionnel pour l'instant
        };
    }

    // Retourne le vecteur s_costs à la position pos
    // (penultSixth, cambiata, mSkip, triad3, m2, syncopation, prefSlider)
    std::vector<int> getSpecificCostsAt(int pos, int voiceIndex = 0) const {
        return {
            getCostAt(COST_PENULT,      pos, voiceIndex),
            getCostAt(COST_CAMBIATA,    pos, voiceIndex),
            0,  // mSkipCost — non positionnel pour l'instant
            getCostAt(COST_TRIAD3,      pos, voiceIndex),
            getCostAt(COST_M2,          pos, voiceIndex),
            getCostAt(COST_SYNCOPATION, pos, voiceIndex),
            50  // prefSlider — non positionnel pour l'instant
        };
    }
};



// =====================================================================
// IMPORTANCE VECTOR — ordre de priorité des coûts (1 = plus important)
// Ordre : borrow, fifth, octave, succ, variety, triad, direct, motion,
//         penult, cambiata, triad3, m2, syncopation, melodic
// =====================================================================
struct ImportanceVector {
    int borrow      = 14;
    int fifth       = 6;
    int octave      = 5;
    int succ        = 2;
    int variety     = 9;
    int triad       = 3;
    int direct      = 8;
    int motion      = 10;
    int penult      = 12;
    int cambiata    = 11;
    int triad3      = 4;
    int m2          = 13;
    int syncopation = 1;
    int melodic     = 7;

    std::vector<int> toVector() const {
        return {borrow, fifth, octave, succ, variety, triad, direct, motion,
                penult, cambiata, triad3, m2, syncopation, melodic};
    }
};


// =====================================================================
// SHAPE PAR VOIX — shape mélodique différente par voix de contrepoint, plus utilisée, version avant généralisation
// =====================================================================
struct MelodicShapeConfig {
    std::vector<std::vector<double>> shapePerVoice;

    const std::vector<double>& getShape(int voiceIndex) const {
        static const std::vector<double> empty;
        if (voiceIndex < (int)shapePerVoice.size() &&
            !shapePerVoice[voiceIndex].empty())
            return shapePerVoice[voiceIndex];
        return empty;
    }
};

// =====================================================================
// FONCTIONS DE GROUPE PRÉDÉFINIES
// L'utilisateur peut en créer d'autres avec la même signature :
// std::vector<int>(double s)
// =====================================================================
std::vector<int> steps1(double s);       // coûts mélodiques
std::vector<int> steps2(double s);       // coûts mélodiques
std::vector<int> harmo(double s);       // fifth + octave
std::vector<int> perf_cons(double s);   // succ + direct
std::vector<int> triad_group(double s); // triad + triad3