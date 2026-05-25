//
// Created by Damien Sprockeels on 11/06/2024.
// Extended and developed by Luc Cleenewerk and Diego de Patoul up to August 2024. 
//

#include "../../headers/Parts/Part.hpp"
#include "../../headers/CostModel.hpp"

/// This class represents a part, so it creates all the variables associated to that part and posts the constraints that are species independent
Part::Part(Home home, int nMes, Species sp, vector<int> cf, int lb, int ub, int v_type, vector<int> m_costs, vector<int> g_costs, vector<int> s_costs, 
    int nV, int bm, const vector<double>& melodicShape, const CostModel* costModel, int voiceIndex) : 
    Voice(home, nMes, lb, ub){
    nVoices         = nV;
    species = sp;
    borrowMode      = bm;
    voice_type      = v_type;
    //lowest          = low;
    isNotLowest        = BoolVarArray(home, nMeasures, 0, 1);
    isHighest          = BoolVarArray(home, nMeasures, 0, 1);

    borrowed_scale = get_all_notes_from_scale(cf[0]%12, BORROWED_SCALE);
    // Détection automatique de la gamme à partir du cantus firmus complet, plutôt
    // que de toujours imposer MAJOR_SCALE. La première note donne la tonique et
    // l'ensemble des notes du CF permet d'identifier le mode (Ionien, Dorien,
    // Phrygien, Lydien, Mixolydien, Aeolien, Locrien, mineur harmonique, mineur
    // mélodique). En cas d'ambiguïté on retient la gamme la plus générale.
    vector<int> detected_scale = detect_scale_for_cf(cf);
    scale = get_all_notes_from_scale(cf[0]%12, detected_scale);
    chromatic_scale = get_all_notes_from_scale(cf[0]%12, CHROMATIC_SCALE);
    cp_range = {};

    secondCost = m_costs[0];
    thirdCost = m_costs[1];
    fourthCost = m_costs[2];
    tritoneCost = m_costs[3];
    fifthCost = m_costs[4];
    sixthCost = m_costs[5];
    seventhCost = m_costs[6];
    octaveCost = m_costs[7];

    // Dorian Genon
    this->melodicShape = melodicShape;
    if (costModel != nullptr && costModel->isGrouped(COST_MELODIC, voiceIndex)) {
        // Nouvelle API : shape mélodique via CostModel, par voix et par position
        int nPos = nMes - 1;
        secondCostProfile.resize(nPos);
        thirdCostProfile.resize(nPos);
        fourthCostProfile.resize(nPos);
        tritoneCostProfile.resize(nPos);
        fifthCostProfile.resize(nPos);
        sixthCostProfile.resize(nPos);
        seventhCostProfile.resize(nPos);
        octaveCostProfile.resize(nPos);
        for (int i = 0; i < nPos; ++i) {
            vector<int> localCosts = costModel->getGroupCostsAt(COST_MELODIC, i, voiceIndex);
            secondCostProfile[i]  = localCosts[0];
            thirdCostProfile[i]   = localCosts[1];
            fourthCostProfile[i]  = localCosts[2];
            tritoneCostProfile[i] = localCosts[3];
            fifthCostProfile[i]   = localCosts[4];
            sixthCostProfile[i]   = localCosts[5];
            seventhCostProfile[i] = localCosts[6];
            octaveCostProfile[i]  = localCosts[7];
        }
    } else if (!melodicShape.empty()) {
        // Ancienne API — rétrocompatibilité
        // La fonction steps1 est utilisée par défaut avec la melodicShape passée directement.
        // Pour utiliser une autre fonction, passer par le CostModel (nouvelle API).
        secondCostProfile.resize(melodicShape.size());
        thirdCostProfile.resize(melodicShape.size());
        fourthCostProfile.resize(melodicShape.size());
        tritoneCostProfile.resize(melodicShape.size());
        fifthCostProfile.resize(melodicShape.size());
        sixthCostProfile.resize(melodicShape.size());
        seventhCostProfile.resize(melodicShape.size());
        octaveCostProfile.resize(melodicShape.size());
        for (size_t i = 0; i < melodicShape.size(); ++i) {
            vector<int> localCosts = steps1(melodicShape[i]);
            secondCostProfile[i]  = localCosts[0];
            thirdCostProfile[i]   = localCosts[1];
            fourthCostProfile[i]  = localCosts[2];
            tritoneCostProfile[i] = localCosts[3];
            fifthCostProfile[i]   = localCosts[4];
            sixthCostProfile[i]   = localCosts[5];
            seventhCostProfile[i] = localCosts[6];
            octaveCostProfile[i]  = localCosts[7];
        }
    }

    borrowCost = g_costs[0];
    h_fifthCost = g_costs[1];
    h_octaveCost = g_costs[2];
    succCost = g_costs[3];
    varietyCost = g_costs[4];
    triadCost = g_costs[5];
    directMoveCost = g_costs[6];
    penultCost = g_costs[7];

    penultSixthCost = s_costs[0];
    cambiataCost = s_costs[1];
    mSkipCost = s_costs[2];
    triad3rdCost = s_costs[3];
    m2ZeroCost = s_costs[4];
    syncopationCost = s_costs[5];
    prefSlider = s_costs[6];

    directCost = 2;
    obliqueCost = 1;
    contraryCost = 0;

    if (costModel != nullptr) {
        buildCostProfiles(*costModel, voiceIndex, nMes - 1, notes.size());
    }
}

string Part::to_string() const{
    string part = "Part characteristics :\n";
    part += "Part costs : \n";
    part += intVarArray_to_string(costs);
    part += "\n";
    return part;
}

// =====================================================================
// Dorian Genon — construction des profiles positionnels
// Pour chaque coût, si le CostModel définit une shape, on calcule
// le profil complet sur nPositions. Sinon le profil reste vide
// et l'accesseur retourne la valeur fixe.
// =====================================================================
void Part::buildCostProfiles(const CostModel& model, int voiceIdx, int nPosByMeasure, int nPosByNote) {

    // Build par mesure
    auto buildByMeasure = [&](int costIndex, int fixedVal, vector<int>& profile) {
        if (model.isGrouped(costIndex, voiceIdx)) {
            profile.resize(nPosByMeasure);
            for (int i = 0; i < nPosByMeasure; ++i) {
                int v = model.getCostAt(costIndex, i, voiceIdx);
                profile[i] = (v >= 0) ? v : fixedVal;
            }
        }
    };

    // Build par note
    auto buildByNote = [&](int costIndex, int fixedVal, vector<int>& profile) {
        if (model.isGrouped(costIndex, voiceIdx)) {
            profile.resize(nPosByNote);
            for (int i = 0; i < nPosByNote; ++i) {
                int measureIdx = (nPosByMeasure > 0) ? 
                    i * nPosByMeasure / nPosByNote : 0;
                int v = model.getCostAt(costIndex, measureIdx, voiceIdx);
                profile[i] = (v >= 0) ? v : fixedVal;
            }
        }
    };

    buildByNote(COST_BORROW,      borrowCost,      borrowCostProfile);
    buildByMeasure(COST_FIFTH,       h_fifthCost,     hFifthCostProfile);
    buildByMeasure(COST_OCTAVE,      h_octaveCost,    hOctaveCostProfile);
    buildByMeasure(COST_SUCC,        succCost,        succCostProfile);
    buildByNote(COST_VARIETY,     varietyCost,     varietyCostProfile);
    buildByMeasure(COST_TRIAD,       triadCost,       triadCostProfile);
    buildByMeasure(COST_DIRECT,      directMoveCost,  directMoveCostProfile);
    buildByMeasure(COST_PENULT,      penultCost,      penultCostProfile);
    buildByMeasure(COST_CAMBIATA,    cambiataCost,    cambiataCostProfile);
    buildByMeasure(COST_TRIAD3,      triad3rdCost,    triad3rdCostProfile);
    buildByNote(COST_M2,          m2ZeroCost,      m2ZeroCostProfile);
    buildByMeasure(COST_SYNCOPATION, syncopationCost, syncopationCostProfile);
}




Part::Part(Home home, Part& s) : Voice(home, s) {
    nVoices = s.nVoices;
    borrowMode = s.borrowMode;
    
    borrowed_scale = s.borrowed_scale;
    scale = s.scale;
    chromatic_scale = s.chromatic_scale;
    species = s.species;
    voice_type = s.voice_type;
    
    cp_range = s.cp_range;

    domain = s.domain;
    off_domain = s.off_domain;

    secondCost = s.secondCost;
    thirdCost = s.thirdCost;
    fourthCost = s.fourthCost;
    tritoneCost = s.tritoneCost;
    fifthCost = s.fifthCost;
    sixthCost = s.sixthCost;
    seventhCost = s.seventhCost;
    octaveCost = s.octaveCost;

    borrowCost = s.borrowCost;
    h_fifthCost = s.h_fifthCost;
    h_octaveCost = s.h_octaveCost;
    succCost = s.succCost;
    varietyCost = s.varietyCost;
    triadCost = s.triadCost;
    directMoveCost = s.directMoveCost;
    penultCost = s.penultCost;

    penultSixthCost = s.penultSixthCost;
    cambiataCost = s.cambiataCost;
    mSkipCost = s.mSkipCost;
    triad3rdCost = s.triad3rdCost;
    m2ZeroCost = s.m2ZeroCost;
    syncopationCost = s.syncopationCost;
    prefSlider = s.prefSlider;

    directCost = s.directCost;
    obliqueCost = s.obliqueCost;
    contraryCost = s.contraryCost;

    
    melodicShape       = s.melodicShape;
    secondCostProfile  = s.secondCostProfile;
    thirdCostProfile   = s.thirdCostProfile;
    fourthCostProfile  = s.fourthCostProfile;
    tritoneCostProfile = s.tritoneCostProfile;
    fifthCostProfile   = s.fifthCostProfile;
    sixthCostProfile   = s.sixthCostProfile;
    seventhCostProfile = s.seventhCostProfile;
    octaveCostProfile  = s.octaveCostProfile;
    // Dorian Genon — copie des profiles positionnels
    borrowCostProfile      = s.borrowCostProfile;
    hFifthCostProfile      = s.hFifthCostProfile;
    hOctaveCostProfile     = s.hOctaveCostProfile;
    succCostProfile        = s.succCostProfile;
    varietyCostProfile     = s.varietyCostProfile;
    triadCostProfile       = s.triadCostProfile;
    directMoveCostProfile  = s.directMoveCostProfile;
    penultCostProfile      = s.penultCostProfile;
    cambiataCostProfile    = s.cambiataCostProfile;
    triad3rdCostProfile    = s.triad3rdCostProfile;
    m2ZeroCostProfile      = s.m2ZeroCostProfile;
    syncopationCostProfile = s.syncopationCostProfile;

    cost_names = s.cost_names;

    toCombineCosts = s.toCombineCosts;
    toCombineCostNames = s.toCombineCostNames;

    melodicDegreeCost.update(home, s.melodicDegreeCost);
    fifthCostArray.update(home, s.fifthCostArray);
    octaveCostArray.update(home, s.octaveCostArray);
    is_off.update(home, s.is_off);
    offCostArray.update(home, s.offCostArray);
    costs.update(home, s.costs);
    varietyCostArray.update(home, s.varietyCostArray);
    directCostArray.update(home, s.directCostArray);
    isConsonance.update(home, s.isConsonance);
    isNotLowest.update(home, s.isNotLowest);
    isHighest.update(home, s.isHighest);

    firstSpeciesHarmonicIntervals.update(home, s.firstSpeciesHarmonicIntervals); // Interval for the first note of each measure
    firstSpeciesNotesCp.update(home, s.firstSpeciesNotesCp);
    firstSpeciesMelodicIntervals.update(home, s.firstSpeciesMelodicIntervals);
    firstSpeciesMotions.update(home, s.firstSpeciesMotions);
    firstSpeciesMotionCosts.update(home, s.firstSpeciesMotionCosts);

    isDiminution.update(home, s.isDiminution);
    penultCostArray.update(home, s.penultCostArray);
    secondSpeciesMotions.update(home, s.secondSpeciesMotions);
    secondSpeciesMelodicIntervals.update(home, s.secondSpeciesMelodicIntervals);
    secondSpeciesRealMotions.update(home, s.secondSpeciesRealMotions);

    is5QNArray.update(home, s.is5QNArray);
    thirdSpeciesHarmonicIntervals.update(home, s.thirdSpeciesHarmonicIntervals); // Interval for the each measure
    thirdSpeciesMelodicIntervals.update(home, s.thirdSpeciesMelodicIntervals);
    cambiataCostArray.update(home, s.cambiataCostArray);

    isNoSyncopeArray.update(home, s.isNoSyncopeArray);
    snycopeCostArray.update(home, s.snycopeCostArray);

    speciesArray.update(home, s.speciesArray);

    toCombineCosts.update(home, s.toCombineCosts);

    relaxationCostArray.update(home, s.relaxationCostArray);
}

// Virtual clone function
Part* Part::clone(Home home) {
    return new Part(home, *this);
}

IntVarArray Part::getBranchingNotes(){
    return notes;
}

IntVarArray Part::getPartNotes(){
    return notes;
}

int Part::getSuccCost(){
    return succCost;
}

IntVarArray Part::getFirstHInterval(){
    return h_intervals;
}

IntVarArray Part::getMotions(){
    return motions;
}

IntVarArray Part::getFirstMInterval(){
    return m_intervals_brut;
}

IntVarArray Part::getCosts(){
    return costs;
}

int Part::getVarietyCost(){
    return varietyCost;
}

IntVar Part::getVarietyArray(int idx){
    return varietyCostArray[idx];
}

int Part::getHIntervalSize(){
    return h_intervals.size();
}

int Part::getTriadCost(){
    return triadCost;
}

int Part::getSecondCost(){
    return secondCost;
}
int Part::getThirdCost(){
    return thirdCost;
}
int Part::getFourthCost(){
    return fourthCost;
}
int Part::getTritoneCost(){
    return tritoneCost;
}
int Part::getFifthCost(){
    return fifthCost;
}
int Part::getSixthCost(){
    return sixthCost;
}
int Part::getSeventhCost(){
    return seventhCost;
}
int Part::getOctaveCost(){
    return octaveCost;
}

int Part::getSecondCostAt(int idx) const {
    if (idx >= 0 && idx < (int)secondCostProfile.size()) return secondCostProfile[idx];
    return secondCost;
}
int Part::getThirdCostAt(int idx) const {
    if (idx >= 0 && idx < (int)thirdCostProfile.size()) return thirdCostProfile[idx];
    return thirdCost;
}
int Part::getFourthCostAt(int idx) const {
    if (idx >= 0 && idx < (int)fourthCostProfile.size()) return fourthCostProfile[idx];
    return fourthCost;
}
int Part::getTritoneCostAt(int idx) const {
    if (idx >= 0 && idx < (int)tritoneCostProfile.size()) return tritoneCostProfile[idx];
    return tritoneCost;
}
int Part::getFifthCostAt(int idx) const {
    if (idx >= 0 && idx < (int)fifthCostProfile.size()) return fifthCostProfile[idx];
    return fifthCost;
}
int Part::getSixthCostAt(int idx) const {
    if (idx >= 0 && idx < (int)sixthCostProfile.size()) return sixthCostProfile[idx];
    return sixthCost;
}
int Part::getSeventhCostAt(int idx) const {
    if (idx >= 0 && idx < (int)seventhCostProfile.size()) return seventhCostProfile[idx];
    return seventhCost;
}
int Part::getOctaveCostAt(int idx) const {
    if (idx >= 0 && idx < (int)octaveCostProfile.size()) return octaveCostProfile[idx];
    return octaveCost;
}

int Part::getBorrowCostAt(int idx) const {
    if (idx >= 0 && idx < (int)borrowCostProfile.size())
        return borrowCostProfile[idx];
    return borrowCost;
}
int Part::getHFifthCostAt(int idx) const {
    if (idx >= 0 && idx < (int)hFifthCostProfile.size())
        return hFifthCostProfile[idx];
    return h_fifthCost;
}
int Part::getHOctaveCostAt(int idx) const {
    if (idx >= 0 && idx < (int)hOctaveCostProfile.size())
        return hOctaveCostProfile[idx];
    return h_octaveCost;
}
int Part::getSuccCostAt(int idx) const {
    if (idx >= 0 && idx < (int)succCostProfile.size())
        return succCostProfile[idx];
    return succCost;
}
int Part::getVarietyCostAt(int idx) const {
    if (idx >= 0 && idx < (int)varietyCostProfile.size())
        return varietyCostProfile[idx];
    return varietyCost;
}
int Part::getTriadCostAt(int idx) const {
    if (idx >= 0 && idx < (int)triadCostProfile.size())
        return triadCostProfile[idx];
    return triadCost;
}
int Part::getDirectMoveCostAt(int idx) const {
    if (idx >= 0 && idx < (int)directMoveCostProfile.size())
        return directMoveCostProfile[idx];
    return directMoveCost;
}
int Part::getPenultCostAt(int idx) const {
    if (idx >= 0 && idx < (int)penultCostProfile.size())
        return penultCostProfile[idx];
    return penultCost;
}
int Part::getCambiataCostAt(int idx) const {
    if (idx >= 0 && idx < (int)cambiataCostProfile.size())
        return cambiataCostProfile[idx];
    return cambiataCost;
}
int Part::getTriad3rdCostAt(int idx) const {
    if (idx >= 0 && idx < (int)triad3rdCostProfile.size())
        return triad3rdCostProfile[idx];
    return triad3rdCost;
}
int Part::getM2ZeroCostAt(int idx) const {
    if (idx >= 0 && idx < (int)m2ZeroCostProfile.size())
        return m2ZeroCostProfile[idx];
    return m2ZeroCost;
}
int Part::getSyncopationCostAt(int idx) const {
    if (idx >= 0 && idx < (int)syncopationCostProfile.size())
        return syncopationCostProfile[idx];
    return syncopationCost;
}

IntVarArray Part::getMelodicDegreeCost(){
    return melodicDegreeCost;
}

vector<string> Part::getCostNames(){
    return cost_names;
}

BoolVarArray Part::getConsonance(){
    return isConsonance;
}

void Part::add_cost(Home home, int idx, IntVarArray to_be_added, IntVarArray costs){
    int sz = to_be_added.size();
    IntVarArgs args(sz);
    for(int i = 0; i < sz; i++){
        args[i] = to_be_added[i];
    }
    rel(home, costs[idx], IRT_EQ, expr(home, sum(args)));
}

void Part::add_toCombineCost(Home home, int idx, IntVarArray to_be_added, IntVarArray costs) {
    int sz = to_be_added.size();
    IntVarArgs args(sz);
    for(int i = 0; i < sz; i++){
        args[i] = to_be_added[i];
    }
    rel(home, costs[idx], IRT_EQ, expr(home, sum(args)));
}

IntVarArray Part::getRelaxationCostArray(){
    return relaxationCostArray;
}

void Part::initRelaxationCostArray(Home home, int size){
    relaxationCostArray = IntVarArray(home, size, 0, 1);
}

BoolVarArray Part::getIsNotLowest(){
    return isNotLowest;
}

IntVarArray Part::getFirstSpeciesHIntervals(){
    return firstSpeciesHarmonicIntervals;
}

IntVarArray Part::getFirstSpeciesNotes(){
    return firstSpeciesNotesCp;
}

IntVarArray Part::getFirstSpeciesMIntervals(){
    return firstSpeciesMelodicIntervals;
}

IntVarArray Part::getFifthCostArray(){
    return fifthCostArray;
}

IntVarArray Part::getOctaveCostArray(){
    return octaveCostArray;
}

int Part::getHFifthCost(){
    return h_fifthCost;
}

int Part::getHOctaveCost(){
    return h_octaveCost;
}

IntVarArray Part::getFirstSpeciesMotions(){
    return firstSpeciesMotions;
}

IntVarArray Part::getDirectCostArray(){
    return directCostArray;
}

int Part::getDirectCost(){
    return directCost;
}

BoolVarArray Part::getIsOffArray(){
    return is_off;
}

vector<int> Part::getOffDomain(){
    return off_domain;
}

vector<int> Part::getDomain(){
    return domain;
}

IntVarArray Part::getOffCostArray(){
    return offCostArray;
}

int Part::getBorrowCost(){
    return borrowCost;
}

BoolVarArray Part::getIsDiminution(){
    return isDiminution;
}

int Part::getPenultCost(){
    return penultCost;
}

IntVarArray Part::getPenultCostArray(){
    return penultCostArray;
}

IntVarArray Part::getSecondSpeciesMotions(){
    return secondSpeciesMotions;
}

IntVarArray Part::getSecondSpeciesMIntervals(){
    return secondSpeciesMelodicIntervals;
}

IntVarArray Part::getSecondSpeciesRealMotions(){
    return secondSpeciesRealMotions;
}

int Part::getDirectMoveCost(){
    return directMoveCost;
}

BoolVarArray Part::getIs5QNArray(){
    return is5QNArray;
}

IntVarArray Part::getThirdSpeciesHIntervals(){
    return thirdSpeciesHarmonicIntervals;
}

IntVarArray Part::getThirdSpeciesMIntervals(){
    return thirdSpeciesMelodicIntervals;
}

int Part::getCambiataCost(){
    return cambiataCost;
}

IntVarArray Part::getCambiataCostArray(){
    return cambiataCostArray;
}

BoolVarArray Part::getNoSyncope(){
    return isNoSyncopeArray;
}

IntVarArray Part::getSyncopeCostArray(){
    return snycopeCostArray;
}

IntVarArray Part::getFourthSpeciesMIntervals(){
    return fourthSpeciesMelodicIntervals;
}

IntVarArray Part::getSpeciesArray(){
    return speciesArray;
}

BoolVarArray Part::getIsHighest(){
    return isHighest;
}

IntVarArray Part::getToCombineCosts(){
    return toCombineCosts;
}

vector<string> Part::getToCombineCostNames(){
    return toCombineCostNames;
}