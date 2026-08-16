include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

add_executable(nf_unit_tests
    test_decoder.cpp
    test_register_file.cpp
    test_alu.cpp
    test_memory.cpp
    test_elf_loader.cpp
    test_interpreter.cpp
)
target_link_libraries(nf_unit_tests PRIVATE nf_core Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(nf_unit_tests)
