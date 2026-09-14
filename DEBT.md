# DEBT.md

**Purpose:** Inter-sprint ledger of debts — bugs, nitpicks, friction observed during usage. Drained by sprints via `/pay` (COUNSELOR planning) and `/log` (hygiene drain). **JRENG = paid in full, cash. No triage.**

**Format:** Each entry uses **O / D / E** articulation — Observation, Divergence, Expectation. IDs are UTC timestamps (`DEBT-YYYYMMDDTHHMMSS`). Newest entries at top. Add via `carol debt add`.

**Lifecycle:** Created lazily on first `carol debt add`. Entries appended via interactive prompt. Entries removed by `carol debt clear <id>` (called by `/log` hygiene step after SPRINT-LOG receipt is written). Survives `carol reset` — debts persist across protocol resets.

---


## DEBT-20260909T120000

**Observation:** Windows Debug has no static-debug OpenSSL archive
**Divergence:** `kuassa_aquatic_prime/` ships `libcrypto64MT.lib` only; `BuildSetup.cmake:207` selects `/MTd` in Debug, so a Windows Debug link hits LNK2038. Never surfaced because the Windows matrix was Release-only.
**Expectation:** Build `libcrypto64MTd.lib` from OpenSSL 1.1.1l (`opensslv.h:43`, 24 Aug 2021), `VC-WIN64A no-shared` with `/MTd`, beside the MT archive — `cmake.cast` already points `IMPORTED_LOCATION_DEBUG` at that path. Precedent: `kuassa_vulkan/lib/build.bat` builds the /MT and /MTd shaderc and spirv-cross archives.

---


## DEBT-20260831T021425

**Observation:** plugin_bootstrap conformance owed
**Divergence:** JFS must build with JAM
**Expectation:** GUI only (including settings); ProcessorChain may stay stubs

---





