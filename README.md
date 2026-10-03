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

# Milestones

## Done

1. Functional-only RV32IM interpreter, verified against 
	1. `riscv-tests`
	2. `spike`

## Working On

2. Bolt on the 5-stage pipeline _assuming no hazards_ (just get the plumbing/registers-between-stages right)
- handle hazards (control, data) via stalls (no forwarding)

## Remaining

3. Add FU timing: multi-cycle FU timing (opLatency, issLatency, pipelined)
4. Superscalar: 2 wide + contention
5. Forwarding via Scoreboard
6. Add always-taken prediction + misprediction flush
7. Add perf counters + Python analysis
8. Stretch: caches, 2-bit/gshare predictor



# Build

## Simulator

```Sh
# Functional (no timing)
cmake --build build --target nf-functional 

# Pipeline (with timing)
cmake --build build --target nf-pipeline 

```

## ISA Tests

Must be compiled first.
```sh
./tests/isa-compliance/run-riscv-tests.sh
```


## Unit Tests

```Sh
# Build
cmake --build build --target nf-unit-tests

# Run tests
./build/bin/nf-unit-tests
# or more verbose
ctest --test-dir build --output-on-failure
```

# Project Structure

src/core 	mutable architectural state
src/isa		stateless decode logic

