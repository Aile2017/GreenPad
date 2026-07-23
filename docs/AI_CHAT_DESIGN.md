# GreenPad AIチャット機能 設計検討ドキュメント

- ステータス: 検討中（実装未着手）
- 対象: GreenPad（GreenStarへの展開は将来検討）

## 1. 目的・概要

編集中のファイルに対して、AIと対話しながら指示を出し、内容を操作（提案・挿入・置換など）できる
チャットボックスをGreenPadに新設する。使用するAIサービスは利用者が選択できる汎用設計とし、
少なくとも GitHub Copilot と Claude Code を利用可能にする。

本ドキュメントは実装方針の検討結果をまとめたものであり、実装そのものはまだ着手しない。

## 2. 全体アーキテクチャ

```
+-----------------------------------------------------------+
|                      GpMain (GreenPad)                    |
|                                                             |
|   +-------------------+        +--------------------------+ |
|   |  EwEdit (editor)  |<------>|  EditorAgent (facade)    | |
|   |  - getDoc()       |        |  - GetSelection()        | |
|   |  - getView()      |        |  - GetFullText()         | |
|   |  - getCursor()     |        |  - ApplyEdit(Command)    | |
|   +-------------------+        +-----------+--------------+ |
|                                             |                |
|   +-------------------+        +-----------v--------------+ |
|   |  ChatPane (新規UI) |<------>|  ChatController          | |
|   |  - 入力欄/履歴表示 |        |  - プロンプト組立        | |
|   +-------------------+        |  - 応答パース/差分適用   | |
|                                 +-----------+--------------+ |
|                                             |                |
|                                 +-----------v--------------+ |
|                                 |  IAiProvider (抽象IF)     | |
|                                 +---+-----------------+----+ |
|                                     |                   |    |
|                     +---------------v--+   +-----------v---+ |
|                     | HttpAiProvider    |   | ProcessAiProvider |
|                     | (OpenAI/Azure等)  |   | (Copilot CLI /  | |
|                     |  WinHTTP + JSON   |   |  Claude Code CLI)| |
|                     +-------------------+   +-----------------+ |
+-----------------------------------------------------------+
```

既存資産の再利用:
- `editwing::doc::Document` の `Insert`/`Delete`/`Replace`/`MacroCommand`（`ewDoc.h`）を
  そのまま編集適用の実行単位として使う。Undo/Redoは既存の仕組みに自動的に乗る。
- `EwEdit::getDoc()`/`getView()`/`getCursor()`（`ewCtrl1.h`）を土台に、UIスレッド安全な
  ファサード`EditorAgent`を新設する。
- プロバイダの複数実装を切り替え可能にする発想は、`Search.h`の`Searchable`インタフェースを
  `NSearch`/`RSearch`/`PcreSearch`が実装する既存パターンを踏襲する。

## 3. コンポーネント詳細

### 3.1 EditorAgent（編集操作ファサード）

役割: ChatController からの編集要求を、UIスレッド上で安全に `editwing` の Command 列へ変換して適用する。

想定インタフェース（設計イメージ、実装時に詳細化）:
- `GetFullText(encoding指定なしのunicode文字列)`
- `GetSelection(start, end)` … 行/列 or 文字オフセットなど外部向け座標系で返す
- `ReplaceRange(start, end, newText)` … 内部的に `editwing::doc::Replace` を発行
- `InsertAt(pos, text)` … `editwing::doc::Insert` を発行
- `ApplyMacro(edits[])` … 複数編集を `MacroCommand` にまとめ1回のUndo単位にする
- `on_text_update` 相当の変更通知を購読し、チャット側に「編集完了」を伝える

注意点:
- `DPos`はeditwing内部表現のため、外部（AI応答）とやり取りする座標系（行番号+列番号、または
  文字オフセット）との相互変換層が必要。
- `Insert`/`Delete`/`Replace`を連続適用する場合、位置ずれは自動調整されない
  （`ewDoc.h`のコメントに明記）ため、`EditorAgent`側で位置計算を行う。
- 呼び出しは必ずUIスレッドで実行する。AI応答はバックグラウンドスレッド/非同期I/Oで届くため、
  `PostMessage`等でUIスレッドにマーシャリングする。

### 3.2 ChatPane（チャットUI）

- ドッキング可能なペイン、またはモードレスダイアログとして新設。
- 入力欄、会話履歴表示、実行中インジケータ、プロバイダ選択UI、適用/差し戻しボタンを持つ。
- 既存の言語ファイル(`lang/`)の仕組みに合わせ、表示文字列は`LangManager`経由でローカライズする。

### 3.3 ChatController

- ChatPaneからの入力を受け、プロンプトを組み立てて`IAiProvider`に渡す。
- AI応答を受信し、以下のいずれかとして解釈する:
  - 通常の会話テキスト（チャット履歴に表示するだけ）
  - 編集指示（構造化フォーマット、例: JSON diff や `<edit>` タグ等）→ `EditorAgent`へ適用要求
- 編集指示を即時適用するか、プレビュー後にユーザー承認を経て適用するかは要検討
  （事故防止の観点から「提案→承認→適用」をデフォルトに、設定で自動適用を許可する案が妥当）。

### 3.4 IAiProvider（AIプロバイダ抽象インタフェース）

最小契約（イメージ）:
```
class IAiProvider {
public:
    virtual bool SendChat(const ChatRequest& req, ChatResponseHandler& out) = 0;
    virtual void Cancel() = 0;
    virtual ~IAiProvider() {}
};
```

- `ChatRequest`: メッセージ履歴、モデル名、パラメータ（temperature等、プロバイダにより無視可）
- `ChatResponseHandler`: ストリーミングのトークン/チャンクをコールバックで受け取るハンドラ
  （UIスレッドへの受け渡しは呼び出し側で行う）

実装は大きく2系統に分かれる。

#### (A) HttpAiProvider 系（OpenAI / Azure OpenAI など汎用API）
- 通信: WinHTTP API（`winhttp.dll`、OS標準、追加DLL不要。STL/例外/RTTI不使用方針と両立）。
- JSON: 軽量な単一ヘッダ相当のCライブラリを`pcre2`/`libchardet`同様に外部ライブラリとして
  ワークスペースに追加するか、必要フィールドが限定的であれば自作の簡易パーサで対応。
- ストリーミング: SSE(Server-Sent Events)をWinHTTPの非同期/チャンク受信で処理。
- 設定: エンドポイントURL、APIキー、モデル名を`ConfigManager`にプロファイル単位で保存。
- 多くのOpenAI互換サービス（Ollama、LM Studio、OpenRouter等）は共通スキーマのため、
  「OpenAI互換アダプタ」1本でかなりの範囲をカバーできる。Anthropic APIやGemini APIは
  スキーマが異なるため個別アダプタが必要。

#### (B) ProcessAiProvider 系（GitHub Copilot CLI / Claude Code CLI）
- GitHub Copilot、Claude Codeともに「第三者アプリ向けの汎用チャット補完HTTP API」は
  公式には提供されていない（Copilot REST APIは管理/利用状況取得用、Copilot Extensionsは
  GitHub Chat UI内で完結する仕組み）。
- 現実的な連携方法は、ローカルにインストール・認証済みのCLIを**外部プロセスとして起動**し、
  標準入出力をパイプで受け渡す方式。
  - Claude Code: `claude -p "プロンプト" --output-format stream-json` 等のヘッドレスモードが
    公式に提供されており、構造化出力・ストリーミング出力に対応。
  - GitHub Copilot CLI: 同様にCLIへプロンプトを渡し標準出力を受け取る想定
    （非対話呼び出しの詳細仕様は導入時に要確認）。
- 実装: `CreateProcess` + 匿名パイプで子プロセスの標準入出力を接続し、非同期に読み取って
  ストリーミング表示に反映する。プロセスの生存確認・キャンセル（`Cancel()`実行時のプロセス終了）
  も本アダプタの責務とする。
- 前提条件: 利用者自身がそれぞれのCLIをインストール・ログイン（サブスクリプション契約）済みで
  あること。GreenPad側でOAuth等の認証フローそのものは代行しない想定。

### 3.5 設定管理

- `ConfigManager`にAIプロバイダ関連の設定セクションを追加。
  - プロバイダ一覧（種別: Http/Process、表示名、エンドポイントorコマンドパス、APIキー、モデル名等）
  - 既定使用プロバイダ
  - 自動適用の可否（前述の「提案→承認→適用」設定）
- APIキーの平文保存の可否は要検討事項（Windows DPAPIによる暗号化保存等の代替案あり）。

## 4. データフロー（例: 「この関数をリファクタリングして」）

1. ユーザーがChatPaneに指示を入力し送信。
2. ChatControllerが`EditorAgent`経由で選択範囲/カーソル周辺のテキストを取得しプロンプトに含める。
3. 設定で選ばれた`IAiProvider`実装（例: ProcessAiProvider = Claude Code）にリクエストを送信。
4. ストリーミング応答をChatPaneの会話欄にリアルタイム表示。
5. 応答内に編集指示（構造化フォーマット）が含まれる場合、ChatControllerが差分を抽出。
6. プレビュー表示 → ユーザー承認 → `EditorAgent.ApplyMacro()`で`MacroCommand`として一括適用。
7. 適用結果は既存のUndo/Redoチェーンに1操作として記録される。

## 5. 未確定・要検討事項

- 編集指示の応答フォーマット（AIにどう構造化出力させるか。プロバイダごとにプロンプト調整が必要）。
- 自動適用 vs 承認制のデフォルト方針、および取り消し導線のUI設計。
- ファイル内容を外部AIサービスに送信することのプライバシー/セキュリティ方針
  （社外秘ファイルを扱う利用者への注意喚起、送信範囲の制限オプション等）。
- GitHub Copilot CLIの非対話呼び出し仕様の詳細確認（フラグ、出力フォーマット、認証状態の検出方法）。
- APIキー等の秘匿情報の保存方式（平文 or DPAPI暗号化）。
- GreenStarへの展開要否（`GsMain`側の対応、WordStarキー操作との競合有無）。
- バイナリサイズ・依存関係方針との整合（JSON/HTTP関連ライブラリ追加時の許容サイズ増）。

## 6. 次のステップ（実装着手前に合意すべき事項）

1. 編集指示の応答フォーマット仕様を確定する。
2. 承認制/自動適用のデフォルト方針を決定する。
3. `IAiProvider`の最小契約（メソッドシグネチャ）をレビューし確定する。
4. JSON処理ライブラリの採否（自作 or 外部ライブラリ導入）を決定する。
5. 上記が固まった時点で、段階的実装計画（EditorAgent → HttpAiProvider → ProcessAiProvider →
   ChatPane/ChatControllerの順、など）を別途起票する。
