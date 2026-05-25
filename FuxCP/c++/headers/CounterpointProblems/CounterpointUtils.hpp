// 
// Created by Luc Cleenewerk and Diego de Patoul. 
// This file contains the declarations of the main functions to create problems and counterpoints.  
// 

#ifndef COUNTERPOINTUTILS_HPP
#define COUNTERPOINTUTILS_HPP

#include "../Utilities.hpp"
#include "../CostModel.hpp"
#include "TwoVoiceCounterpoint.hpp"
#include "ThreeVoiceCounterpoint.hpp"
#include "FourVoiceCounterpoint.hpp"
#include "../Parts/FirstSpeciesCounterpoint.hpp"
#include "../Parts/SecondSpeciesCounterpoint.hpp"
#include "../Parts/ThirdSpeciesCounterpoint.hpp"
#include "../Parts/FourthSpeciesCounterpoint.hpp"
#include "../Parts/FifthSpeciesCounterpoint.hpp"

using namespace std;
using namespace Gecode;


/**
 * This function creates the appropriate Part given the species of the counterpoint requested. 
 * @return a pointer to a Part object
*/
Part* create_counterpoint(Home home, int species, int nMeasures, vector<int> cantusFirmus, int lowerBound, int upperBound, Stratum* low,
    CantusFirmus* c, int v_type, vector<int> m_costs, vector<int> g_costs, vector<int> s_costs, int bm, int nV, const vector<double>& melodicShape = {}, const CostModel* costModel = nullptr,
    int voiceIndex = 0);

/**
 * This function creates the appropriate counterpoint problem given the number of counterpoints (size of the species list) requested. 
 * @return a pointer to a counterpoint problem object
*/
CounterpointProblem* create_problem(vector<int> cf, vector<Species> sp, vector<int> v_type, vector<int> m_costs, vector<int> g_costs,
    vector<int> s_costs, vector<int> imp, int bm, ObjectiveMode objMode = OBJECTIVE_LEX, const vector<double>& melodicShape = {});

// Dorian Genon : signature pour les expériences avec dynamic sliders généralisés
CounterpointProblem* create_problem(vector<int> cf, vector<Species> sp,
    vector<int> v_type,
    const CostModel& costModel, vector<int> imp, int bm,
    ObjectiveMode objMode = OBJECTIVE_LEX);

bool notInt(char* argv);

bool has_solution(CounterpointProblem* problem);



#endif