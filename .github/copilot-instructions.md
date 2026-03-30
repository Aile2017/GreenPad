# Copilot Instructions for GreenPad text editor

## 前提条件
- 回答は必ず日本語でしてください.
- 200行以上の大規模な変更を行う前には, まず変更計画を提案してください.
- 既存コードを読まずに変更を提案しないこと.

## アプリの概要
GreenPad : テキストエディタ. コンパクトなバイナリ. Windows標準操作によるインタフェイス. 多言語対応.
GreenStar : GreenPad を元にした WordStarライクなキー操作を導入したテキストエディタ.

どちらもファイルを読み込む際のエンコーディング判定をある程度自前で行えるが
chardet.dll を使うことでさらに制度を上げることができる.
検索/置換では簡単な正規表現が使用できるが, pcre2-16.dll を使うことでPerl準拠の正規表現が使えるようになる.

## 技術スタック（エコシステム）
ビルド環境は MSVC(VS 2026), msys2(UCRT64, CLANG64)

| ツールチェーン | Makefile | 出力 |
|---|---|---|
| MSVC | Makefiles/vcc.mak | release/GreenPad.exe |
| MSVC | Makefiles/greenstar_vcc.mak | release/GreenStar.exe |
| MSYS2 UCRT64 / mingw64 | Makefiles/gcc64.mak | release/GreenPad.exe |
| MSYS2 CLANG64 | Makefiles/clang64.mak | release/GreenPad.exe |

※ GreenStar の MSYS2 向け Makefile は現時点では未作成.

MSVC は以下に導入されている
`C:\Program Files\Microsoft Visual Studio\18\Community`

msys2 は以下に導入されている
`C:\usr\msys64`

## プロジェクト構造
```
GreenPad/
  GpMain.cpp/h        … GreenPad のメインウィンドウ実装
  GreenStar/
    GsMain.cpp/h      … GreenStar のメインウィンドウ実装
    rsrc/             … GreenStar 専用リソース (アイコン, メニュー, 文字列)
  editwing/           … テキスト編集コアライブラリ (描画・カーソル・折り返し等)
  kilib/              … Windowsラッパーユーティリティ (ウィンドウ, ファイル, 文字列等)
  Search.cpp/h        … 検索UIと SearchManager
  NSearch.h           … 単純文字列検索 (Searchable 実装)
  RSearch.cpp/h       … 組み込み正規表現検索
  PcreSearch.cpp/h    … pcre2-16.dll 経由の PCRE2 正規表現検索
  ConfigManager.cpp/h … 設定管理 (INIファイル読み書き)
  LangManager.cpp/h   … 実行時言語切り替え (.lng ファイル)
  OpenSaveDlg.cpp/h   … ファイルダイアログ
  Makefiles/          … 各ツールチェーン向け Makefile
  release/            … ビルド成果物の配置先
    lang/             … GreenPad 用言語ファイル (en-US, ja-JP, zh-CN, zh-TW, ko-KR, ru-RU)
    lang-gs/          … GreenStar 用言語ファイル (同上)
    chardet.dll       … 文字コード自動判別 (省略可)
    pcre2-16.dll      … PCRE2 正規表現 (省略可)
```

## コード設計の注意点
- GreenPad と GreenStar は `kilib` / `editwing` / `Search` / `ConfigManager` / `LangManager` を**共有**している.
  GpMain と GsMain は並行進化を避けるため, 共通ロジックは上記モジュールに置くこと.
- `GreenStarWnd::HandleWsKey()` が WordStar 2ストロークキー操作の全てを担う.
  Ctrl+K / Ctrl+Q がプレフィックスキーで, 2ストローク目で各コマンドに振り分ける.
- 言語ファイルは実行時にロードされる. GreenPad は `lang/`, GreenStar は `lang-gs/` を参照する.
- バイナリサイズを削減するため, STL・例外・RTTIを使わない方針で書かれている
  (`/GR- /EHs-c-`, clang64 側も `-fno-exceptions -fno-rtti`).
- 文字コードは原則 UTF-16 (`unicode` = `wchar_t`) で処理する.

## 外部ライブラリ
| ライブラリ | 場所 | 用途 |
|---|---|---|
| pcre2 | ワークスペース内 `pcre2/` | PCRE2 正規表現. `pcre2-16.dll` をビルドして `release/` に置く |
| libchardet | ワークスペース内 `libchardet/` | 文字コード自動判別. `chardet.dll` をビルドして `release/` に置く |

