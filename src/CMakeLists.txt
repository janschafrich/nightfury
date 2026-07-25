add_library(nf_core STATIC
    isa/decoder.cpp
    core/interpreter.cpp
    pipeline/pipeline.cpp
    mem/memory.cpp
    sim/simulator.cpp
)

target_include_directories(nf_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
