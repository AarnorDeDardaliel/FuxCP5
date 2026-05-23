// 
// Created by Luc Cleenewerk and Diego de Patoul. Modified by Bryce Burignat
//

#include "../../headers/Tests/fuxTest.hpp"
#include "../../headers/Tests/figureTests.hpp"
#include "../../headers/GenerationsAPI.hpp"

using namespace Gecode;
using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Warning :no test type, number, or 'figs' as argument. Doing default API check." << std::endl;
    }
    if (consSize != constraintNames.size()) {
        cout << "Error: constraintNames size does not match the number of constraints." << endl;
        cout << "consSize = " << consSize << endl;
        cout << "constraintNames.size() = " << constraintNames.size() << endl;
        return 1;
    }

    if(argc==1){ // QUICK TEST on API
        cout << "------------- API TEST -------------" << endl;

        GenerationInput gi;
        gi.cf_notes = {60,   62,   65,   64,   67,   65,   64,   62,   60}; // The cantus firmus notes
        gi.species = {THIRD_SPECIES, FIRST_SPECIES};
        gi.v_type = {2 , 1};
        gi.verbose = true;
        gi.stagnation_ms = 0;
        gi.output_subdir = "bryce_gen";
        gi.timeout_ms = 2000;

        GenerationResult result = generate_counterpoint(gi);
    } else if(argc==2){ // FIGURE / FUX TESTS
        string arg1 = argv[1];
        char* str = argv[1];
        if (arg1 == "figs") {
            FigureTests figureTests;
            if (argc > 2) {
                string arg2 = argv[2];
                if (arg2 == "2v") {
                    figureTests.run_twoVoice_tests();
                } else if (arg2 == "3v") {
                    figureTests.run_threeVoice_tests();
                } else if (arg2 == "4v") {
                    figureTests.run_fourVoice_tests();
                } else if (arg2 == "4sp") {
                    figureTests.run_fourthSpecies_tests();
                } else if (arg2 == "5sp") {
                    figureTests.run_fifthSpecies_tests();
                } else if (arg2 == "MUS") {
                    figureTests.MUSTest();
                } else if (arg2 == "quick") {
                    figureTests.quickTest();
                }
                else {
                    std::cout << "Invalid argument: " << arg2 << std::endl;
                    return 1;
                }
            } else {
                figureTests.run_all_tests();
            }
            return 0;
        } else if(!notInt(str)){ // Run test targetting a figure example
            FuxTest* test = new FuxTest(atoi(argv[1]));
            vector<Species> species = test->getSpList();
            //la do si re do mi fa mi re do si la
            //57 60 59 62 60 64 65 64 62 60 59 57
            vector<int> cantusFirmus = test->getCf();
            
            int size = cantusFirmus.size();
            vector<int> v_type = test->getVType();

            vector<int> melodic_params = {0, 1, 1, 576, 2, 2, 2, 1};
            //borrow, h-5th, h-octave, succ, variety, triad, direct move, penult rule check
            vector<int> general_params = {4, 1, 1, 2, 2, 2, 8, 1};

            //penult sixth, non-ciambata, con m after skip, h triad 3rd species, m2 eq zero, no syncopation, pref species slider
            vector<int> specific_params = {8 , 4 , 0 , 2 , 1 , 8 , 50};

            vector<int> importance = {8,7,5,2,9,3,14,12,6,11,4,10,1,13};

            int borrowMode = test->getBMode();

            // create a new problem
            // auto* problem = new TwoVoiceCounterpoint(cantusFirmus, species[0], C, lower_bound_domain, upper_bound_domain);
            auto* problem = create_problem(cantusFirmus, species, v_type, melodic_params, general_params, specific_params,
                importance, borrowMode);
            //cout << problem->to_string() << endl;
            // create a new search engine
            if(atoi(argv[1])==125){
                cout << "HERE" << endl;
                for(int j = 0; j < problem->getSize(); j++){
                    if(j!=10){
                        rel(problem->getHome(), problem->getSolutionArray()[j], IRT_EQ, test->getCp()[j]);
                    }
                }
            }
            else{
                if(species[0]==FIRST_SPECIES || species[0]==THIRD_SPECIES){
                    for(int j = 0; j < problem->getSize(); j++){
                        rel(problem->getHome(), problem->getSolutionArray()[j], IRT_EQ, test->getCp()[j]);
                    }
                } else if(species[0]==SECOND_SPECIES || species[0]==FOURTH_SPECIES){
                    for(int j = 1; j < problem->getSize(); j++){
                        rel(problem->getHome(), problem->getSolutionArray()[j], IRT_EQ, test->getCp()[j]);
                    }
                }
            }

            BAB<CounterpointProblem> e(problem);

            int nb_sol = 0;
            while(CounterpointProblem* pb = e.next()){
                nb_sol++;
                
                // cout << int_vector_to_string(cantusFirmus) << endl;
                cout << "Solution " << nb_sol << ": " << endl;
                cout << pb->to_string() << endl;
                delete pb;
                if (nb_sol >= 1)
                    break;
            }
            if(nb_sol!=1){
                cout << "This test did not pass. An error was found!" << endl;
            } else {
                cout << "This test passed successfully!" << endl;
            }
        } else { // Run test targetting a specific constraint
            FuxTest* test = new FuxTest(argv[1]); // FUXTESTS
        }
    } else if(argc==3){ // Run test targetting a figure example by fixing the fisrt n measures
        FuxTest* test = new FuxTest(atoi(argv[1]), atoi(argv[2]));
        vector<Species> species = test->getSpList();
        //la do si re do mi fa mi re do si la
        //57 60 59 62 60 64 65 64 62 60 59 57
        vector<int> cantusFirmus = test->getCf();
        
        int size = cantusFirmus.size();
        vector<int> v_type = test->getVType();

        vector<int> melodic_params = {0, 1, 1, 576, 2, 2, 2, 1};
        //borrow, h-5th, h-octave, succ, variety, triad, direct move, penult rule check
        vector<int> general_params = {4, 1, 1, 2, 2, 2, 8, 1};

        //penult sixth, non-ciambata, con m after skip, h triad 3rd species, m2 eq zero, no syncopation, pref species slider
        vector<int> specific_params = {8 , 4 , 0 , 2 , 1 , 8 , 50};

        vector<int> importance = {8,7,5,2,9,3,14,12,6,11,4,10,1,13};

        int borrowMode = test->getBMode();

        // create a new problem
        // auto* problem = new TwoVoiceCounterpoint(cantusFirmus, species[0], C, lower_bound_domain, upper_bound_domain);
        auto* problem = create_problem(cantusFirmus, species, v_type, melodic_params, general_params, specific_params,
            importance, borrowMode);
        //cout << problem->to_string() << endl;
        // create a new search engine

        if(atoi(argv[1])==125){
            for(int j = 0; j < problem->getSize(); j++){
                if(j!=10){
                    rel(problem->getHome(), problem->getSolutionArray()[j], IRT_EQ, test->getCp()[j]);
                }
            }
        }
        else if(atoi(argv[1])==74){
            for(int j = 0; j < test->getIdx(); j++){
                if(j!=0){
                    rel(problem->getHome(), problem->getSolutionArray()[j], IRT_EQ, test->getCp()[j]);
                }
            }
        }
        else if(atoi(argv[1])==82){
            vector<int> speciesArray = {-1,-1,3,-1,3,2,2,2,2,2,2,2,2,2,3,-1,3,2,2,2,2,2,3,-1,3,-1,3,-1,3,2,3,-1,3,2,3,-1,3,-1,3,-1,3};
            for(int j = 0; j < test->getIdx(); j++){
                if(j>1){
                    rel(problem->getHome(), problem->getSolutionArray()[j], IRT_EQ, test->getCp()[j]);
                    //rel(problem->getHome(), problem->get_species_array_5sp(0)[j], IRT_EQ, test->getCp()[j]);
                }
            }
        }
        else{
            if(species[0]==FIRST_SPECIES || species[0]==THIRD_SPECIES || species[0]==FOURTH_SPECIES){
                for(int j = 0; j < test->getIdx(); j++){
                    rel(problem->getHome(), problem->getSolutionArray()[j], IRT_EQ, test->getCp()[j]);
                }
            } else if(species[0]==SECOND_SPECIES){
                for(int j = 1; j < test->getIdx(); j++){
                    rel(problem->getHome(), problem->getSolutionArray()[j], IRT_EQ, test->getCp()[j]);
                }
            }
        }

        BAB<CounterpointProblem> e(problem);

        int nb_sol = 0;
        while(CounterpointProblem* pb = e.next()){
            nb_sol++;
            cout << "Solution " << nb_sol << ": " << endl;
            cout << pb->to_string() << endl;
            // cout << int_vector_to_string(cantusFirmus) << endl;

            delete pb;
            if (nb_sol >= 1)
                break;
        }
        cout << "No (more) solutions." << endl;
    } else {
        throw std::invalid_argument("Wrong amount of arguments!");
    }
    
    return 0;
}