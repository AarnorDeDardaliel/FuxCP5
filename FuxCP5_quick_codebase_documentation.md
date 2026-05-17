# FuxCP5 Quick Codebase Documentation

## Scope of this document

This document is a **developer-oriented overview** of the current FuxCP5 C++ codebase.

Its goal is not to document every single function line by line.  
Its goal is to help a new developer quickly answer the following questions:

1. What does the project do?
2. Which files are responsible for what?
3. How are the musical concepts represented in code?
4. How is a counterpoint problem built and solved?
5. Where should a new rule, cost, or generation feature be added?

---

# 1. Broad Vision of the codebase 


## Global implementation files:

- `Generations` : **The orchestration layer for actual generation**. It handles generation cases, presets and parameter loading, benchmark control, timeout / stagnation stop logic, CSV output, MIDI export, and practical search launching.

- `Utilities` : **The shared toolbox of the whole project**. It defines global enums, constants, scales, interval/chord data, search-heuristic names, note / mode / voice naming helpers, scale-detection helpers for a cantus firmus, vector-set operations (union, intersection, difference), string/debug conversions for Gecode variables and arrays, MIDI/note naming helpers, and formatting helpers for solver statistics and logs.

- `Configloader` : The helper to load configurations of counterpoints presets, for keeping track of sensitivity testing of launching predetermined campaigns of generations.

- `Midi` : The helper to generated MIDI files from solver's outputs.


## `CounterpointProblems/`
Implementations of the high-level problem classes and construction helpers.

- `CounterpointProblem` : **The root of the Constrains Programming Problem**. It stores and coordinates the cantus firmus, the generated parts, the strata, local and global costs, final optimization arrays, string/debug output, and the global objective returned by `cost()`.

- `Two-`, `Three-`, and `FourVoiceCounterpoint` : These classes decide how many generated counterpoints exist, which species are instantiated, how many strata are needed, which inter-voice rules are posted, and how local costs are unified into the final objective.

- `CounterpointUtils` : This is the main dispatch / factory layer. It is responsible for creating the correct species object from a species identifier, and the correct global problem class from the number of voices.


## `Parts/`
Implementations of the more precise problem classes.

- `Voice`: **The most generic musical-line abstraction in the project**. It owns the lowest-level variable arrays that are reused everywhere, such as `notes`, `h_intervals`, `m_intervals_brut` and `motions`

- `Part` : **A musically meaningful generated line**. Extends `Voice` and adds species identity, cost arrays, cost names and combined-cost names, helper arrays used by the various species, common getters for branching notes, melodic intervals, harmonic intervals, and costs.

- `CantusFirmus` : The fixed musical line given as input. Extends `Part` inside the problem, but it is not searched the same way as generated counterpoints.

- Species classes (`FirstSpeciesCounterpoint` to `FifthSpeciesCounterpoint`) : Responsible for defining species-specific arrays, rules, views of branching notes, and melodic and harmonic helper arrays. Extends `Part`, so the common voice/cost machinery is centralized while the rhythmic and rule-specific logic stays local to each species.

- `Stratum` : **Vertical layer of the current sonority**. Extends `Voice`. In multi-voice counterpoint, the code distinguishes the lowest stratum, intermediate upper strata, and the highest stratum.

- `constrains` : **The central reusable constrains library** of the project. It contains all the harmonic, melodic, motion / progression, and stratum-level rules. This is the main file to edit when adding or correcting a rule.


## `Tests/`
Test entrypoint and test suites:

- `main` : **The command-line test entrypoint**. It checks the global constraint registry consistency, dispatches either to FigureTests (figs, filtered by number of voices / species / MUS / quick modes) or to the lower-level FuxTest routines, and also still contains a small manual experimentation block for directly constructing and solving a problem from hardcoded parameters.

- `fuxTest` : **The low-level testing framework** for individual constraints and targeted configurations. It initializes standard test parameters, builds problems from small hand-crafted setups, dispatches named tests such as 1H1, 2H2, etc., and is mainly used to check whether specific rules, species configurations, or debugging scenarios behave correctly.

- `figureTests`: **The high-level regression / validation suite** based on Fux’s reference examples. It stores complete figure configurations (cantus firmus, expected counterpoint, species layout, parameters), rebuilds the corresponding problems, checks whether known examples are accepted, can enumerate solutions for a figure, and also includes MUS / unsat-diagnosis helpers to isolate conflicting constraints.

---

# 2. Core concepts
A large part of the difficulty of FuxCP comes from the fact that the same musical idea is represented differently depending on the species, and that many constraints rely on selecting the **correct structural time indices**. We will explain the key parts in this section.


## 2.1 The internal time grid
Most of the code follows a **4-slots-per-measure logic**, through the `notes` array of the counterpoints. Beware that the cantus firmus is an exception : its notes array has only 1 slot per measure (*Should be changed in a future update, for consistency !*).

Typical structural positions are:
- `m*4` : thesis 
- `m*4+1` : intermediate slot 
- `m*4+2` : arsis 
- `m*4+3` : intermediate slot 

This does **not** mean that every species really has four independent notes per measure. It means the arrays are embedded in a common grid that makes cross-species comparisons easier.

Each species manages it its way, and have their own array of notes (`firstSpeciesNotesCp`, `secondSpeciesNotesCp`, ...) :

- First species : only thesis, size of nMeasures

- Second species : thesis + arsis, size of 2*nMeasures-1

- Third species : same than `notes`, size of 4*nMeasures-3

- Fourth species : **arsis** + thesis (in this order), size of 2*nMeasures-2

- Fifth species : hybrid logic depending on the species-array system, size of 4*nMeasures-3


### `getHIntervals()`
Stores the harmonic interval of a `Part` relative to the **lowest stratum** at each index.

Useful to know, the following are equivalent :
- `abs(p1->getHIntervals()[t] - p2->getHIntervals()[t]) % 12`
- `abs(p1->getNotes()[t] - p2->getNotes()[t]) % 12`


### Melodic interval arrays
Melodic interval arrays store the interval between **successive notes** of a single voice or part. They are used for horizontal rules such as step / leap detection, repeated notes, diminution patterns, and species-specific melodic movement constraints.

The generic ones are :

- `m_intervals_brut` : Array of successive interval between the notes of the `notes` array (size of notes.size()-1)

- `xxxxSpeciesMelodicIntervals` : Array of successive intervals between the notes of the corresponding `xxxxSpeciesNotesCp`


## 2.2 Search and optimization model

As one does in Constrains Programming, FuxCP separates two things:

- the **model**: variables, arrays, hard constraints, and costs constrains

- the **search**: how the solver explores the space of possible solutions

A valid counterpoint is obtained when all hard constraints are satisfied. A better counterpoint is obtained when the search minimizes the objective built from the cost arrays.

### Branching

Branch-and-Bound (BAB) is the optimization mode used in `Generation`. It works by finding a valid solution, then searching for better ones according to the objective.

The search must choose:
- which variable to branch on next,
- which value to try first

These **branching choices** are all defined in `Two-` `Three-` and `FourVoiceCounterpoint`. 

### Costs comparison

The usual flow of costs is :
1. local rules create cost variables,
2. these costs are grouped into arrays or families,
3. the global objective is built from them.

Then, **Gecode BAB solver** uses the output of the `cost()` function (in `CounterpointProblem`) to compare solutions, using lexicographical order.

This means:
- the first objective component has priority over all the others,
- the second matters only if the first is equal,
- and so on.

This is why, depending on the `objectiveMode`, the `cost()` function can add before the raw cost arrays the `objectiveCostSum`, an pondered sum based on the array. This ensures no cost will be allowed to explode to optimize the first one. 

This sum is computed in the `orderCosts()` function.


# 3. Generation parameters

Here is an explaination of the relevant parameters given through `gen_case` in `Generations`.

- `cf` : the cantus firmus, in midi value

- `cf_name` : the name of the cantus firmus

- `cf_scale` : the scale in which the cantus firmus is

- `n_voices` : the total number of voices desired (cantus firmus included)

- `spList` : the list of the species of the counterpoints to generate

- `v_type` : the relative height of the counterpoint, compared to the first not of the cantus firmus (each voice has a 2-octave ranged, centered on **cf[0] + v_type*6**, in MIDI value)

- `melodic_params` : array of factors for the corresponding melodic costs (unisson, m2, M2, sauts, m3, M3, P4, sup4)

- `general_params` : array of factors for the corresponding other costs (octave, P5, m6, M6, P8, mvtCommun, variete, triade)

- `specific_params` : array of factors for the corresponding specific costs (parfaite, faiblet, mvtDirect, syncope, cambiata, melodique, fux2)

- `importance` : array of the **unique** ranking of the corresponding cost categories, **from 1 to 14** (borrow, fifth, octave, succ, variety, triad, direct, motion, penult,  cambiata, triad3, m2, syncopation, melodic). 

- `borrow_mode` : if activated, notes that are not on the cantus firmus scale are authorized, in exchange for a bigger `borrow` cost

- `obj_mode` : decides the general cost objective by defining the `objectiveMode` variable. If its value is 0, the `cost()` function returns the original cost array. If not, it builds an aggregated sum and adds it at the beginning of the array. See cost() and orderCosts() functions in `CounterpointProblem` for more details.

- `timeout_ms` : the maximal duration of the total BAB search (in ms)

- `stagnation_ms` : the maximal duration of one iteration of the BAB search (in ms)

- `output_root` : the path to the global output directory (generally the 'result' folder)

- `output_subdir` : the name of an eventual subfolder



# 4. Guidelines to add things

When adding a new rule:

1. Add the core implementation in `constrains.cpp`.

2. Add the declaration in `constraints.hpp`.

3. Add activation flags / naming in `Utilities.hpp` if needed.

4. Post it from the correct constructor:
   - species constructor if the rule is local to one part/species
   - `Two/Three/FourVoiceCounterpoint.cpp` if it is an inter-voice or global rule

5. Add a minimal targeted test in `fuxTest.cpp` if relevant.

6. (Optionnal) Check whether the rule also affects cost unification, debug output or generation logs, it could uncover unintended consequences.

---

## 4.1 Constraint naming conventions

The codebase broadly follows:

- `G` → general/global rules
- `H` → harmonic rules
- `M` → melodic rules
- `P` → motion / progression / inter-voice motion rules
- stratum-prefixed rules → rules applying specifically to upper/lower strata
- `V2_*`, `V3_*`, `V4_*` → activation flags scoped to 2-, 3-, 4-voice contexts
- `SP*_` → species-specific rule flags


