---
name: add-language
description: 'GreenPad/GreenStar に新しい言語ファイルを追加する。Use when: 多言語対応を追加したい、新しいロケールの .lng ファイルを作りたい、既存の言語ファイルの文字列を更新したい。'
argument-hint: '追加したいロケール識別子 (例: fr-FR)'
---

# 多言語対応の追加

## 対象ファイル

GreenPad と GreenStar はそれぞれ独立した言語ファイルディレクトリを持つ。

| アプリ | 言語ファイル置き場 | 参照タイミング |
|---|---|---|
| GreenPad | `release/lang/` | 起動時に `LangManager::AutoLoad()` |
| GreenStar | `release/lang-gs/` | 起動時に `LangManager::AutoLoad()` |

既存のロケール: `en-US`, `ja-JP`, `zh-CN`, `zh-TW`, `ko-KR`, `ru-RU`

---

## 手順

### 1. ベースファイルの確認

まず英語版を読んで翻訳すべき文字列を把握する。

```
release/lang/en-US.lng     ← GreenPad 用
release/lang-gs/en-US.lng  ← GreenStar 用
```

### 2. GreenPad 用言語ファイルの作成

`release/lang/<ロケール>.lng` を作成する。  
`en-US.lng` をコピーして各 `= ` の右辺を翻訳する。  
ヘッダ行は以下のように変更する:

```
# GreenPad Language File
# locale: <ロケール>  (例: fr-FR)
```

### 3. GreenStar 用言語ファイルの作成

`release/lang-gs/<ロケール>.lng` を作成する。  
`release/lang-gs/en-US.lng` をコピーして同様に翻訳する。  
GreenStar 固有の文字列として以下を必ず翻訳する:

- `IDS_APPNAME` = `GreenStar`（そのまま）
- `IDS_MODIFIEDOUT` = "ファイルが GreenStar 外で変更されました..." に相当する文

### 4. エスケープ規則

| 表記 | 意味 |
|---|---|
| `\n` | 改行 |
| `\\` | バックスラッシュ |
| `%d`, `%s` | printf 書式指定子 (そのまま残す) |

### 5. 動作確認

起動時のコマンドライン引数でロケールを指定してテストする:

```
release\GreenPad.exe -lang fr-FR
release\GreenStar.exe -lang fr-FR
```

または `GreenPad.ini` の `[SharedConfig]` セクションに `Language=fr-FR` を追記してもよい。

---

## 注意点

- `.lng` ファイルの文字コードは **UTF-8 (BOM なし)**。
- 翻訳しなかった項目は自動的に英語フォールバックされる。
- `IDS_PROJECT_URL` 等の URL 文字列は翻訳不要。
- LangManager の実装は `LangManager.cpp/h` を参照。
