# External Filter Feature

### Overview

A feature that passes the text being edited to an external program and replaces it with the program's output.

### Behavior

- If there is a selection: passes only the selected text to the external program's stdin, and replaces the selection with stdout.
- If there is no selection: passes the entire file to stdin, and replaces the entire file with stdout.

### How to Invoke

- Menu: added directly to the Edit menu
- Shortcut: `Ctrl+G`
- The shortcut opens a command-line input dialog; the entered command is executed via `cmd.exe /c`.

### Encoding / Line Endings

- Text is passed to stdin as raw bytes using the current file's encoding and line ending style (same as if the file were saved and handed to the external program).
- The stdout output is interpreted with the same encoding and used to replace the text.

### Error Handling

Exit code handling follows the grep convention:

- **Exit code 0**: success — stdout replaces the selection (or the whole file if no selection was active).
- **Exit code 1 with no stderr output**: treated as success with no output — the selection is left unchanged and no error dialog is shown.
  This covers tools such as `grep` that use exit code 1 to mean "no matches found" rather than a runtime error.
- **Any other exit code, or exit code 1 with stderr output**: the original text is preserved and an error dialog is shown.
  - If stderr has output, it is included in the dialog message.
  - If stderr is empty, only `Exit code: N` is shown.

### Why Execute via `cmd.exe /c`

- Allows use of built-in commands such as `sort` and `find`.
- Supports pipe chains like `cmd1 | cmd2`.
- Supports `2>&1 |` to redirect stderr to another program.
- Supports conditional execution like `cmd1 && cmd2`.

### Command Input Dialog

- Window title: `External Filter` (localized per language)
- Layout: Command history list (ListView) with input field and control buttons
- Buttons: `Run`, `Cancel`, `Del`, `Up`, `Down`, `Pin` / `Unpin` (localized)

**UI Layout:**

```
┌─ External Filter ──────────────────────────────┐
│                                                 │
│ Command History:                                │
│ ┌─────────────────────────────────────────┐    │
│ │ Pin │ Command                           │    │
│ ├─────┼─────────────────────────────────────┤    │
│ │  *  │ sort -r                           │    │
│ │  *  │ grep -n TODO                      │    │
│ │     │ wc -l                             │    │
│ │     │ cat -v                            │    │
│ └─────────────────────────────────────────┘    │
│ [Del] [Up] [Down] [Pin]   [  Run  ] [Cancel]  │
│                                                 │
│ Command: [________________________]             │
└─────────────────────────────────────────────────┘
```

**Interaction:**

- The ListView displays command history with two columns:
  - Column 1: Pin marker (`*` for pinned items, empty for regular history)
  - Column 2: Command text
- Click a list item to select it; the command appears in the input field below.
- `Del`, `Up`, `Down` buttons operate on the selected list item.
- `Pin` / `Unpin` button toggles the pin state of the selected item.
- The input field allows manual entry of new commands.
- `Run` executes the command from the input field.

### Command History

#### Storage

History is stored in the ini file and split into two independent lists:

| List | INI keys | Capacity | Constant |
|---|---|---|---|
| Pinned commands | `FilterPin1` – `FilterPin10` | 10 | `kFilterPinnedMax` |
| Regular history | `FilterCmd1` – `FilterCmd20` | 20 | `kFilterHistoryMax` |

A command is added to regular history when `CreateProcess` succeeds.
- Launch failures (command not found, etc.) are not added.
- Commands that launch but exit with a non-zero code are added.
- If the executed command is already in the pinned list, it is **not** added to regular history.

#### List Order in the ListView

Pinned commands are shown first in the ListView, followed by regular history entries.
Pinned commands are marked with `*` in the first column:

```
* │ sort -r          ← pinned (pinnedHistory_[0])
* │ grep -n TODO     ← pinned (pinnedHistory_[1])
  │ wc -l            ← regular history (filterHistory_[0])
  │ cat -v           ← regular history (filterHistory_[1])
```

When a pinned command is executed, the `*` prefix is stripped before passing the command to `cmd.exe`.

#### Pin / Unpin Button

The `Pin` button label changes dynamically based on whether the selected list item is pinned:

| Current selection state | Button label |
|---|---|
| Not pinned | `Pin` |
| Already pinned | `Unpin` |

The label is updated whenever the list selection changes.

**Pin operation** (not pinned → pinned):

1. Read the current text from the selected list item or input field.
2. If `pinnedHistory_` already holds `kFilterPinnedMax` entries → show error message (`"Pinned list is full"`), abort.
3. If the same command exists in `filterHistory_`, remove it.
4. Insert the command at the front of `pinnedHistory_` (shift existing entries down).
5. Save to INI and refresh the list view.

**Unpin operation** (pinned → not pinned):

1. Read the current text from the selected list item (strip leading `* `).
2. Remove the command from `pinnedHistory_`.
3. Insert it at the front of `filterHistory_` (same as `AddFilterHistory`).
4. Save to INI and refresh the list view.

#### Del Button

- Selected item is pinned → remove from `pinnedHistory_`.
- Selected item is regular history → remove from `filterHistory_`.

#### Up / Down Buttons

Movement is constrained **within each group**; commands cannot cross the boundary between the pinned group and the regular history group via Up/Down.

| Situation | Result |
|---|---|
| Up on the first pinned item | No-op |
| Down on the last pinned item | No-op (does not enter regular history) |
| Up on the first regular history item | No-op (does not enter pinned group) |
| Down on the last regular history item | No-op |

To move a command between groups, use the Pin / Unpin button explicitly.

### Language Resources

Add the following entries to each language file.

#### `[Menu]` — Menu item

| Language | String |
|---|---|
| en-US | `E&xternal Filter...\tCtrl+G` |
| ja-JP | `外部フィルタ(&X)...\tCtrl+G` |
| zh-CN | `外部过滤器(&X)...\tCtrl+G` |
| zh-TW | `外部篩選器(&X)...\tCtrl+G` |
| ko-KR | `외부 필터(&X)...\tCtrl+G` |
| ru-RU | `Внешний фильтр(&X)...\tCtrl+G` |

#### `[Dialog.IDD_EXFILTER]` — Dialog strings

| Key | en-US | ja-JP | zh-CN | zh-TW | ko-KR | ru-RU |
|---|---|---|---|---|---|---|
| `Caption`  | `External Filter` | `外部フィルタ` | `外部过滤器` | `外部篩選器` | `외부 필터` | `Внешний фильтр` |
| `static.0` | `&Command:` | `コマンド(&C):` | `命令(&C):` | `命令(&C):` | `명령(&C):` | `Команда(&C):` |
| `IDOK`     | `&Run` | `実行(&R)` | `运行(&R)` | `執行(&R)` | `실행(&R)` | `Выполнить(&R)` |
| `IDCANCEL` | `Cancel` | `キャンセル` | `取消` | `取消` | `취소` | `Отмена` |
| `IDC_FILTERDELBTN` | `Del` | `削除` | `删除` | `刪除` | `삭제` | `Удалить` |
| `IDC_FILTERUPBTN` | `Up` | `上へ` | `上移` | `上移` | `위로` | `Вверх` |
| `IDC_FILTERDOWNBTN` | `Down` | `下へ` | `下移` | `下移` | `아래로` | `Вниз` |
| `IDC_FILTERPINBTN` | `Pin` | `固定` | `固定` | `固定` | `고정` | `Закрепить` |
| `IDC_FILTERPINBTN.unpin` | `Unpin` | `固定解除` | `取消固定` | `取消固定` | `고정 해제` | `Открепить` |

> `IDC_FILTERPINBTN.unpin` is a virtual key used to look up the "Unpin" label at runtime; the dialog code calls `LangManager` to retrieve both strings and swaps the button text dynamically.

#### `[Strings]` — Error dialog

Filter failure message (`IDS_EXFILTER_FAILED`). `%d` is replaced with the exit code.
If stderr has output, it is appended after a newline following this string.

| Language | String |
|---|---|
| en-US | `External filter failed (exit code: %d)` |
| ja-JP | `外部フィルタが失敗しました (終了コード: %d)` |
| zh-CN | `外部过滤器失败（退出代码：%d）` |
| zh-TW | `外部篩選器失敗（結束代碼：%d）` |
| ko-KR | `외부 필터가 실패했습니다 (종료 코드: %d)` |
| ru-RU | `Внешний фильтр завершился с ошибкой (код завершения: %d)` |

Pinned list full message (`IDS_EXFILTER_PINFULL`):

| Language | String |
|---|---|
| en-US | `Pinned list is full (max: %d).` |
| ja-JP | `固定リストが満杯です (上限: %d)。` |
| zh-CN | `固定列表已满（上限：%d）。` |
| zh-TW | `固定清單已滿（上限：%d）。` |
| ko-KR | `고정 목록이 가득 찼습니다 (최대: %d).` |
| ru-RU | `Список закреплённых команд заполнен (макс.: %d).` |

### Usage Examples

Commands are entered as-is in the dialog; they are executed as `cmd.exe /c <command>`.

#### Using MSYS2 Tools (sort, sed, awk)

MSYS2 Unix tools (`sort`, `sed`, `awk`, etc.) are not on the Windows PATH by default.
Prepend `c:\usr\msys2\usr\bin` inline using `set PATH=...` so the tools are found without
permanently modifying system settings.

```
set PATH=c:\usr\msys2\usr\bin;%PATH% & sort
```

Sort lines in reverse order:

```
set PATH=c:\usr\msys2\usr\bin;%PATH% & sort -r
```

Remove duplicate lines (input must be sorted first):

```
set PATH=c:\usr\msys2\usr\bin;%PATH% & sort -u
```

Delete trailing whitespace with `sed`:

```
set PATH=c:\usr\msys2\usr\bin;%PATH% & sed "s/[[:space:]]*$//"
```

Extract only non-empty lines:

```
set PATH=c:\usr\msys2\usr\bin;%PATH% & sed "/^$/d"
```

Number each line with `awk`:

```
set PATH=c:\usr\msys2\usr\bin;%PATH% & awk "{print NR\": \"$0}"
```

Sum a column of numbers (first field) with `awk`:

```
set PATH=c:\usr\msys2\usr\bin;%PATH% & awk "{s+=$1} END{print s}"
```

#### Perl One-liners

Sort lines:

```
perl -e "print sort <STDIN>"
```

Sort lines in reverse order:

```
perl -e "print reverse sort <STDIN>"
```

Remove duplicate lines (preserving order):

```
perl -ne "print unless $seen{$_}++"
```

Delete trailing whitespace:

```
perl -pe "s/\s+$/\n/"
```

Convert to uppercase:

```
perl -pe "$_ = uc"
```

#### Python One-liners

Sort lines:

```
python -c "import sys; print(''.join(sorted(sys.stdin.readlines())), end='')"
```

Sort lines in reverse order:

```
python -c "import sys; print(''.join(sorted(sys.stdin.readlines(), reverse=True)), end='')"
```

Remove duplicate lines (preserving order):

```
python -c "import sys; seen=set(); [print(l, end='') for l in sys.stdin if not (l in seen or seen.add(l))]"
```

#### Ruby One-liners

Sort lines:

```
ruby -e "puts $stdin.readlines.sort"
```

Sort lines in reverse order:

```
ruby -e "puts $stdin.readlines.sort.reverse"
```

Remove duplicate lines (preserving order):

```
ruby -e "puts $stdin.readlines.uniq"
```

Delete trailing whitespace:

```
ruby -pe "$_.rstrip!; $_ += $/"
```

#### Pipeline Examples

Sort, remove duplicates, then number each line:

```
set PATH=c:\usr\msys2\usr\bin;%PATH% & sort -u | awk "{print NR\". \"$0}"
```

Filter lines matching a pattern, then sort:

```
set PATH=c:\usr\msys2\usr\bin;%PATH% & grep "TODO" | sort
```

Extract the second field (space-separated), sort, and remove duplicates:

```
set PATH=c:\usr\msys2\usr\bin;%PATH% & awk "{print $2}" | sort -u
```

Delete blank lines, trim trailing spaces, then sort:

```
set PATH=c:\usr\msys2\usr\bin;%PATH% & sed "/^$/d" | sed "s/[[:space:]]*$//" | sort
```

Reverse line order using `tac` (tail-to-head):

```
set PATH=c:\usr\msys2\usr\bin;%PATH% & tac
```

### Implementation Notes

- Dialog: ListViewベースの履歴管理 UI と入力フィールド、制御ボタンを含むダイアログ（`DlgImpl` 継承）
- Use `CreateProcess` with anonymous pipes connected to stdin/stdout/stderr.
- Block UI during filter execution (to prevent corruption of the edit buffer).
- Use threads to avoid pipe buffer deadlock:
  - Write to stdin and read from stdout/stderr concurrently.
  - Suggested layout:
    - Writer thread: sends all data to stdin via `WriteFile`, then calls `CloseHandle` to signal EOF.
    - Main thread: reads stdout continuously via `ReadFile`.
    - stderr thread: reads stderr continuously via `ReadFile` (may be combined with the writer thread).
  - After all threads complete, call `WaitForSingleObject` to wait for the child process to exit.
  - Retrieve the exit code to determine success or failure.
- Reuse the existing thread wrapper in `kilib/thread.cpp`.
- Size dialog controls to fit the Russian locale:
  - The `Run` button must be wide enough for `Выполнить` (10 characters).
  - Dialog width and list view width should be based on the caption `Внешний фильтр`.
  - Error messages use `MessageBox` auto-wrap, so no special handling is needed.

#### Pin Feature Implementation Notes

- `ConfigManager` holds two arrays:
  - `filterHistory_[kFilterHistoryMax]` (existing, `kFilterHistoryMax = 20`)
  - `pinnedHistory_[kFilterPinnedMax]` (new, `kFilterPinnedMax = 10`)
- INI read/write follows the same pattern as `FilterCmd*`:
  - Keys `FilterPin1` – `FilterPin10` under the user section.
- `ExFilterDlg` receives both arrays and their lengths.
  - On `on_init()`: populate the ListView with pinned entries (marked with `*` in column 1) then regular history entries.
  - Track `pinnedCount_` (number of non-empty pinned entries) to determine group boundaries for Up/Down and Pin/Unpin logic.
- Pin/Unpin button label update: compare selected list item text (after stripping leading `*` if present) against `pinnedHistory_` entries; set button text accordingly.
- When the ListView selection changes, refresh the Pin button label and update the input field with the selected command.
- `AddFilterHistory` must check `pinnedHistory_` before inserting into `filterHistory_`; skip the insert if the command is already pinned.
