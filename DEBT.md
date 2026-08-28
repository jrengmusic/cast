# DEBT.md

**Purpose:** Inter-sprint ledger of debts — bugs, nitpicks, friction observed during usage. Drained by sprints via `/pay` (COUNSELOR planning) and `/log` (hygiene drain). **JRENG = paid in full, cash. No triage.**

**Format:** Each entry uses **O / D / E** articulation — Observation, Divergence, Expectation. IDs are UTC timestamps (`DEBT-YYYYMMDDTHHMMSS`). Newest entries at top. Add via `carol debt add`.

**Lifecycle:** Created lazily on first `carol debt add`. Entries appended via interactive prompt. Entries removed by `carol debt clear <id>` (called by `/log` hygiene step after SPRINT-LOG receipt is written). Survives `carol reset` — debts persist across protocol resets.

---

## DEBT-20260828T205932

**Observation:** Source/generated/ and cast own manifest/tables have not been swept for convergence under current SPEC.
**Divergence:** Current SPEC requires diff == generated (fixpoint), same law applied in the jam sweep; CAST project has not received the equivalent pass.
**Expectation:** Source/generated/ and cast manifest/tables converge under current SPEC: diff == generated, same law as the jam sweep.

---




