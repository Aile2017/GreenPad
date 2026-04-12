# `.lay` ファイル処理リファクタリング

`懸念.md` §1 で特定された問題への対処をまとめた作業管理ドキュメントです。
タスクは実施順に並んでいます。完了したものはチェックを入れてください。

---

## フェーズ A — バグ修正（データロス）— 優先度：高

`ConfigManager::LoadLayout()` が読み込むにもかかわらず、すべての書込み経路で
サイレントに破棄されているフィールドの修正です。
`cs=`・`fq=`・`fx=` が設定された `.lay` ファイルは、次回書込み時にこれらの値を失います。

### A-1  `SaveFontToLayFile` — `cs=` / `fq=` / `fx=` の保持

- [x] 読取ループの前に `fontCS`・`fontQual`・`fontXWidth` のローカル変数を適切なデフォルト値で初期化する。
- [x] 読取ループに `case 0x6373`（`cs=`）・`case 0x6671`（`fq=`）・`case 0x6678`（`fx=`）を追加し、既存値を退避する。
- [x] 書込みブロックに `cs=`・`fq=`・`fx=` の出力を追加する（デフォルト値と同じ場合は省略: `cs=DEFAULT_CHARSET`・`fq=DEFAULT_QUALITY`・`fx=0`）。

### A-2  `LayEditDlg::ParseLayData` — `cs=` / `fq=` / `fx=` の読込み

- [x] `LayEditDlg` にメンバ変数 `fontCS_`・`fontQual_`・`fontXWidth_` を適切なデフォルト値で追加する。
- [x] `ParseLayData` の `switch` に3つの不足 case を追加する。

### A-3  `LayEditDlg::SaveToFile` — `cs=` / `fq=` / `fx=` の書出し

- [x] `ff=` の直後に `cs=`・`fq=`・`fx=` の出力を追加する（A-1 と同様、デフォルト値なら省略）。

---

## フェーズ B — 構造的な整理 — 優先度：中

コードの重複を排除し、`.lay` フォーマットの定義を一箇所に集約します。
将来フィールドを追加するときに修正箇所が1箇所で済むようにすることが目的です。

### B-1  `LayData` 構造体の導入

- [x] `ConfigManager.h`（または新規ヘッダ）に `struct LayData` を定義する。
  保持フィールド: `colors[7]`・`fontName`・`fontSize`・`fontWeight`・`fontFlags`・
  `fontCS`・`fontQual`・`fontXWidth`・`tabSize`・`scBits`・`wrapType`・`wrapWidth`・`wrapSmart`・`showLN`。
- [x] ビルトインデフォルト値（システムカラー・折り返し幅 80 など）を適用する
  `SetDefaults()` メソッドを追加する。

### B-2  共通パーサ `ConfigManager::ParseLayBuf()` の抽出

- [x] `ConfigManager` に `static void ParseLayBuf(unicode* buf, size_t len, LayData& out)` を追加する。
- [x] `LoadLayout` のパースループ（全フィールド対応の正規版）を `ParseLayBuf` に移植する。
- [x] `LoadLayout` のパースループを `ParseLayBuf` 呼び出しに置き換え、その後 `LayData` から `DocType` へフィールドをコピーする。
- [x] `LayEditDlg::ParseLayData` のパースループを `ParseLayBuf` 呼び出しに置き換え、その後 `LayData` からダイアログメンバ変数へコピーする。
- [x] `SaveFontToLayFile` の読取ループ（現在値の読み込みステップ）を `ParseLayBuf` 呼び出しに置き換える。

### B-3  共通ライタ `ConfigManager::WriteLayBuf()` の抽出

- [x] `ConfigManager` に `static size_t WriteLayBuf(unicode* out, const LayData& d)` を追加する（戻り値は書き込んだ文字数）。
- [x] `LayEditDlg::SaveToFile` の書込みブロック（最もフィールドが揃っている）を `WriteLayBuf` に移植する。
- [x] `LayEditDlg::SaveToFile` の書込みブロックを `WriteLayBuf` 呼び出しに置き換える。
- [x] `SaveFontToLayFile` の書込みブロックを `WriteLayBuf` 呼び出しに置き換える。
- [x] `on_addfile` の新規 `.lay` 作成ブロックを `WriteLayBuf` 呼び出しに置き換える。

### B-4  デフォルト値の一元管理

- [x] `LoadLayout`・`LayEditDlg` コンストラクタ・`on_addfile` の3箇所にある
  手書きのデフォルトカラーブロックを `LayData::SetDefaults()` 呼び出しに置き換える。

---

## 対象外（やらないこと）

以下は意図的にスコープ外とします。

- `SaveFontToLayFile` と `SaveToFile` の統合：呼び出し元と責務が異なるため（フォントのみ上書き vs. 全項目の往復）、統合するとかえって複雑になる。
- `LoadLayout` と `ParseLayData` の統合：出力先の型が異なる。`ParseLayBuf` の共有（B-2）で十分。

---

## 参考：リファクタ前のフィールドカバレッジ

| フィールド | `LoadLayout` | `SaveFontToLayFile` | `ParseLayData` | `SaveToFile` | 新規作成 |
|---|:---:|:---:|:---:|:---:|:---:|
| ct ck cb cr cc cn cl | 読 | 読/書 | 読 | 書 | 書 |
| ft sz fw ff | 読 | 書 | 読 | 書 | 書 |
| tb sc wp ww ws ln | 読 | 読/書 | 読 | 書 | 書 |
| **cs** | 読 | ❌ | ❌ | ❌ | ❌ |
| **fq** | 読 | ❌ | ❌ | ❌ | ❌ |
| **fx** | 読 | ❌ | ❌ | ❌ | ❌ |

フェーズ A 完了後: すべての ❌ セルが 読/書 になります。
フェーズ B 完了後: 重複していた読み書きロジックが共通関数に集約されます。
