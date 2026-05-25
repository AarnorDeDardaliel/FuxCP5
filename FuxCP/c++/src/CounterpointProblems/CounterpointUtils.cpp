// 
// Created by Luc Cleenewerk and Diego de Patoul. 
// This file contains the implementations of the main functions to create problems and counterpoints.  
//

#include "../../headers/CounterpointProblems/CounterpointUtils.hpp"


Part* create_counterpoint(Home home, int species, int nMeasures, vector<int> cantusFirmus, int lowerBound, int upperBound, Stratum* low,
    CantusFirmus* c, int v_type, vector<int> m_costs, vector<int> g_costs, vector<int> s_costs, int bm, int nV, const vector<double>& melodicShape, const CostModel* costModel, int voiceIndex){
    switch(nV) {
        case TWO_VOICES :
            switch (species) { /// call the appropriate constructor for the counterpoint
            case FIRST_SPECIES:
                return new FirstSpeciesCounterpoint(home, nMeasures, cantusFirmus, lowerBound, upperBound, low, c, v_type, m_costs, g_costs, s_costs, bm, TWO_VOICES, melodicShape, costModel, voiceIndex);
                break;
            case SECOND_SPECIES:
                return new SecondSpeciesCounterpoint(home, nMeasures, cantusFirmus, lowerBound, upperBound, low, c, v_type, m_costs, g_costs, s_costs, bm, TWO_VOICES, melodicShape, costModel, voiceIndex);
                break;
            case THIRD_SPECIES:
                return new ThirdSpeciesCounterpoint(home, nMeasures, cantusFirmus, lowerBound, upperBound, low, c, v_type, m_costs, g_costs, s_costs, bm, TWO_VOICES, melodicShape, costModel, voiceIndex);
                break;
            case FOURTH_SPECIES:
                return new FourthSpeciesCounterpoint(home, nMeasures, cantusFirmus, lowerBound, upperBound, low, c, v_type, m_costs, g_costs, s_costs, bm, TWO_VOICES, melodicShape, costModel, voiceIndex);
                break;
            case FIFTH_SPECIES:
                return new FifthSpeciesCounterpoint(home, nMeasures, cantusFirmus, lowerBound, upperBound, low, c, v_type, m_costs, g_costs, s_costs, bm, TWO_VOICES, melodicShape, costModel, voiceIndex);
                break;
            default:
                throw std::invalid_argument("Species not implemented");
            }
            break;
        case THREE_VOICES :
            switch (species) { /// call the appropriate constructor for the counterpoint
            case FIRST_SPECIES:
                return new FirstSpeciesCounterpoint(home, nMeasures, cantusFirmus, lowerBound, upperBound, low, c, v_type, m_costs, g_costs, s_costs, bm,
                    TWO_VOICES, THREE_VOICES, melodicShape, costModel, voiceIndex);
                break;
            case SECOND_SPECIES:
                return new SecondSpeciesCounterpoint(home, nMeasures, cantusFirmus, lowerBound, upperBound, low, c, v_type, m_costs, g_costs, s_costs, bm, 
                    TWO_VOICES, THREE_VOICES, melodicShape, costModel, voiceIndex);
                break;
            case THIRD_SPECIES:
                return new ThirdSpeciesCounterpoint(home, nMeasures, cantusFirmus, lowerBound, upperBound, low, c, v_type, m_costs, g_costs, s_costs, bm,
                    TWO_VOICES, THREE_VOICES, melodicShape, costModel, voiceIndex);
                break;
            case FOURTH_SPECIES:
                return new FourthSpeciesCounterpoint(home, nMeasures, cantusFirmus, lowerBound, upperBound, low, c, v_type, m_costs, g_costs, s_costs, bm,
                    TWO_VOICES, THREE_VOICES, melodicShape, costModel, voiceIndex);
                break;
            case FIFTH_SPECIES:
                return new FifthSpeciesCounterpoint(home, nMeasures, cantusFirmus, lowerBound, upperBound, low, c, v_type, m_costs, g_costs, s_costs, bm,
                    TWO_VOICES, THREE_VOICES, melodicShape, costModel, voiceIndex);
                break;
            default:
                throw std::invalid_argument("Species not implemented");
            }
            break;
        case FOUR_VOICES :
            switch (species) { /// call the appropriate constructor for the counterpoint
            case FIRST_SPECIES:
                return new FirstSpeciesCounterpoint(home, nMeasures, cantusFirmus, lowerBound, upperBound, low, c, v_type, m_costs, g_costs, s_costs, bm,
                    TWO_VOICES, THREE_VOICES, FOUR_VOICES, melodicShape, costModel, voiceIndex);
                break;
            case SECOND_SPECIES:
                return new SecondSpeciesCounterpoint(home, nMeasures, cantusFirmus, lowerBound, upperBound, low, c, v_type, m_costs, g_costs, s_costs, bm, 
                    TWO_VOICES, THREE_VOICES, FOUR_VOICES, melodicShape, costModel, voiceIndex);
                break;
            case THIRD_SPECIES:
                return new ThirdSpeciesCounterpoint(home, nMeasures, cantusFirmus, lowerBound, upperBound, low, c, v_type, m_costs, g_costs, s_costs, bm,
                    TWO_VOICES, THREE_VOICES, FOUR_VOICES, melodicShape, costModel, voiceIndex);
                break;
            case FOURTH_SPECIES:
                return new FourthSpeciesCounterpoint(home, nMeasures, cantusFirmus, lowerBound, upperBound, low, c, v_type, m_costs, g_costs, s_costs, bm,
                    TWO_VOICES, THREE_VOICES, FOUR_VOICES, melodicShape, costModel, voiceIndex);
                break;
            case FIFTH_SPECIES:
                return new FifthSpeciesCounterpoint(home, nMeasures, cantusFirmus, lowerBound, upperBound, low, c, v_type, m_costs, g_costs, s_costs, bm,
                    TWO_VOICES, THREE_VOICES, FOUR_VOICES, melodicShape, costModel, voiceIndex);
                break;
            default:
                throw std::invalid_argument("Species not implemented");
            }
            break;
        default :
            throw std::invalid_argument("Number of voices not implemented");
    }
};


CounterpointProblem* create_problem(vector<int> cf, vector<Species> spList, vector<int> v_type, vector<int> m_costs, vector<int> g_costs,
    vector<int> s_costs, vector<int> imp, int bm, ObjectiveMode objMode, const vector<double>& melodicShape){

    switch (spList.size())
    {
    case 1:
        return new TwoVoiceCounterpoint(cf, spList[0], v_type[0], m_costs, g_costs, s_costs, imp, bm, objMode, melodicShape);
        break;
    case 2: 
        return new ThreeVoiceCounterpoint(cf, spList, v_type, m_costs, g_costs, s_costs, imp, bm, objMode, melodicShape); 
        break;
    case 3:
        return new FourVoiceCounterpoint(cf, spList, v_type, m_costs, g_costs, s_costs, imp, bm, objMode, melodicShape); 
        break;
    default:
        throw std::invalid_argument("The number of voices you asked for is not implemented (yet).");
        break;
    }
}

// // Dorian Genon : implémentation pour les expériences avec dynamic sliders généralisés
CounterpointProblem* create_problem(vector<int> cf, vector<Species> spList,
    vector<int> v_type,
    const CostModel& costModel, vector<int> imp, int bm,
    ObjectiveMode objMode) {

    switch (spList.size()) {
    case 1:
        return new TwoVoiceCounterpoint(cf, spList[0], v_type[0],
            costModel, imp, bm, objMode);
    case 2:
        return new ThreeVoiceCounterpoint(cf, spList, v_type,
            costModel, imp, bm, objMode);
    case 3:
        return new FourVoiceCounterpoint(cf, spList, v_type,
            costModel, imp, bm, objMode);
    default:
        throw std::invalid_argument("The number of voices you asked for is not implemented (yet).");
    }
}


bool notInt(char* argv){
    bool noInt = false;
    for(int i = 0; i < strlen(argv); i++){
        if(!isdigit(argv[i])){
            noInt = true;
            break;
        }
    }
    return noInt;
}

bool has_solution(CounterpointProblem* problem) {
    BAB<CounterpointProblem> e(problem);
    if (e.next()) {
        // cout << problem->getCantusFirmus()->getHIntervals() << endl;
        // cout << problem->getCounterpoint_1()->getFirstSpeciesHIntervals() << endl;
        // cout << problem->getCounterpoint_2()->getFirstSpeciesHIntervals() << endl;
        return true;
    }
    return false;
}
