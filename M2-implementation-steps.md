1. include/nf/pipeline/latch.hpp — define the template methods in-header (currently declared-only, won't link). Unit test d/q isolation and the Commit edge.

2. include/nf/pipeline/payloads.hpp
- MemoryPacket: add wb_value — muxed in MEM (ALU result vs. load data), sign extension done in MEM (memory stays dumb, caller extends).
- ExecutePacket: generalize branch_taken/branch_target → is_control + resolved next_pc; carry rd/wb_value for non-load instructions; add CSR payload fields (old value for csrrs-style read, new value) so CSR commit lands in WB.
- Bubbles: default-constructed packets with valid=false.

3. Stages in src/pipeline/ — each an Evaluate(in_latch, out_latch) reading only latch q:
- FetchStage (owns fetch PC): on stall, hold PC + inject bubble into IF/ID; else fetch word, drive PC+4.
- DecodeStage: decode (eager, via existing Decoder), read rs1/rs2 from the shared regfile.
- ExecuteStage: ALU compute, branch/jump resolution (is_control, next_pc), CSR old-value read.
- MemoryStage: load (sign-extend here) / store, mux wb_value, detect store-to-tohost → halt flag.
- WritebackStage: regfile write (rd≠0), CSR write, ecall/ebreak/mret effects.

4. Pipeline class — owns the four latches + stages + control, per the two-phase tick: compute stall/redirect/halt from this-cycle q snapshots → Evaluate IF→WB (WB before halt check, so the older in-flight instruction retires) → Commit() all latches. Control stall: opcode peek in IF/ID (branch/jal/jalr) or control op in ID/EX. Data stall: ID's rs1/rs2 vs. rd of valid packets in ID/EX, EX/MEM, and MEM/WB (rd≠0). Gem5 comparison point for decision.md: textbook P&H squashes on taken branches; you stall-until-resolve — simpler control, 2-cycle penalty on every branch/jump, replaced by M6's predict+flush.

5. Simulator refactor — hoist RegisterFile + CsrFile to Simulator (ThreadContext pattern); Interpreter takes references; add pipelined run path; wire apps/nf-pipeline/main.cpp to load and run an ELF.

6. Verification
- tests/unit/test_pipeline.cpp: latch semantics, per-stage packet tests, stall/bubble/redirect cases (branch in ID → IF holds; load-use → 3 bubbles).
- Lockstep vs. interpreter on riscv-tests: compare tohost value and final regfile state.
- Extend run-riscv-tests.sh or add a variant targeting nf-pipeline — full compliance on the pipelined model at M2.
- Cycle-count sanity: with stalls, CPI ≥ 1; no-hazard stretches ≈ 1 CPI + 4-cycle fill.

7. Docs — decision.md: stall-until-resolve + IF pre-decode peek, halt-discards-pipe semantics, hoisted architectural state; README: re-split milestones (M2 = pipeline + all-stall hazards; M3 = multi-cycle FU timing).
Ordering note (per the sub-step agreement): land steps 1–3 + straight-line unit tests first, then the stall logic (step 4), then compliance — so plumbing bugs and hazard bugs are debugged separately.
Workflow reminders when we implement: the Simulator/Interpreter refactor touches src/core → re-run ISA compliance; ctest --preset debug before declaring done.
Ready to start with step 1 (the latch) whenever you switch me out of plan mode.