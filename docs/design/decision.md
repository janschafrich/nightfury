# Register File

- use class to model state
- use read and write methods to ease adding timing information later
- skip out of bounds access handling since the decoder guarantess that rs1, rs2, rd will always be in the range [0:31].


# ALU

- implement as pure function to avoid any state
- treat it like combinational logic
- convert from ArchOps to AluOps to take advantage off similarity of immediate and non-immediate operations (ADDI, ADD)


# Memory

- keep memory dumb: it just moves bytes, independent of ISA
- sign extension is handled at caller (interpreter)


# Decoder

- Eager decode: to prevent decode and pipeline logic from drifting apart, everything needed later is derived at decode and then frozen
- Exceptions to the rule:
    - to support control hazard resolution via stalling of IF, IF must know whether it fetched a branch/jump
    - for this purpose IF will inspect the opcode of a fetched instruction for branch/jal/jalr
- Learning: this apparently how Gem5 does it, 
- In contrast:
- Hardware goes for lazy decoding to save on area (wiring): each stage only decodes the information it needs, the rest travels encoded down the pipe


# Loader

- check executable (elf magic word, executable type, riscv isa)
- extract symbols from riscv-test suite instead of hard coding them


# Timed / Pipelined Simulation

Latency is a property of units, declared centrally, injected at construction, keyed by eagerly-decoded op class; functional code is timing-free.

## Latency Sources:

- Functional Units (is allocated at issue time to execute an op and produce a data result): opLatency, issLatency
- Wire/Latch delays (mispredict penalty, etc): 
- Memory: when caches arrive
- Predictor: when predictor arrives


## Intra cycle evaluation order: 

When evaluating the instruction from first stage to last stage, one has to ensure that the consumer stage always evaluates this-cycles producer-stage result (q), and not the just determined next-cycle producer-stage result (d).

This simulator handles this by using a two phase tick: Consuming the input, and producing the input happens to two phases:
First, the next state (q) for each stage is evaluated from a snapshot of this cycles input (d), only then it is committed (becomes observable for the other stages)
This eases handling of hazard resolution (stalls/backpressure/replay), since each stage control logic sees the snapshot from the same cycle. 

Considered alternative: Reverse-order (WB->MEM->EX->ID->IF): Each stage consumes its inputs and produces its outputs in one pass. Initially simple, until complex hazard resolution required.

## Commit Point:

Once speculation arrives, execution and commit must be distinguishable
Options:
1. Execute in EX and Commit in WB: 
- 

2. Execute-early, commit-at-retire
- used by ChampSim
 
3. Speculative Execute with Repair:
- better handling of misprediction-resolution (i.e. branch squashing)
- relies an renamed registers
- used by Gem5 

Chose 1, execute in EX and commit WB 


## Pipeline Configuration Location

Options:
1. Single latency struct: simpler, scales worse
2. per module latency struct: remains clean as the simulator scales

Chose 1., single pipeline config, the simulator is not intended to grow very complex, so single, nested struct for all latencies, instead of per module config structs should be fine.


# Simulation Engine

1. Plain cycle loop: jump from cycle to cycle
2. Event driven: jump from event to event, allows to skip cycles. Interesting when modeling sytems with Caches, dozens of IO devices, DVFS

Chose 1, as I want to learn micro-architectural modeling and not simulation infrastructure


# Pipeline Location

- registers are read in ID stage
- outcome and target is determined in EX stage
- when evaluated a halt, execution stops and pipeline contents are discarded 


# Control Hazard Resolution
- stall: IF inserts pipeline bubbles, by generates non-valid packets 




