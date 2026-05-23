// 
// Created by Luc Cleenewerk and Diego de Patoul. 
// This file contains the implementations of general utility functions. 
//

#include "../headers/Utilities.hpp"

vector<bool> activeConstraints = std::vector<bool>(consSize, false);
vector<bool> softConstraints = std::vector<bool>(consSize, false);

string get_constraint_name(int constraint) {
    if (constraint < 0 || constraint >= constraintNames.size()) {
        return "Unknown constraint";
    }
    return constraintNames[constraint];
}

/**
 * For a given set of intervals between notes that loops and a starting note, returns all the possible notes
 * @param note the starting note
 * @param intervals the set of intervals between notes. It must make a loop. For example, to get all notes from a major
 * scale from note, use {2, 2, 1, 2, 2, 2, 1}. To get all notes from a minor chord, use {3, 4, 5}.
 * @return vector<int> all the notes
 */
vector<int> get_all_notes_from_interval_loop(int n, vector<int> intervals)
{
    int note = n % PERFECT_OCTAVE; // bring the root back to [12,23] in case the argument is wrong
    vector<int> notes;

    int i = 0;
    while (note <= 127)
    {
        notes.push_back(note);
        note += intervals[i % intervals.size()];
        ++i;
    }
    return notes;
}

/**
 * For a given tonality (root + mode), returns all the possible notes
 * @param root the root of the tonality (in [0,11])
 * @param scale the set of tones and semitones that define the scale
 * @return vector<int> all the possible notes from that tonality
 */
vector<int> get_all_notes_from_scale(int root, vector<int> scale)
{
    return get_all_notes_from_interval_loop(root, scale);
}


// Liste des gammes candidates pour la détection automatique.
// Les noms affichés utilisent "Majeur" / "Mineur" (au lieu d'Ionien/Aeolien)
// pour la lisibilité utilisateur. Les pentatoniques et blues sont incluses.
static const vector<pair<string, vector<int>>> SCALE_CANDIDATES = {
    {"Majeur",                MAJOR_SCALE},        // = Ionien
    {"Mineur naturel",        AEOLIAN_SCALE},      // = mineur naturel
    {"Dorien",                DORIAN_SCALE},
    {"Mixolydien",            MIXOLYDIAN_SCALE},
    {"Lydien",                LYDIAN_SCALE},
    {"Phrygien",              PHRYGIAN_SCALE},
    {"Locrien",               LOCRIAN_SCALE},
    {"Mineur harmonique",     HARMONIC_MINOR_SCALE},
    {"Mineur mélodique",      MELODIC_MINOR_SCALE},
    {"Pentatonique majeure",  PENTATONIC_MAJOR_SCALE},
    {"Pentatonique mineure",  PENTATONIC_MINOR_SCALE},
    {"Blues majeure",         BLUES_MAJOR_SCALE},
    {"Blues mineure",         BLUES_MINOR_SCALE}
};

static pair<string, vector<int>> detect_scale_pair(const vector<int>& cf) { // Pair name, scale
    if (cf.empty()) {
        return {"Majeur [fallback CF vide]", MAJOR_SCALE};
    }
    int root = ((cf[0] % 12) + 12) % 12;

    // Classes de hauteurs présentes dans le CF
    set<int> cf_pcs;
    for (int n : cf) cf_pcs.insert(((n % 12) + 12) % 12);

    // On cherche la gamme qui couvre le plus grand nombre de notes du CF.
    // En cas d'égalité on garde la première candidate (= la plus générale).
    int best_covered = -1;
    int best_idx = -1;
    for (size_t i = 0; i < SCALE_CANDIDATES.size(); ++i) {
        set<int> scale_pcs;
        int cur = root;
        scale_pcs.insert(cur);
        for (int step : SCALE_CANDIDATES[i].second) {
            cur = (cur + step) % 12;
            scale_pcs.insert(cur);
        }
        int covered = 0;
        for (int pc : cf_pcs) {
            if (scale_pcs.find(pc) != scale_pcs.end()) ++covered;
        }
        if (covered > best_covered) {
            best_covered = covered;
            best_idx = static_cast<int>(i);
            if (covered == static_cast<int>(cf_pcs.size())) {
                // couverture parfaite : on s'arrête (gamme la plus générale qui couvre tout)
                break;
            }
        }
    }
    if (best_idx < 0) {
        return {"Majeur [fallback]", MAJOR_SCALE};
    }
    auto chosen = SCALE_CANDIDATES[best_idx];
    if (best_covered < static_cast<int>(cf_pcs.size())) {
        chosen.first += " [couverture partielle " + std::to_string(best_covered)
                      + "/" + std::to_string(cf_pcs.size()) + "]";
    }
    return chosen;
}

vector<int> detect_scale_for_cf(const vector<int>& cf) {
    return detect_scale_pair(cf).second;
}

string detect_scale_name_for_cf(const vector<int>& cf) {
    return detect_scale_pair(cf).first;
}

/**
 * For a given chord (root + mode), returns all the possible notes
 * @param root the root of the chord
 * @param quality the set of tones and semitones that define the chord
 * @return vector<int> all the possible notes from that chord
 */
vector<int> get_all_notes_from_chord(int root, vector<int> quality)
{
    return get_all_notes_from_interval_loop(root, quality);
}

/**
 * Get all values for a given note
 * @param note a note
 * @return vector<int> a vector containing all the given notes
 */
vector<int> get_all_given_note(int note)
{
    int current = note % PERFECT_OCTAVE;
    vector<int> notes;
    while (current < 127)
    {
        notes.push_back(current);
        current += 12;
    }
    return notes;
}

/**
 * Transforms a vector of integers into a string
 * @param vector a vector of integers
 * @return string the string representation of the vector
 */
string int_vector_to_string(vector<int> vector){
    string s;
    for (int i = 0; i < vector.size(); ++i) {
        s += to_string(vector[i]);
        if(i != vector.size() - 1)
            s += " , ";
    }
    return s;
}

/**
 * Transforms an IntVarArray into a string, with values separated by a space (for csv utilisation)
 * @param array IntVarArray
 * @return string the string representation of the vector
 */
string int_var_array_to_string(IntVarArray array){
    ostringstream oss;
    oss << array;
    string my_str = oss.str();
    return my_str.substr(1,my_str.size()-2);
}

/**
 * Transforms an int* into a vector<int>
 * @param ptr an int* pointer
 * @param size the size of the array
 * @return a vector<int> containing the same values as the array
 */
vector<int> int_pointer_to_vector(int* ptr, int size){
    vector<int> v;
    for(int i = 0; i < size; i++){
        v.push_back(ptr[i]);
    }
    return v;
}

/**
 * Union algorithm found and adapted from GeeksForGeeks.com
 */
vector<int> vector_union(vector<int> v1, vector<int> v2){
    vector<int> v(v1.size() 
                  + v2.size()); 
    vector<int>::iterator it, st; 

    it = set_union(v1.begin(), v1.end(), 
                   v2.begin(), 
                   v2.end(), v.begin()); 

    vector<int> res = {};
    for(st = v.begin(); st != it; ++st){
        res.push_back(*st);
    }
    return res;
}

/**
 * Intersection algorithm found and adapted from GeeksForGeeks.com
 */
vector<int> vector_intersection(vector<int> v1, vector<int> v2){
    vector<int> v(v1.size() 
                  + v2.size()); 
    vector<int>::iterator it, st; 

    it = set_intersection(v1.begin(), v1.end(), 
                   v2.begin(), 
                   v2.end(), v.begin()); 

    vector<int> res = {};
    for(st = v.begin(); st != it; ++st){
        res.push_back(*st);
    }
    return res;
}

/**
 * Difference algorithm found and adapted from GeeksForGeeks.com
 */
vector<int> vector_difference(vector<int> v1, int lb, int ub){
    vector<int> v = {};
    for(int i = lb; i <= ub; i++){
        int t = 0;
        for(int j = 0; j < v1.size(); j++){
            if(i==v1[j]){
                t=1;
                break;
            }
        }
        if(t==0){
            v.push_back(i);
        }
    }
    return v;
}

/**
 * Prints the Search::Statistics object into a readable format
 * @param stats a Search::Statistics object representing the statistics of a search
 * @return The string representation of the statistics object
 */
string statistics_to_string(Search::Statistics stats){
    string s = "Nodes traversed: " + to_string(stats.node) + "\n";
    s += "Failed nodes explored: " + to_string(stats.fail) + "\n";
    s += "Restarts performed: " + to_string(stats.restart) + "\n";
    s += "Propagators executed: " + to_string(stats.propagate) + "\n";
    s += "No goods generated: " + to_string(stats.nogood) + "\n";
    s += "Maximal depth of explored tree: " + to_string(stats.depth) + "\n";
    return s;
}

/**
 * Prints the Search::Statistics object into a csv format (coma separated)
 * @param stats a Search::Statistics object representing the statistics of a search
 * @return The string representation of the statistics object
 */
string statistics_to_csv_string(Search::Statistics stats){
    string s = to_string(stats.node) + ",";
    s += to_string(stats.fail) + ",";
    s += to_string(stats.restart) + ",";
    s += to_string(stats.propagate) + ",";
    s += to_string(stats.nogood) + ",";
    s += to_string(stats.depth) + ",";
    return s;
}

/**
 * Returns the value of a variable as a string
 * @param var an integer variable
 * @param absolute a boolean indicating if the value should be returned as an absolute value (default is false)
 * @return a string representing the value of the variable
 */
string intVar_to_string(const IntVar &var, bool absolute) {
    if (var.assigned()){
        if(absolute)
            return to_string(abs(var.val()));
        return to_string(var.val());
    }
    return "<not assigned>";
}

/**
 * Returns the values of an array of variables as a string
 * @param vars an array of integer variables
 * @return a string representing the values of the variables
 */
string intVarArray_to_string(IntVarArray vars){
    int s = vars.size();
    string res = "{";
    for(int i = 0; i < s; i++){
        res += intVar_to_string(vars[i]);
        if(i != s - 1)
            res += ", ";
    }
    res += "}";
    return res;
}

/**
 * Returns the value of a variable as a string
 * @param var an integer variable
 * @param absolute a boolean indicating if the value should be returned as an absolute value (default is false)
 * @return a string representing the value of the variable
 */
string boolVar_to_string(const BoolVar &var, bool absolute) {
    if (var.assigned()){
        if(absolute)
            return to_string(abs(var.val()));
        return to_string(var.val());
    }
    return "<not assigned>";
}

/**
 * Returns the values of an array of variables as a string
 * @param vars an array of integer variables
 * @return a string representing the values of the variables
 */
string boolVarArray_to_string(BoolVarArray vars){
    int s = vars.size();
    string res = "{";
    for(int i = 0; i < s; i++){
        res += boolVar_to_string(vars[i]);
        if(i != s - 1)
            res += ", ";
    }
    res += "}";
    return res;
}

/**
 * Returns the values of an array of variables as a string
 * @param vars an array of integer variables
 * @return a string representing the values of the variables, ignoring unassigned variables
 */
string cleanIntVarArray_to_string(IntVarArray vars){
    int s = vars.size();
    string res = "{";
    for(int i = 0; i < s; i++){
        if (vars[i].assigned()){
            res += intVar_to_string(vars[i]);
            if(i != s - 1)
                res += ", ";
        }
    }
    res += "}";
    return res;
}

/**
 * Returns the values of an IntVarArgs as a string
 * @param args an IntVarArgs
 * @return a string representing the values
 */
string intVarArgs_to_string(IntVarArgs args){
    int s = args.size();
    string res;
    for(int i = 0; i < s; i++){
        res += intVar_to_string(args[i], 0);
        if(i != s - 1)
            res += ", ";
    }
    return res;
}

Species int_to_species(int sp) {
    switch (sp) {
        case 1: return FIRST_SPECIES;
        case 2: return SECOND_SPECIES;
        case 3: return THIRD_SPECIES;
        case 4: return FOURTH_SPECIES;
        case 5: return FIFTH_SPECIES;
        default: return THIRD_SPECIES;
    }
}

string midi_to_french(int note) {
    static const string names[] = {"Do","Do#","Ré","Mib","Mi","Fa","Fa#","Sol","Lab","La","Sib","Si"};
    int oct = note / 12 - 1;
    return names[note % 12] + to_string(oct);
}


/**
 * Returns the name of a note based on its MIDI value
 * @param note an integer
 */
string midi_to_letter(int note){
    return noteNames[note % PERFECT_OCTAVE];
}

/**
 * Returns the name of a mode based on its integer value
 * @param mode an integer
 * @return a string representing the name of the mode
 */
string mode_int_to_name(int mode){
    return modeNames[mode];
}

/**
 * returns a string with the time
 * @return a string with the time
 */
string time(){
    /// date and time for logs
    std::time_t currentTime = std::time(nullptr); // Get the current time
    std::string timeString = std::asctime(std::localtime(&currentTime)); // Convert to string
    return timeString;
}

/**
 * Write a text into a log file
 * Useful for debugging in the OM environment
 * @param message the text to write
 */
void write_to_log_file(const char *message, const string& filename) { // TODO : TO ACTIVATE LOGGING, UNCOOMMENT THE CONTENTS OF THIS FUNCTION AND THE NEXT FUNCTION.
    // const char* homeDir = std::getenv("HOME"); // Get the user's home directory
    // if (homeDir) {
    //     std::string filePath(homeDir);
    //     filePath += "/Documents/Libraries/MusicConstraints/out/" + filename; // Specify the desired file path, such as $HOME/log.txt

    //     std::ofstream myfile(filePath, std::ios::app); // append mode
    //     if (myfile.is_open()) {
    //         myfile << message << endl;
    //         myfile.close();
    //     }
    // }
}

/**
 * Write a text into a log file
 * @param message the text to write
 */
void writeToLogFile(const char* message){ // TODO : TO ACTIVATE LOGGING, UNCOOMMENT THE CONTENTS OF THIS FUNCTION AND THE PREVIOUS FUNCTION.
    // std::time_t currentTime = std::time(nullptr); // Get the current time
    // std::string timeString = std::asctime(std::localtime(&currentTime)); // Convert to string

    // const char* homeDir = std::getenv("HOME"); // Get the user's home directory
    // if (homeDir) {
    //     std::string filePath(homeDir);
    //     filePath += "/log.txt"; // Specify the desired file path, such as $HOME/log.txt

    //     std::ofstream myfile(filePath, std::ios::app); // append mode
    //     if (myfile.is_open()) {
    //         myfile <<timeString<< endl << message << endl;
    //         myfile.close();
    //     }
    // }
}


/* ================================================
 *         CONSTRAINT HELPERS
 * ================================================
 */

IntVarArray expandCantusNotes(Home home, IntVarArray cantus){
    IntVarArray expandedCantus = IntVarArray(home, 4*cantus.size()-3, 0, 127);
    for (int i = 0; i < expandedCantus.size(); i++){
        expandedCantus[i] = cantus[i/4];
    }
    return expandedCantus;
}

vector<int> createRangeVector(int from, int to, int multiplier) {
    vector<int> indices;
    for (int i = from; i < to; i++) {
        indices.push_back(i*multiplier);
    }
    return indices;
}

BranchVarSel g_solution_var_sel = BR_VAR_AFC_MAX;
BranchValSel g_solution_val_sel = BR_VAL_RND;
unsigned int g_solution_val_rnd_seed = 1U;

const char* branch_var_sel_name(BranchVarSel s) {
    switch (s) {
        case BR_VAR_SIZE_MIN:   return "size-min";
        case BR_VAR_SIZE_MAX:   return "size-max";
        case BR_VAR_DEGREE_MAX: return "degree-max";
        case BR_VAR_AFC_MAX:    return "afc-max";
        case BR_VAR_ACTION_MAX: return "action-max";
        case BR_VAR_NONE:       return "none";
    }
    return "unknown";
}

const char* branch_val_sel_name(BranchValSel s) {
    switch (s) {
        case BR_VAL_MIN:       return "min";
        case BR_VAL_MAX:       return "max";
        case BR_VAL_MED:       return "med";
        case BR_VAL_RND:       return "rnd";
        case BR_VAL_SPLIT_MIN: return "split-min";
        case BR_VAL_SPLIT_MAX: return "split-max";
    }
    return "unknown";
}

BranchVarSel parse_branch_var_sel(const string& s) {
    if (s == "size-min")   return BR_VAR_SIZE_MIN;
    if (s == "size-max")   return BR_VAR_SIZE_MAX;
    if (s == "degree-max") return BR_VAR_DEGREE_MAX;
    if (s == "afc-max")    return BR_VAR_AFC_MAX;
    if (s == "action-max") return BR_VAR_ACTION_MAX;
    if (s == "none")       return BR_VAR_NONE;
    return BR_VAR_AFC_MAX;
}

BranchValSel parse_branch_val_sel(const string& s) {
    if (s == "min")       return BR_VAL_MIN;
    if (s == "max")       return BR_VAL_MAX;
    if (s == "med")       return BR_VAL_MED;
    if (s == "rnd")       return BR_VAL_RND;
    if (s == "split-min") return BR_VAL_SPLIT_MIN;
    if (s == "split-max") return BR_VAL_SPLIT_MAX;
    return BR_VAL_RND;
}

static IntVarBranch resolve_var_branch(BranchVarSel s) {
    switch (s) {
        case BR_VAR_SIZE_MIN:   return INT_VAR_SIZE_MIN();
        case BR_VAR_SIZE_MAX:   return INT_VAR_SIZE_MAX();
        case BR_VAR_DEGREE_MAX: return INT_VAR_DEGREE_MAX();
        case BR_VAR_AFC_MAX:    return INT_VAR_AFC_MAX();
        case BR_VAR_ACTION_MAX: return INT_VAR_ACTION_MAX();
        case BR_VAR_NONE:       return INT_VAR_NONE();
    }
    return INT_VAR_AFC_MAX();
}

static IntValBranch resolve_val_branch(BranchValSel s, unsigned int seed) {
    switch (s) {
        case BR_VAL_MIN:       return INT_VAL_MIN();
        case BR_VAL_MAX:       return INT_VAL_MAX();
        case BR_VAL_MED:       return INT_VAL_MED();
        case BR_VAL_RND:       return INT_VAL_RND(seed);
        case BR_VAL_SPLIT_MIN: return INT_VAL_SPLIT_MIN();
        case BR_VAL_SPLIT_MAX: return INT_VAL_SPLIT_MAX();
    }
    return INT_VAL_RND(seed);
}

void branch_solution_array_dynamic(Home home, const IntVarArgs& vars) {
    branch(home, vars,
           resolve_var_branch(g_solution_var_sel),
           resolve_val_branch(g_solution_val_sel, g_solution_val_rnd_seed));
}
