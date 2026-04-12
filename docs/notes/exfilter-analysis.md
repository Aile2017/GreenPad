# External Filter Concerns — Analysis

Analysis of the external-filter-related concerns listed in `懸念.md` §2–3.

---

## §2-a — External filter uses a separate launch path from `on_external_exe_start`

**Verdict: Low concern — the separation is appropriate.**

`on_external_exe_start` (used by Grep and Help) and `on_exfilter` have fundamentally
different responsibilities:

| Aspect | `on_external_exe_start` | `on_exfilter` |
|---|---|---|
| Purpose | Launch external tool and forget | Pipe text through filter, replace selection |
| I/O | None (fire-and-forget) | stdin/stdout/stderr fully connected |
| Result | Return value unused | Result replaces editor content |
| Threading | Not needed | Three threads: stdin writer, stderr reader, stdout reader (main) |
| History | None | Yes |

Merging these two paths would yield at most a shared `CreateProcess` call;
the surrounding setup and teardown differ entirely. **No action required.**

---

## §2-b — Filter history duplicates the MRU pattern

**Verdict: Medium concern — acceptable as-is for now.**

The algorithms are similar: both maintain a fixed-length array with most-recently-used
ordering and front-shift insertion. However, FilterHistory has two operations that MRU
lacks:

- `RemoveFilterHistory` — explicit deletion by the user
- `SwapFilterHistory` — reordering within the history list (Up/Down buttons in the dialog)

These differences mean the interfaces do not align cleanly. Extracting a shared template
would require non-trivial design work under the project's no-STL / no-exceptions
constraints, and the payoff is limited. **Defer to a future refactoring session if a
third history-like structure is ever added.**

---

## §3 — Redundant I/O in the external filter execution path

**Verdict: High concern (confirmed in code) — medium-to-low priority to fix.**

The actual data flow in `on_exfilter` is:

```
selection (memory)
  → tmpIn  (write to temp file)
  → [ExfStdinThread] read tmpIn → write to stdin pipe
  → [child process] stdin → process → stdout
  → [main thread]   read stdout pipe → stdoutBuf (memory)
  → tmpOut (write to temp file)      ← redundant
  → [TextFileR]     read tmpOut      ← redundant
  → replace editor content (memory)
```

`stdoutBuf` is already in memory when the child process exits.
Writing it to `tmpOut` and reading it back via `TextFileR` serves only one purpose:
**charset decoding** — `TextFileR` handles auto-detection and conversion, which is
needed because the filter output may not be UTF-16.

To eliminate `tmpOut`, `TextFileR` (in `kilib/textfile`) would need a way to accept
an in-memory byte buffer instead of a file path. That is a `kilib` API change, not
a trivial edit.

**Impact in practice:** The extra disk round-trip is invisible for typical file sizes.
It only becomes a concern with very large selections or high-throughput filter
commands. Recognized as technical debt; no immediate action planned.

---

## Summary

| Concern | Validity | Recommended action |
|---|---|---|
| §2-a Separate launch paths | Low — responsibilities differ fundamentally | No action |
| §2-b Duplicate history logic | Medium — similar but not identical | Defer; revisit if a third history is added |
| §3 Redundant tmpOut round-trip | High — confirmed in code | Acknowledge as tech debt; fix if `TextFileR` is ever extended to accept buffers |
