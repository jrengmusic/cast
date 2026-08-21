# PLAN: Unify Definition/Scope render; converge jam/ byte-identical

**RFC:** none — objective from ARCHITECT prompts
**Date:** 2026-08-21 (revision 3)
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE / JAM; cast engine (`jam::Document`, `jam::Strings`, `jam::Format`, `jam::Function::Map`)

## Context

cast generates jam's headers. Correctness = **byte-identical** to `jam/generated/` (oracle), proven by `jam/diff/` vs `jam/generated/` + **fixpoint** (2nd run = zero changes).

## The Design

**One mechanism: `jam::Strings::joinIntoString(separator)`.**

Two templates, two axes, one join:

| Template | Axis | Jack | Structure token | Expansion |
|----------|------|------|-----------------|-----------|
| **Scope.cast** | vertical | `:::line:::` | `- line: @text:break:line` | Source rows expanded through template, joined by resolved separator |
| **Definition.cast** | horizontal | `:::list:::` | `- list: @text:break:comma` | Source row columns expanded, joined by resolved separator |

**Scope.cast:**
```
:::files:::

:::keyword::: :::type::: :::name:::
{
:::prologue:::

:::line:::

:::epilogue:::
}:::terminator:::
```

**Definition.cast:**
```
:::keyword::: :::type::: :::name:toCamel::: :::open::: :::list::: :::close::::::terminator::: :::doxygen:toComment:::
```

**Data dictates logic.** The engine holds no hardcoded token names. Structure cell tokens (`- line:`, `- list:`) match template placeholders by name. The token value resolves to a separator from the `## break` table. The engine discovers expansion jacks dynamically — a template placeholder whose name matches a structure cell token that resolves to a `.cast` template path triggers per-row expansion; a token that resolves to a separator text triggers horizontal join.

**No hardcoded tokens on engine.** Rules = token match template. The engine reads structure cell tokens, matches them against template placeholders, and dispatches accordingly.

**Separators** come from `## break` table in `text.md`:
- `line` = `//==============================================================================` (vertical section rule)
- `comma` = `, ` (horizontal inline join)
- `semicolon` = `; ` (horizontal inline join)

**`:::open:::` / `:::close:::`** in Definition.cast replace hardcoded `{ }`. Structure cell provides `- open: {` / `- close: }` for declarations. Enum entries use `- open: =` (no close). Empty open/close = elided.

## Resolved Design Decisions

| Decision | Resolution |
|----------|-----------|
| Sigil | `@` exclusively |
| Jack names | `:::line:::` (Scope vertical body), `:::list:::` (Definition horizontal body) |
| Open/close | `:::open:::` / `:::close:::` in Definition.cast; data-driven from structure cell |
| Maps decompose | Scope (object) + Definition (entry); specialized templates only for genuinely irregular shapes |
| Bimap | Scope (struct) + Definition (map-init via `:::line:::`) + enum entries (Scope with `:::line:::`, Definition with `- open: =`) + epilogue (methods, specialized template) |
| LookupTable | Specialized wrapper template with `:::line:::` body for Definition entries |
| HashMap | Scope.cast with type alias (backtick-wrapped in index to avoid `<`/`>` hazard) |

## Engine Rewrite

The engine is rewritten from scratch with the correct pattern. Not patched. `joinIntoString(separator)` everywhere.

### Expansion mechanism

When the engine encounters a template placeholder `:::name:::`:

1. **Token match:** Look up `name` in the structure cell's tokens (`- name: value`)
2. **Template expansion:** If the token value resolves to a `.cast` template path → expand source rows through that template, join by `joinIntoString(separator)` where separator comes from the corresponding `- line:` or `- list:` token
3. **Separator join:** If the token value resolves to a separator text (from `## break`) → use as the join string for items
4. **Cell resolution:** Otherwise → resolve as a cell value (existing getCell mechanism)

### Writer sibling join

Writer.h `getFile` joins sibling output rows sharing a file. The separator column in the output table drives this join — same mechanism: `joinIntoString(separator)`.

### Elision

Empty-token whitespace rule preserved: empty placeholder + preceding space → both elided. The assembly+classification mechanism handles this.

## Key Files

### Templates (current state)
- `jam/cast/template/Scope.cast` — `:::line:::` (was `:::body:::`)
- `jam/cast/template/Definition.cast` — `:::open::: :::list::: :::close:::` (was `{ :::list::: }`)
- `jam/cast/template/Bimap.cast` — specialized wrapper with `:::line:::` body
- `jam/cast/template/LookupTable.cast` — specialized wrapper with `:::line:::` body
- `jam/cast/template/Chars.cast` — epilogue (special + isNumeric)
- `jam/cast/template/Instance.cast` — SharedInstance declarations
- `jam/cast/template/Include.cast` — include directives

### Engine (rewrite)
- `Source/TemplateDocument.h` — template parsing (keep), expansion mechanism (rewrite), elision (keep)
- `Source/Writer.h` — file assembly with separator join (rewrite getFile)

### Data (current state)
- `jam/cast/CAST.md` — all output rows use `- line: @definition` (was `- body: @definition`)
- `jam/cast/tables/text.md` — `## break` table: line, comma, semicolon
- `cast/tables/template.md` — template token type: text, placeholder (regions removed)

## Steps

### Step 1: Engine rewrite — expansion mechanism ✅ partial

Rewrite `Source/TemplateDocument.h` expansion to implement: token-match → template expansion or separator join → `joinIntoString(separator)`. No hardcoded token names.

### Step 2: Engine rewrite — Writer sibling join ✅ partial

Rewrite `Source/Writer.h` `getFile` to use separator column with `joinIntoString`.

### Step 3: Converge all files

ARCHITECT builds + runs `./cast jam/cast/CAST.md`. Diff every `jam/diff/<f>` vs `jam/generated/<f>`. Fix residuals iteratively until byte-identical.

### Step 4: Fixpoint

2nd `./cast` = zero changes.

### Step 5: Auditor sweep

Full-sprint audit vs MANIFESTO/NAMES/CODING + locked plan decisions.

## Verification

Per step: ARCHITECT builds + runs. `cmp -s jam/diff/<f> jam/generated/<f>` byte-identical is the gate. Final: all files identical AND fixpoint.
