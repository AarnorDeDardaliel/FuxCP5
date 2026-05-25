//
// Created by Damien Sprockeels on 11/06/2024.
// Extended and developed by Luc Cleenewerk and Diego de Patoul up to August 2024. 
//

#ifndef FUXCP_BASE_COUNTERPOINT_HPP
#define FUXCP_BASE_COUNTERPOINT_HPP

#include "../Utilities.hpp"
#include "../Voice.hpp"
#include "../Stratum.hpp"
#include "../constraints.hpp"
#include "../CostModel.hpp"

#include "gecode/kernel.hh"
#include "gecode/int.hh"
#include "gecode/search.hh"
#include "gecode/minimodel.hh"
#include "gecode/set.hh"

using namespace Gecode;
using namespace Gecode::Search;
using namespace std;

/// abstract class!!! Should not be instanciated
/// This class represents a part, so it creates all the variables associated to that part and posts the constraints that are species independent
class Part : public Voice {
    protected:
        Species species;
        int nVoices;
        int borrowMode;
        int voice_type;
        //Stratum* lowest;
        IntVarArray melodicDegreeCost;
        IntVarArray fifthCostArray;
        IntVarArray octaveCostArray;
        BoolVarArray is_off;
        IntVarArray offCostArray;
        IntVarArray costs;
        IntVarArray varietyCostArray;
        IntVarArray directCostArray;
        BoolVarArray isConsonance;
        IntVarArray speciesArray;
        BoolVarArray isNotLowest;
        BoolVarArray isHighest;

        vector<int> borrowed_scale;
        vector<int> scale;
        vector<int> chromatic_scale;

        vector<int> cp_range;

        vector<int> domain;
        vector<int> off_domain;

        vector<string> cost_names;

        int secondCost;
        int thirdCost;
        int fourthCost;
        int tritoneCost;
        int fifthCost;
        int sixthCost;
        int seventhCost;
        int octaveCost;

        int borrowCost;
        int h_fifthCost;
        int h_octaveCost;
        int succCost;
        int varietyCost;
        int triadCost;
        int directMoveCost;
        int penultCost;

        int penultSixthCost;
        int cambiataCost;
        int mSkipCost;
        int triad3rdCost;
        int m2ZeroCost;
        int syncopationCost;
        int prefSlider;

        // Dorian Genon
        // Vide = coût fixe, rempli = coût varie selon la position
        vector<double> melodicShape;  // shape positionnelle (vide = coûts constants)
        vector<int> secondCostProfile;
        vector<int> thirdCostProfile;
        vector<int> fourthCostProfile;
        vector<int> tritoneCostProfile;
        vector<int> fifthCostProfile;
        vector<int> sixthCostProfile;
        vector<int> seventhCostProfile;
        vector<int> octaveCostProfile;
        vector<int> borrowCostProfile;
        vector<int> hFifthCostProfile;
        vector<int> hOctaveCostProfile;
        vector<int> succCostProfile;
        vector<int> varietyCostProfile;
        vector<int> triadCostProfile;
        vector<int> directMoveCostProfile;
        vector<int> penultCostProfile;
        vector<int> cambiataCostProfile;
        vector<int> triad3rdCostProfile;
        vector<int> m2ZeroCostProfile;
        vector<int> syncopationCostProfile;



        IntVarArray relaxationCostArray;

        int directCost;
        int obliqueCost;
        int contraryCost;

        IntVarArray toCombineCosts;
        vector<string> toCombineCostNames;

        //First Species specific variables
        IntVarArray firstSpeciesNotesCp;
        IntVarArray firstSpeciesHarmonicIntervals;
        IntVarArray firstSpeciesMelodicIntervals;
        IntVarArray firstSpeciesMotions;
        IntVarArray firstSpeciesMotionCosts;

        //Second species specific variables
        BoolVarArray isDiminution;
        IntVarArray penultCostArray;
        IntVarArray secondSpeciesMotions;
        IntVarArray secondSpeciesMelodicIntervals;
        IntVarArray secondSpeciesRealMotions;

        //Third species specific variables
        IntVarArray thirdSpeciesHarmonicIntervals;
        IntVarArray thirdSpeciesMelodicIntervals;
        BoolVarArray is5QNArray;
        IntVarArray cambiataCostArray;

        //Fourth species specific variables
        BoolVarArray isNoSyncopeArray;
        IntVarArray snycopeCostArray;
        IntVarArray fourthSpeciesMelodicIntervals;

    public:
        Part(Home home, int nMes, Species sp, vector<int> cf, int lb, int ub, int v_type, vector<int> m_costs, vector<int> g_costs,
            vector<int> s_costs, int nV, int bm, const vector<double>& melodicShape = {}, const CostModel* costModel = nullptr, int voiceIndex = 0);

        // Part(Part& s); (no longer copy constructor since not a space anymore. Now just a clone constructor to deep copy the object (called by the Space's copy constructor))
        Part(Home home, Part& s);  // clone constructor

        /// must be implemented in the child classes, returns the variables to branch on
        // virtual IntVarArray getBranchingNotes();

        virtual Part* clone(Home home) override;

        virtual string to_string() const override;

        virtual IntVarArray getBranchingNotes();

        virtual IntVarArray getFirstHInterval();

        virtual IntVarArray getFirstMInterval();

        virtual IntVarArray getMotions();
        
        int getSpecies() { return species; }

        IntVarArray getPartNotes();

        IntVarArray getCosts();

        BoolVarArray getIsHighest();

        int getSuccCost();

        int getTriadCost();

        int getVarietyCost();

        IntVar getVarietyArray(int idx);

        virtual int getHIntervalSize();

        BoolVarArray getConsonance();

        BoolVarArray getIsNotLowest();

        int getSecondCost();
        int getThirdCost();
        int getFourthCost();
        int getTritoneCost();
        int getFifthCost();
        int getSixthCost();
        int getSeventhCost();
        int getOctaveCost();
        int getHFifthCost();
        int getHOctaveCost();
        int getDirectCost();
        int getBorrowCost();
        int getPenultCost();
        int getDirectMoveCost();
        int getCambiataCost();

        int getSecondCostAt(int idx) const;
        int getThirdCostAt(int idx) const;
        int getFourthCostAt(int idx) const;
        int getTritoneCostAt(int idx) const;
        int getFifthCostAt(int idx) const;
        int getSixthCostAt(int idx) const;
        int getSeventhCostAt(int idx) const;
        int getOctaveCostAt(int idx) const;

    
        // Retournent la valeur du profile à idx si défini, sinon la valeur fixe
        int getBorrowCostAt(int idx)       const;
        int getHFifthCostAt(int idx)       const;
        int getHOctaveCostAt(int idx)      const;
        int getSuccCostAt(int idx)         const;
        int getVarietyCostAt(int idx)      const;
        int getTriadCostAt(int idx)        const;
        int getDirectMoveCostAt(int idx)   const;
        int getPenultCostAt(int idx)       const;
        int getCambiataCostAt(int idx)     const;
        int getTriad3rdCostAt(int idx)     const;
        int getM2ZeroCostAt(int idx)       const;
        int getSyncopationCostAt(int idx)  const;

        void buildCostProfiles(const CostModel& model, int voiceIdx, int nPosByMeasure, int nPosByNote);

        IntVarArray getMelodicDegreeCost();

        IntVarArray getFirstSpeciesHIntervals();

        IntVarArray getFirstSpeciesNotes();

        IntVarArray getFifthCostArray();

        IntVarArray getOctaveCostArray();

        IntVarArray getFirstSpeciesMIntervals();

        IntVarArray getFirstSpeciesMotions();

        IntVarArray getDirectCostArray();

        BoolVarArray getIsOffArray();

        vector<int> getOffDomain();

        vector<int> getDomain();

        IntVarArray getOffCostArray();

        BoolVarArray getIsDiminution();

        IntVarArray getPenultCostArray();

        IntVarArray getSecondSpeciesMotions();

        IntVarArray getSecondSpeciesMIntervals();

        IntVarArray getSecondSpeciesRealMotions();

        BoolVarArray getIs5QNArray();

        IntVarArray getThirdSpeciesHIntervals();

        IntVarArray getThirdSpeciesMIntervals();

        IntVarArray getCambiataCostArray();

        BoolVarArray getNoSyncope();

        IntVarArray getSyncopeCostArray();

        IntVarArray getFourthSpeciesMIntervals();

        IntVarArray getSpeciesArray();

        void add_cost(Home home, int idx, IntVarArray to_be_added, IntVarArray costs);

        vector<string> getCostNames();
        
        IntVarArray getToCombineCosts();

        vector<string> getToCombineCostNames();

        void add_toCombineCost(Home home, int idx, IntVarArray to_be_added, IntVarArray costs);

        IntVarArray getRelaxationCostArray();
        void initRelaxationCostArray(Home home, int size);
};


#endif //FUXCP_BASE_COUNTERPOINT_HPP
