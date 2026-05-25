// =====================================================================
// CostModel.cpp — Paramètres de coûts pour FuxCP
// Auteur : Dorian Genon
// =====================================================================

#include "../headers/CostModel.hpp"
#include <cmath>
#include <algorithm>

static double between01(double x) {
    return std::max(0.0, std::min(1.0, x));
}
static int q(double x) {
    return (int)std::lround(x);
}

// =====================================================================
// SLIDERS SHAPES
// =====================================================================
std::vector<double> build_constant_zero_shape(int n) {
    return std::vector<double>(n, 0.0);
}
std::vector<double> build_constant_one_shape(int n) {
    return std::vector<double>(n, 1.0);
}
std::vector<double> build_linear_shape(int n) {
    std::vector<double> s; s.reserve(n);
    for (int i = 0; i < n; ++i)
        s.push_back(n <= 1 ? 0.0 : (double)i / (n - 1));
    return s;
}
std::vector<double> build_linear_shape_desc(int n) {
    std::vector<double> s; s.reserve(n);
    for (int i = 0; i < n; ++i)
        s.push_back(n <= 1 ? 1.0 : 1.0 - (double)i / (n - 1));
    return s;
}
std::vector<double> build_inverted_v_shape(int n) {
    std::vector<double> s; s.reserve(n);
    for (int i = 0; i < n; ++i) {
        double x = n <= 1 ? 0.0 : (double)i / (n - 1);
        s.push_back(1.0 - 2.0 * std::abs(x - 0.5));
    }
    return s;
}
std::vector<double> build_v_shape(int n) {
    std::vector<double> s; s.reserve(n);
    for (int i = 0; i < n; ++i) {
        double x = n <= 1 ? 1.0 : (double)i / (n - 1);
        s.push_back(std::abs(2.0 * x - 1.0));
    }
    return s;
}
std::vector<double> build_M_shape(int n) {
    std::vector<double> s; s.reserve(n);
    for (int i = 0; i < n; ++i) {
        double x = n <= 1 ? 0.0 : (double)i / (n - 1);
        double v;
        if      (x <= 0.25) v = x / 0.25;
        else if (x <= 0.5)  v = 1.0 - (x - 0.25) / 0.25;
        else if (x <= 0.75) v = (x - 0.5) / 0.25;
        else                v = 1.0 - (x - 0.75) / 0.25;
        s.push_back(v);
    }
    return s;
}

// =====================================================================
// FONCTIONS DE GROUPE
// =====================================================================

// Coûts mélodiques : s=0 favorise conjoints, s=1 favorise sauts
std::vector<int> steps1(double s) {
    s = between01(s);
    return {
        q(4.0 * s),         // 2nd
        q(1.0 + 2.0 * s),   // 3rd
        2,                   // 4th  fixe
        576,                 // tritone toujours très pénalisé
        2,                   // 5th  fixe
        q(3.0 - 2.0 * s),   // 6th
        q(3.0 - 2.0 * s),   // 7th
        q(4.0 - 4.0 * s)    // 8th
    };
}

std::vector<int> steps2(double s) {
    s = between01(s);
    return {
        q(576.0 - 574.0 * s), // 2nd : de "interdit" à "excellent"
        q(2.0),               // 3rd : fixe et stable
        q(576.0 - 574.0 * s),     // 4th : de "interdit" à "excellent"
        q(576.0 - 560.0 * s), // tritone : de "interdit" à "acceptable"
        q(2.0 + 574.0 * s),    // 5th : de "parfait" à "interdit"
        q(2.0),               // 6th : fixe et stable
        q(576.0 - 574.0 * s), // 7th : de "interdit" à "excellent"
        q(2.0 + 574.0 * s)     // 8th : de "parfait" à "interdit"
    };
}

// Fifth + octave : s=0 peu pénalisés, s=1 fortement pénalisés
std::vector<int> harmo(double s) {
    s = between01(s);
    return {
        q(4.0 * s),        // fifth  : 0 -> 4
        q(4.0 - 4.0 * s)   // octave : 4 -> 0 (sens inverse)
    };
}

// Succ + direct : s=0 peu pénalisés, s=1 fortement pénalisés
std::vector<int> perf_cons(double s) {
    s = between01(s);
    return {
        q(8.0 * s),   // succ
        q(8.0 * s)    // direct
    };
}

// Triad + triad3 : s=0 peu pénalisés, s=1 fortement pénalisés
std::vector<int> triad_group(double s) {
    s = between01(s);
    return {
        q(4.0 * s),   // triad
        q(2.0 * s)    // triad3
    };
}