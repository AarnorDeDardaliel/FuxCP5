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
std::vector<double> build_step_shape(int n); // s fait 0->1 avec un escalier au milieu
std::vector<double> build_step_desc_shape(int n); // s fait 1->0 avec un escalier au milieu


// =====================================================================
// TYPE DE FONCTION DE GROUPE : Prend s dans [0,1], retourne un vecteur de coûts (un par coût du groupe)
// =====================================================================
using CostGroupFn = std::function<std::vector<int>(double)>;


// =====================================================================
// FONCTIONS DE GROUPE PRÉDÉFINIES
// L'utilisateur peut en créer d'autres avec la même signature :
// std::vector<int>(double s)
// =====================================================================
std::vector<int> steps1(double s);       // coûts mélodiques
std::vector<int> steps2(double s);       // coûts mélodiques
std::vector<int> harmo(double s);       // fifth + octave


// =====================================================================
// COST GROUP — groupe de coûts variant selon un même slider s
//
// Un groupe associe :
//   - une liste de coûts (indices CostIndex)
//   - une fonction f(s) par défaut -> {cout1, cout2, ...}
//   - une liste de fonctions par voix
//   - une shape positionnelle par défaut : s varie selon la position dans la pièce
//   - une liste de shapes par voix
//
// Exemple : grouper fifth et octave avec harmo(s) et une shape en V inversé
//   group.costIndices = {COST_FIFTH, COST_OCTAVE}
//   group.fn          = harmo
//   group.shape       = build_inverted_v_melodic_shape(nbr_measures)
// =====================================================================
struct CostGroup {
    std::vector<int> costIndices;  // indices des coûts dans ce groupe
    CostGroupFn      fn;           // fonction par défaut
    std::vector<CostGroupFn> fnPerVoice; // fonctions par voix
    std::vector<double> defaultShape;     // shape par défaut
    std::vector<std::vector<double>> shapePerVoice; // shapes positionnelles par voix

    // Retourne la shape à utiliser pour une voix donnée (shape spécifique ou defaultShape)
    const std::vector<double>& getShapeForVoice(int voiceIndex) const {
        if (voiceIndex < (int)shapePerVoice.size() &&
            !shapePerVoice[voiceIndex].empty())
            return shapePerVoice[voiceIndex];
        return defaultShape;
    }

    // Retourne la fonction à utiliser pour une voix donnée
    const CostGroupFn& getFnForVoice(int voiceIndex) const {
        if (voiceIndex < (int)fnPerVoice.size() && fnPerVoice[voiceIndex])
            return fnPerVoice[voiceIndex];
        return fn;
    }

    std::vector<int> getCostsAt(int pos, int voiceIndex = 0) const {
        const auto& shape = getShapeForVoice(voiceIndex);
        double s = (pos >= 0 && pos < (int)shape.size()) ? shape[pos] : 1.0;
        return getFnForVoice(voiceIndex)(s);  // ← utilise la fonction de la voix
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

    // Valeurs par défaut pour chaque coût (si non groupé), attention valeur max du domaine si cout groupé
    std::vector<int> defaultCosts = {
        1,   // borrow
        1,   // fifth
        1,   // octave
        1,   // succ
        1,   // variety
        1,   // triad
        1,   // direct
        1,   // motion
        1,   // penult
        1,   // cambiata
        1,   // triad3
        1,   // m2
        1,   // syncopation
        0    // melodic, géré séparément car comprend plusieurs couts
    };


    // penser au triton, 576 = cout maximal du domaine
    std::vector<int> getMelodicCostsDefault() const {
        return {1, 1, 1, 576, 1, 1, 1, 1};
    }

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
        //  groupe avec shape explicite pour cette voix
        for (const auto& g : groups) {
            if (voiceIndex < (int)g.shapePerVoice.size() &&
                !g.shapePerVoice[voiceIndex].empty()) {
                int v = g.getCostAt(costIndex, pos, voiceIndex);
                if (v >= 0) return v;
            }
        }
        // groupe avec defaultShape
        for (const auto& g : groups) {
            if (g.shapePerVoice.empty() || voiceIndex >= (int)g.shapePerVoice.size() ||
                g.shapePerVoice[voiceIndex].empty()) {
                int v = g.getCostAt(costIndex, pos, voiceIndex);
                if (v >= 0) return v;
            }
        }
        // valeur par défaut
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
        // shape explicite pour cette voix
        for (const auto& g : groups) {
            if (voiceIndex < (int)g.shapePerVoice.size() &&
                !g.shapePerVoice[voiceIndex].empty()) {
                for (int idx : g.costIndices) {
                    if (idx == costIndex)
                        return g.getCostsAt(pos, voiceIndex);
                }
            }
        }
        // defaultShape
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
            0 // pas dans vecteur importance (sorte de multiplicateur si une certaine note n'est pas une quinte parfaite)
        };
    }

    // retourne le maximum des couts par défaut g_costs
    std::vector<int> getGeneralCostsMax(int voiceIndex = 0) const {
        return {
            defaultCosts[COST_BORROW],
            defaultCosts[COST_FIFTH],
            defaultCosts[COST_OCTAVE],
            defaultCosts[COST_SUCC],
            defaultCosts[COST_VARIETY],
            defaultCosts[COST_TRIAD],
            defaultCosts[COST_DIRECT],
            0 
        };
    }

    // Retourne le vecteur s_costs à la position pos
    // (penultSixth, cambiata, mSkip, triad3, m2, syncopation, prefSlider)
    std::vector<int> getSpecificCostsAt(int pos, int voiceIndex = 0) const {
        return {
            getCostAt(COST_PENULT,      pos, voiceIndex),
            getCostAt(COST_CAMBIATA,    pos, voiceIndex),
            0,  // mSkipCost — pas dans vecteur importance, inactif (cout si en 3sp pas de mouvement contraire après un saut)
            getCostAt(COST_TRIAD3,      pos, voiceIndex),
            getCostAt(COST_M2,          pos, voiceIndex),
            getCostAt(COST_SYNCOPATION, pos, voiceIndex),
            50  // prefSlider — pas dans vecteur importance, inactif (curseur entre 0 et 100 en 5sp, 1/2 sp vs 3/4 sp)
        };
    }

    // retourne le maximum des couts par défaut 
    std::vector<int> getSpecificCostsMax(int voiceIndex = 0) const {
        return {
            defaultCosts[COST_PENULT],
            defaultCosts[COST_CAMBIATA],
            0,   // mSkipCost
            defaultCosts[COST_TRIAD3],
            defaultCosts[COST_M2],
            defaultCosts[COST_SYNCOPATION],
            50   // prefSlider
        };
    }

    // Retourne la taille de la shape mélodique pour une voix donnée
    int getMelodicShapeSize(int voiceIndex) const {
        for (const auto& g : groups)
            for (int idx : g.costIndices)
                if (idx == COST_MELODIC)
                    return (int)g.getShapeForVoice(voiceIndex).size();
        return 0;
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