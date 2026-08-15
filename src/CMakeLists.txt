add_library(nf_core STATIC
    isa/decoder.cpp
    core/interpreter.cpp
    core/register_file.cpp
    core/alu.cpp
    pipeline/pipeline.cpp
    mem/memory.cpp
    loader/elf_loader.cpp
    sim/simulator.cpp
)

target_include_directories(nf_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
