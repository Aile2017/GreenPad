# GreenPad TODO

## External Filter Feature

### Overview

A feature that passes the text being edited to an external program and replaces it with the program's output.

### Behavior

- If there is a selection: passes only the selected text to the external program's stdin, and replaces the selection with stdout.
- If there is no selection: passes the entire file to stdin, and replaces the entire file with stdout.

### How to Invoke

- Menu: added to the Edit > Modify submenu
- Shortcut: `Ctrl+\`
- The shortcut opens a command-line input dialog; the entered command is executed via `cmd.exe /c`.

### Encoding / Line Endings

- Text is passed to stdin as raw bytes using the current file's encoding and line ending style (same as if the file were saved and handed to the external program).
- The stdout output is interpreted with the same encoding and used to replace the text.

### Error Handling

- If the filter program exits with a non-zero exit code, the original text is preserved.
- An error dialog is shown on non-zero exit:
  - If stderr has output, it is included in the dialog message.
  - If stderr is empty, only `Exit code: N` is shown.

### Why Execute via `cmd.exe /c`

- Allows use of built-in commands such as `sort` and `find`.
- Supports pipe chains like `cmd1 | cmd2`.
- Supports `2>&1 |` to redirect stderr to another program.
- Supports conditional execution like `cmd1 && cmd2`.

### Command Input Dialog

- Window title: `External Filter` (localized per language)
- Label above combo box: `Command:` (localized)
- Buttons: `Run` and `Cancel` (localized)

```
┌─ External Filter ──────────────────────┐
│ Command:                               │
│ [ sort -r                         ]▼  │
│                                        │
│                    [  Run  ] [Cancel]  │
└────────────────────────────────────────┘
```

- Input area is a combo box:
  - Up/Down keys cycle through history in the input field.
  - The ▼ button opens the dropdown list.
  - Same UX as the existing search dialog combo box.
- History is stored in the ini file (same mechanism as the recent files list):
  - Holds up to 10 entries (may be made configurable in the future).
  - A command is added to history when `CreateProcess` succeeds.
    - Launch failures (command not found, etc.) are not added.
    - Commands that launch but exit with a non-zero code are added.

### Language Resources

Add the following entries to each language file.

#### `[Menu]` — Menu item

| Language | String |
|---|---|
| en-US | `E&xternal Filter...\tCtrl+\` |
| ja-JP | `外部フィルタ(&X)...\tCtrl+\` |
| zh-CN | `外部过滤器(&X)...\tCtrl+\` |
| zh-TW | `外部篩選器(&X)...\tCtrl+\` |
| ko-KR | `외부 필터(&X)...\tCtrl+\` |
| ru-RU | `Внешний фильтр(&X)...\tCtrl+\` |

#### `[Dialog.IDD_EXFILTER]` — Dialog strings

| Key | en-US | ja-JP | zh-CN | zh-TW | ko-KR | ru-RU |
|---|---|---|---|---|---|---|
| `Caption`  | `External Filter` | `外部フィルタ` | `外部过滤器` | `外部篩選器` | `외부 필터` | `Внешний фильтр` |
| `static.0` | `&Command:` | `コマンド(&C):` | `命令(&C):` | `命令(&C):` | `명령(&C):` | `Команда(&C):` |
| `IDOK`     | `&Run` | `実行(&R)` | `运行(&R)` | `執行(&R)` | `실행(&R)` | `Выполнить(&R)` |
| `IDCANCEL` | `Cancel` | `キャンセル` | `取消` | `取消` | `취소` | `Отмена` |

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

### Implementation Notes (not yet started)

- Dialog: a single-line input dialog using the existing `DlgImpl` base class.
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
  - Dialog width and combo box width should be based on the caption `Внешний фильтр`.
  - Error messages use `MessageBox` auto-wrap, so no special handling is needed.
