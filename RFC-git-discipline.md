# RFC: Git Isolation and Destructive-Edit Discipline

**For:** MACHINIST — CAROL framework amendment
**From:** ARCHITECT via COUNSELOR
**Status:** Ratified by ARCHITECT — implement verbatim
**Date:** 17 Aug 2026

---

## Motivating Incident

During a mechanical sigil migration, an Engineer subagent:

1. Ran an in-place `perl -pi -e 's/#([A-Za-z][A-Za-z0-9_]*)/@$1/g'` across three data
   files. In perl, `@$1` is an array dereference interpolating to the empty string —
   every matched token was **deleted**, not replaced. No dry run was performed, no
   backup was taken.
2. On discovering the damage, the agent ran `git diff`, `git show`, `git status`, and
   `git log` to "diagnose", misattributed weeks of legitimate uncommitted sprint work
   visible in the diff as new corruption, declared the files unrecoverable, and gave up
   with a recommendation to restore from git.
3. A separate Pathfinder agent had earlier run read-only git commands for the same
   reason, with the same confusion.

Both failure modes are systemic, not individual: agents are trained to treat git as
task-relevant state and as a safety net. In this environment both assumptions are
false and proven counterproductive.

## The Law

### 1. Git is unrelated to task state

- The working tree is the single source of truth. File content read via the Read tool
  is the only evidence of current state.
- Design implementations may be exhaustively long-lived and uncommitted. Work is NOT
  committed per-implementation; throwaway iterations never enter history. An old HEAD
  tells an agent nothing about the present and actively misleads.
- **No agent runs any git command — including read-only `status`, `diff`, `log`,
  `show` — for any purpose: not for diagnosis, not for verification, not for
  recovery planning.** The existing "agents never run git commands" rule is extended
  explicitly to read-only invocations.
- Sole exception: ARCHITECT explicitly asks for a git reference in the current
  session. The ask names git; nothing is inferred.

### 2. No reliance on git as a safety net

- Recovery strategies that assume a commit exists ("restore from HEAD", "checkout the
  file") are forbidden in agent reasoning and in options presented to ARCHITECT.
- An agent that damages a file owns the repair from working-tree evidence, structure,
  and deterministic reconstruction — or reports the exact damage mechanism and waits.
  "Restore from git" never appears as an agent-proposed remedy.

### 3. Destructive scripted edits: dry run + backup, always

Any scripted or mechanical in-place modification (sed/perl/awk/python, bulk Edit
loops) over project files MUST:

1. **Dry-run first** — print the proposed transformation to stdout (matched lines
   before/after, or a generated diff preview) and verify expected match counts BEFORE
   any write.
2. **Back up first** — copy each target file (e.g. `cp file file.bak` or a temp-dir
   copy) before the in-place write. The backup is deleted only after post-edit
   verification passes.
3. **Verify after** — post-edit counts must reconcile with the dry-run prediction
   (replacements made == matches predicted; zero unexpected residue). A mismatch
   means restore from the backup immediately and report.
4. **Never chain destructive steps** — one transformation, one verification, before
   the next.

Overconfidence is the named threat: a script that "cannot fail" still gets the full
protocol.

## Implementation (MACHINIST)

1. **~/.carol/CAROL.md — Git section:** extend "AGENTS NEVER RUN GIT COMMANDS" to
   state that read-only git commands (`status`, `diff`, `log`, `show`, any other) are
   equally forbidden, for all agents including subagents, for all purposes including
   diagnosis and verification; working tree via Read is the only state evidence; git
   reference requires ARCHITECT's explicit ask. Keep the existing sole
   commit-on-command exception for MACHINIST unchanged.
2. **~/.carol/CAROL.md — new or existing discipline section:** add the
   destructive-edit protocol (dry run, backup, verify, single-step) as a mandatory
   rule for all agents that modify files.
3. **Specialist definitions (locate via Pathfinder — agent definition files for
   Engineer, Pathfinder, Auditor, Librarian):** embed both rules so every subagent
   carries them in its own brief, independent of the delegating primary's prompt:
   - "You never run git commands, including read-only ones. The working tree is the
     only truth; verify via Read."
   - Engineer additionally: "Scripted in-place edits require dry run, backup, and
     post-verification before the backup is removed."
4. **Version bump** CAROL.md and note the amendment in its changelog line.

## Non-Goals

- No change to ARCHITECT's own git workflow.
- No change to MACHINIST's commit-on-explicit-command exception.
- No tooling/hooks — this is contract text; enforcement is the Violation Protocol.
