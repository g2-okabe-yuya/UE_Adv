# First_Adv

# ADV System Architecture & Development Roadmap

Unreal Engine（UE）において外部シナリオデータを読み込み、Widget Blueprint（WBP）で再生・表示するADV（アドベンチャーゲーム）システムの基盤開発フローです。

---

## 🛠 開発フェーズ一覧

### Phase 1: シナリオデータ構造の定義 (Data Base)

1コマ（発言）に必要な情報を整理し、UEと外部ファイルでやり取りするデータ構造を設計します。

#### データ構造 (`FScenarioRow` Struct)
*   `ID` (`FString` / `FName`): 一意の行識別子
*   `Speaker` (`FText`): 発言者名
*   `Dialogue` (`FText`): セリフテキスト
*   `ChoiceGroup` (`TArray<FChoiceData>`): 選択肢データリスト（選択肢テキスト、遷移先IDなど）

#### ファイル形式の選定
*   **JSON（推奨）**: 複雑なネスト構造や選択肢分岐、拡張パラメータの記述が容易。
*   **CSV**: スプレッドシート（Excel / Google Sheets）で管理しやすいが、複雑な分岐データには不向き。

---

### Phase 2: データ読み込み & インポート機構 (Data Ingestion)

外部シナリオデータをUE内で扱える形式にロードする仕組みを構築します。

*   **DataTable インポート（静的ロード）**
    *   作成した Struct（`FScenarioRow`）を定義とし、JSON / CSV を UE 上の `DataTable` アセットとしてインポート。
*   **ランタイム外部ファイル読み込み（動的ロード）**
    *   C++ または `Json Blueprint Utilities` を使用し、ビルド後もゲーム実行時に指定フォルダ（`Saved/` や `Content/External/` 等）から直接 `.json` ファイルを読み込んでパースする処理を実装。

---

### Phase 3: ADV管理システムの実装 (Adv Subsystem / Manager)

シナリオの進行状態およびフラグ（変数）を管理する進行管理クラス（`GameInstanceSubsystem` または `ActorComponent`）を作成します。

*   **シナリオ進行ロジック**
    *   現在の行ID / 行インデックスの保持。
    *   入力イベントに応じた「次の行データ」の取得・抽出処理。
*   **フラグ・変数管理**
    *   ゲーム内分岐フラグ（Boolean / Integer / Map）の読み書き。
    *   「`FlagA == true` の場合は ID: 100 へジャンプ」などの条件分岐処理の実装。

---

### Phase 4: UI表示部 (WBP) の作成

データをUIに反映し、プレイヤーの入力を視覚的に描画する画面を構築します。

#### WBP レイアウト構成
*   `NameText` (`TextBlock`): 発言者名を表示
*   `DialogueText` (`TextBlock` / `RichTextBlock`): セリフ本文を表示
*   `CharacterImage` (`Image`): 立ち絵表示用エリア（左右・中央用の複数配置に対応）
*   `ChoiceContainer` (`VerticalBox`): 選択肢ボタンを動的に生成・配置するエリア

#### UI描画・演出ロジック
*   **タイプライターエフェクト**: テキストを1文字ずつ順次表示（Timer または `RichTextBlock` による制御）。
*   **アセットの動的ロード**: `Async Load Asset` を活用し、表示直前に立ち絵・ボイスを非同期ロード。

---

### Phase 5: 入力・イベントトリガー & 拡張機能

操作性向上およびゲームシステムとしての完成度を高める機能を実装します。

*   **プレイヤー入力・送り制御**
    *   マウスクリック / 決定キーによるシナリオ送り。
    *   文字送り中のクリック: **全文即時表示**。
    *   全文表示後のクリック: **次のテキストへ進む**。
*   **ADV機能の拡充**
    *   **バックログ機能**: 既読メッセージの履歴保持および閲覧UI。
    *   **オート / スキップ機能**: タイマーによる自動進行、既読判定フラグによる高速スキップ。
    *   **セーブ / ロードシステム**: `SaveGame` アセットへの「現在のシナリオID」「フラグ状況」の保存と復元。
