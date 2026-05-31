// ConfigLoader.hpp
// Chargement des configurations CSV (cantus_firmus, presets).
// Toutes les valeurs autrefois hardcodées dans GenerateCounterpoint.cpp / Generations.cpp
// sont désormais externalisées dans FuxCP/config/*.csv.

#ifndef FUXCP_CONFIG_LOADER_HPP
#define FUXCP_CONFIG_LOADER_HPP

#include <string>
#include <vector>
#include <map>

struct CFEntry {
    int id;
    std::vector<int> notes;  // notes MIDI du cantus firmus
    std::string name;        // libellé court (ex "Do Majeur (court)")
    std::string scale;       // gamme indicative (informatif uniquement)
    std::string description;
};

struct PresetEntry {
    std::string name;
    std::vector<int> melodic_params;   // 8 entiers
    std::vector<int> general_params;   // 8 entiers
    std::vector<int> specific_params;  // 7 entiers
    std::vector<int> importance;       // 14 entiers
    int borrow_mode;
    std::string description;
};

class ConfigLoader {
public:
    // Localise le dossier config/ (cherche ../config/ depuis le CWD courant).
    static std::string find_config_dir();

    // Lecture des deux CSV (chemin = dossier config).
    static std::map<int, CFEntry>     load_cantus_firmus(const std::string& config_dir);
    static std::map<std::string, PresetEntry> load_presets(const std::string& config_dir);

    // Helpers
    static std::vector<int> parse_int_list(const std::string& s, char sep = ' ');
    static std::vector<std::string> split(const std::string& s, char sep);
    static std::string trim(const std::string& s);
};

#endif
