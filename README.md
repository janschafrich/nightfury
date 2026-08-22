# Nightfury

A cycle accurate CPU microarchitecture simulator to learn about performance modeling.

Areas I want to explore:
- Modeling pipeline stages
- how to load test programs
- how to measure performance

Requirements:
- ISA: RV32IM
- pipelined (IF/ID/EX/MEM/WB), in order, scalar, 
- branch prediction: disabled, maybe later
- memory hierarchy: just memory for now, caches maybe later
- language: C++23 combined with Python
- C++ simulator core writes trace file
- Python reads trace file and analyzes it
- my OS: PopOS 24.04, based upon Ubuntu
- Time per week: 4 to 5 hours. 

# Milestones

1. Functional-only RV32IM interpreter, verified against 
	1. `riscv-tests`
	2. `spike`
2. Bolt on the 5-stage pipeline _assuming no hazards_ (just get the plumbing/registers-between-stages right)
3. Add hazard detection with stalling only (no forwarding) — correctness over performance
4. Add forwarding/bypassing
5. Add always-taken prediction + misprediction flush
6. Add perf counters + Python analysis
7. Stretch: caches, 2-bit/gshare predictor



# Build

## Simulator

```Sh
# Functional (no timing)
cmake --build build/debug --target nf-functional 

# Pipeline (with timing)
cmake --build build/debug --target nf-pipeline 

```

## Unit Tests

```Sh
# Build
cmake --build build/debug --target nf_unit_tests

# Run tests
./build/debug/bin/nf_unit_tests
# or more verbose
ctest --test-dir build/debug --output-on-failure
```

# Project Structure

src/core 	mutable architectural state
src/isa		stateless decode logic

