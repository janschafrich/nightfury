add_library(nf-core STATIC
    isa/decoder.cpp
    core/interpreter.cpp
    core/register_file.cpp
    core/csr_file.cpp
    core/alu.cpp
    core/csr_update.cpp
    pipeline/pipeline.cpp
    pipeline/fetch.cpp
    pipeline/decode.cpp
    pipeline/execute.cpp
    pipeline/memory.cpp
    pipeline/writeback.cpp
    mem/memory.cpp
    loader/elf_loader.cpp
    sim/simulator.cpp
)

target_include_directories(nf-core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
