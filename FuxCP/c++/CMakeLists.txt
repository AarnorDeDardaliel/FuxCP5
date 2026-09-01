cmake_minimum_required(VERSION 3.10)

# --- Gecode (installé en framework sur macOS) ---

# --- Gecode : sélection selon l'architecture ciblée ---
if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    message(STATUS "Architecture arm64 détectée - Gecode arm64 (compilé pour Arm)")
    set(GECODE_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/../../../gecode-arm64/gecode")
else()
    message(STATUS "Architecture x86_64 détectée - Gecode.framework local")
    find_library(GECODE_LIBRARY
            NAMES Gecode
            PATHS /Library/Frameworks
            NO_DEFAULT_PATH)
    if(NOT GECODE_LIBRARY)
        message(FATAL_ERROR "Gecode.framework introuvable dans /Library/Frameworks. "
                "Vérifie l'installation de Gecode avant de continuer.")
    endif()
endif()
message(STATUS "Gecode library: ${GECODE_LIBRARY}")

# --- Sources FuxCP (reprises du Makefile, sans main.cpp/Experiments.cpp/Tests) ---
set(FUXCP_SOURCES
        src/Utilities.cpp
        src/Voice.cpp
        src/Stratum.cpp
        src/CounterpointProblems/CounterpointUtils.cpp
        src/ConfigLoader.cpp
        src/CounterpointProblems/CounterpointProblem.cpp
        src/Parts/Part.cpp
        src/Parts/FirstSpeciesCounterpoint.cpp
        src/Parts/SecondSpeciesCounterpoint.cpp
        src/Parts/ThirdSpeciesCounterpoint.cpp
        src/Parts/FourthSpeciesCounterpoint.cpp
        src/Parts/FifthSpeciesCounterpoint.cpp
        src/Parts/CantusFirmus.cpp
        src/CounterpointProblems/TwoVoiceCounterpoint.cpp
        src/CounterpointProblems/ThreeVoiceCounterpoint.cpp
        src/CounterpointProblems/FourVoiceCounterpoint.cpp
        src/constraints.cpp
        src/Midi.cpp
        src/CostModel.cpp
)

add_library(FuxCPLib STATIC ${FUXCP_SOURCES})

set_target_properties(FuxCPLib PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
)

target_include_directories(FuxCPLib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/headers/CounterpointProblems
        ${CMAKE_CURRENT_SOURCE_DIR}/headers/Parts
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/CounterpointProblems
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Parts
        /Library/Frameworks/gecode.framework/Headers
)

target_link_libraries(FuxCPLib PUBLIC ${GECODE_LIBRARY})
target_link_options(FuxCPLib PUBLIC "-F/Library/Frameworks")
