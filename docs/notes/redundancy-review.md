# Redundancy and Cleanup Candidates

This note summarizes cleanup candidates found during a repository-wide review.
The items below are not urgent bug fixes, but they are good candidates for future maintenance work because they increase duplication, make intent harder to follow, or leave obsolete code paths in place.

## 1. Consolidate replacement expansion in `Search.cpp`

`SearchManager::ReplaceImpl()` and `SearchManager::ReplaceAllImpl()` both expand regular-expression replacement text with nearly identical logic.
They each call `DoExpandRepl()`, allocate a temporary buffer, write the expanded text, and add a null terminator.

**Why this matters**

- The replacement behavior is maintained in two places.
- Future fixes to backreference handling can easily drift.
- The duplicated flow makes the replace code longer than necessary.

**Suggested follow-up**

Extract the shared flow into a helper that returns the expanded replacement buffer and length.

## 2. Reduce duplicated dialog-to-`LayData` copying in `ConfigManager.cpp`

The layout editor dialog manually copies many of the same fields between dialog state and `LayData`.
This appears in `LayEditDlg::ParseLayData()`, `LayEditDlg::SaveToFile()`, and the constructor's initialization path.

**Why this matters**

- Adding a new layout field requires touching several copy blocks.
- It increases the chance of partial updates or missed fields.
- The current code is more verbose than the actual layout logic.

**Suggested follow-up**

Introduce small helpers for `DialogState -> LayData` and `LayData -> DialogState`.

## 3. Simplify duplicated combo-box text loading in `SearchManager::UpdateData()`

`UpdateData()` reads the find text and replace text with the same allocation and `GetItemText()` pattern twice.

**Why this matters**

- This is small duplication, but it adds noise to a central update routine.
- Any future change to history handling or text extraction must be repeated.

**Suggested follow-up**

Use a small helper for reading combo-box text into a `ki::String` and updating history.

## 4. Revisit `SearchManager::NotFound()`

`SearchManager::NotFound()` still exists as a separate method, but most not-found behavior is already handled directly in `FindNextImpl()` and `FindPrevImpl()`.
The remaining helper is only used from `ReplaceImpl()` and does not reflect the newer selection-aware logic.

**Why this matters**

- The code exposes extra behavior that is only partially aligned with the current search flow.
- It makes the not-found handling look more shared than it really is.

**Suggested follow-up**

Either remove `NotFound()` and inline its remaining use, or rebuild it as the single shared not-found path.

## 5. Clarify window size restoration logic in `ConfigManager.h`

`GetWndW()` and `GetWndH()` mix two ideas: honoring `rememberWindowSize_` and falling back to stored rectangle values when they look valid.
Because window position is stored even when only position restoration is enabled, the logic is harder to read than it needs to be.

**Why this matters**

- The code works, but the intent is not immediately clear.
- It is easy to misread the current behavior as a bug or accidental overlap.

**Suggested follow-up**

Refactor the width/height getters so the behavior is explicit and documented.

## 6. Review legacy non-Unicode branches

Several files still contain `#ifdef UNICODE` / `#ifndef _UNICODE` branches, including code in `Search.cpp`.
Current build files define Unicode for all supported toolchains, so these branches are likely legacy compatibility paths.

**Why this matters**

- Dead or effectively dead branches increase maintenance burden.
- They make active logic harder to inspect.

**Suggested follow-up**

Confirm whether non-Unicode builds are still a real target.
If not, remove the unused branches in small, verified steps.

## Priority Guidance

If these items are addressed incrementally, the most valuable order is:

1. Consolidate replacement expansion in `Search.cpp`
2. Reduce repeated `LayData` copying in the layout editor dialog
3. Clean up `SearchManager::NotFound()`
4. Simplify combo-box text loading
5. Clarify window restoration logic
6. Audit and remove legacy non-Unicode branches where safe
