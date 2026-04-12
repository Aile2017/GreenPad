# Search System Concerns — Analysis

Analysis of the search-related concerns listed in `懸念.md` §3.
The concern is that "backward search and replacement expansion are handled in a
scattered way" across the three search engines (plain, built-in regex, PCRE2).

---

## Out of scope (by design)

The following items were examined but are intentional design decisions, not problems:

- **`RSearch` does not implement capture groups** — Capture groups are outside the
  scope of the built-in NFA engine by design. `\1`–`\9` references in replacements
  are only meaningful when PCRE2 is available; when falling back to `RSearch` this
  is an accepted limitation.
- **Search/replace history is not persisted** — `findHistoric_` and `replHistoric_`
  are in-memory only. The history is intentionally scoped to the current session
  for within-session reuse; persistence across sessions is not a goal.

---

## §1 — Direction design is inconsistent across `Searchable` implementations

**Verdict: Low concern — works correctly, but the abstraction leaks.**

The `Searchable` interface carries no notion of search direction. Each implementation
encodes direction differently:

| Implementation | How direction is expressed |
|---|---|
| `NSearch` / `NSearchRev` | Separate classes per direction |
| `RSearch` | `bool down` constructor argument (single class) |
| `PcreSearch` | `bool down` constructor argument (single class) |

As a consequence, `SearchManager::ConstructSearcher` must handle the `NSearch` case
with a four-way branch (down × ignoreCase), while `RSearch` and `PcreSearch` are
constructed once and handed the flag.

Additionally, the `stt` parameter of `Searchable::Search()` carries two different
meanings depending on direction:
- Forward: "start searching from this index onward"
- Backward: "return the rightmost match whose start is at or before this index"

This dual meaning is implicit — not expressed in the interface. The code works
because each implementation knows its own direction, but the contract is unclear
to a reader of the interface alone.

**Recommended action:** Acknowledge as a design quirk. The three-engine architecture
makes a clean unified interface difficult without significant rework. No immediate
action needed; document the `stt` convention in a comment on `Searchable::Search()`
if confusion arises during future maintenance.

---

## §2 — `DoExpandRepl` two-step pattern is duplicated

**Verdict: Low concern — straightforward to clean up.**

Both `ReplaceImpl` and `ReplaceAllImpl` contain the same two-step pattern for
replacement string expansion:

```cpp
// Step 1: compute expanded length (out = NULL)
ulong expLen = DoExpandRepl(replPat, replPatLen, line, searcher_, NULL);
// Step 2: allocate and fill
unicode* expBuf = (unicode*)ki::TS.alloc((expLen+1) * sizeof(unicode));
DoExpandRepl(replPat, replPatLen, line, searcher_, expBuf);
expBuf[expLen] = L'\0';
```

The surrounding `if( bRegExp_ )` branch is also duplicated across the two methods.
This is the most actionable item: extracting a small helper that owns the
allocate-expand-return lifecycle would remove the duplication and reduce the chance
of the two methods drifting apart.

**Recommended action:** Extract a helper function (e.g. `ExpandReplacement`) that
takes the pattern, line, and searcher, and returns an allocated buffer with the
expanded string. Both `ReplaceImpl` and `ReplaceAllImpl` call it. This is a
low-risk, self-contained refactor.

---

## §3 — PCRE2 backward search uses linear forward scan

**Verdict: Low concern — unavoidable given PCRE2's API.**

PCRE2 provides no native backward-search function. `PcreSearch::Search()` with
`down_=false` implements backward search by scanning forward from position 0 to
`stt`, keeping only the last match. For long lines with many matches this is O(n)
in the number of matches.

In practice, lines in a text editor are short enough that this is imperceptible.
There is no straightforward workaround within the PCRE2 API.

**Recommended action:** No action. Document this as an inherent constraint of the
PCRE2 integration.

---

## Summary

| Concern | Verdict | Recommended action |
|---|---|---|
| Direction design inconsistency | Low — works, abstraction leaks | Add clarifying comment on `Searchable::Search()`; no restructuring needed |
| `DoExpandRepl` duplication | Low — easy to fix | Extract `ExpandReplacement` helper (optional cleanup) |
| PCRE2 backward search O(n) | Low — API constraint | No action; document as inherent limitation |
| `RSearch` capture groups absent | Not a problem — by design | — |
| Search history not persisted | Not a problem — by design | — |
