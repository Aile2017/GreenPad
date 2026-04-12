# GreenPad — Code Concerns

This document lists redundant, duplicated, or overlapping implementations found
during a codebase review. Items are ordered by priority.

## 1. High priority

### Multiple implementations of `.lay` file read/write

The most significant concern. Parsing and saving `.lay` files is split across
several locations, each carrying similar but separately maintained logic:

- `ConfigManager::LoadLayout()`
- `ConfigManager::SaveFontToLayFile()`
- `LayEditDlg::ParseLayData()`
- `LayEditDlg::SaveToFile()`
- New `.lay` file creation path

In this state, updating the `.lay` specification in one place does not
automatically propagate to the others, making behavioral divergence and
save-content drift easy to introduce.

The deeper problem is that **responsibility for the same format is spread across
multiple sites** — this is not merely duplicate code but a structural liability
that makes future inconsistencies likely.

**Status: resolved** — see `lay-refactor.md`.

## 2. Medium priority

### `.lay` default values hand-coded in multiple places

Initial values for colours, font, and wrap settings are written out individually
at several sites:

- Built-in defaults applied during layout load
- Layout-edit dialog initialisation
- Fallback values during font save
- New `.lay` file template

No critical breakage is visible today, but changing the default specification
risks leaving some sites on the old values.

**Status: resolved** — see `lay-refactor.md` (Phase B-4, `LayData::SetDefaults()`).

### External filter has grown as a separate launch path from `on_external_exe_start`

The external command launcher (`on_external_exe_start`, used by Grep and Help)
and the external filter (`on_exfilter`) are separate systems. Although they both
call `CreateProcess`, their responsibilities differ enough that merging them would
add complexity rather than reduce it.

**Status: analysed** — see `exfilter-analysis.md`. No action required.

### External filter history reimplements the same pattern as MRU

The fixed-length front-shift history used by MRU and by the filter history are
algorithmically similar but implemented separately. The filter history adds
explicit deletion and reordering operations that MRU lacks, so the interfaces
do not align cleanly.

**Status: analysed** — see `exfilter-analysis.md`. Deferred.

## 3. Low priority

### External filter execution path has redundant I/O

The filter writes selection text to `tmpIn`, pipes it to the child process via
stdin, reads stdout into memory, then writes that memory buffer back out to
`tmpOut` before reading it through `TextFileR` for charset decoding. The
`tmpOut` round-trip exists solely to reuse `TextFileR`'s charset handling.

**Status: analysed** — see `exfilter-analysis.md`. Recognised as tech debt;
no immediate action planned.

### Search system has some scattered responsibilities

The three search engines (plain BM, built-in NFA regex, PCRE2) are a natural
split. However, backward search encoding and replacement expansion handling
are not fully consistent across the implementations.

**Status: analysed** — see `search-analysis.md`.

## 4. Conclusion

The most important items at the time of this review were:

1. Unifying `.lay` read/write — **done**
2. Cleaning up the external filter execution path — analysed, low urgency
3. Unifying history management logic — analysed, deferred

Addressing items 1–3 significantly reduces the risk of "parallel implementations"
growing further with each new feature addition.
