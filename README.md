# Pico2UltraHiResUSBDDC (USB Digital Audio Device)

This project is based on the original work by ArqAlice (MIT License).
This firmware has been fully restructured for Raspberry Pi Pico 2 (RP2350). RP2040-based boards are not supported.

GPIO pin assignments have been remapped to maintain compatibility with the original PICO_AUDIO_PACK hardware, with further modifications to enhance audio quality, stability, and compatibility (including Pico Display Pack 2.0 integration).

---

本プロジェクトは、**Raspberry Pi Pico 2 (RP2350)** 上で動作する、USB Audio Class 1.0 に準拠した高音質 USBデジタルオーディオコンバータ（USB-DDC）です。  
USB経由で入力された2ch PCMオーディオ信号に対し、リアルタイムで高品質なアップサンプリング処理を行い、I²Sインターフェイスを通じてDACチップへ出力します。

---

## 🔍 概要
本デバイスは、PCなどのUSBホストからオーディオ信号を受け取り、リアルタイムで最大32倍のアップサンプリングを行い、DACチップへ32bit I²S形式で出力するUSB-DDC（Digital to Digital Converter）です。  
信号処理には Raspberry Pi Pico 2（RP2350）を採用し、マルチコア構成・DMA・PIO を駆使して低遅延・高精度なオーディオ伝送を実現しています。  
Pico Audio Pack および **Pico Display Pack 2.0** の使用を想定し、ピンアサインの最適化と音質チューニングを行っています。さらに、**機器の接続は必ずドーターボード経由で行ってください。** ドーターボード経由で接続されたディスプレイ上に、内蔵FFT関数を用いたリアルタイムのグラフィックアナライザーやレベルメーター、オーディオフォーマット情報を表示する機能を搭載しています。

---

## ✨ 特長
* USB Audio Class 1.0 準拠（OS標準ドライバで動作）
* リアルタイム・FIR/IIRハイブリッドフィルタによる最大32倍アップサンプリング
* 2ch ステレオ PCM 入力（16bit / 24bit）
* **Pico Display Pack 2.0 連携機能**:
  * ソフトウェアFFT関数によるリアルタイムグラフィックアナライザー & レベルメーター表示
  * ※ピンアサインの衝突を防ぐため、カラーLEDおよび各種ボタンは停止状態（非使用）として運用

---

## 📊 表示仕様（Pico Display Pack 2.0）
ディスプレイ上では、以下のリアルタイム情報およびオーディオビジュアライザーが描画されます。
* **L/R レベルメーター**: ステレオ音声の音量レベルをリアルタイム表示
* **L・R 独立 7バンド FFT 表示**: 左右チャンネルそれぞれの周波数特性を7バンドで分離して視覚化
* **オーディオ情報表示**: 現在の「ビットレート」および「サンプリング周波数 (Fs)」をリアルタイム表示
* **描画に関する注意**: 本ファームウェアは**音質優先設計**となっているため、オーディオ処理を最優先した結果として画面の描画にちらつき（フリッカー）が生じる場合がありますが、これは仕様です。

---

## 🎧 対応DACチップ
* TI PCM5102
* ESS ES9038Q2M
* ESS ES9039Q2M

※ PCM5100 は **PICO_AUDIO_PACK 上でのみ動作確認済み**（Pico2 直結では未確認）

---

## 🔈 入力仕様（USB側）
* **オーディオクラス**: USB Audio Class 1.0
* **チャンネル数**: 2ch ステレオ
* **ビット深度**: 16bit / 24bit
* **サンプルレート**: 44.1kHz / 48kHz / 88.2kHz / 96kHz

---

## 🔊 出力仕様（I²S側）
* **フォーマット**: I²S 32bit（左右チャンネル交互）
* **チャンネル数**: 2ch ステレオ
* **ビット深度**: 固定 32bit
* **サンプルレート**: 最大 1536kHz / 1411.2kHz
* **対応出力周波数**: 1536kHz / 1411.2kHz, 768kHz / 705.6kHz, 384kHz / 352.8kHz, 192kHz / 176.4kHz

---

## ⚙ 使用技術・構成
* **ハードウェア**: Raspberry Pi Pico 2 (RP2350)
* **表示・解析デバイス**: Pico Display Pack 2.0 (ドーターボード経由接続)
* **データ転送**: DMA + PIO による I²S 出力
* **マルチコア処理**:
  * **Core0**: USB通信処理 + アップサンプリング処理 + ディスプレイ描画・FFT処理
  * **Core1**: アップサンプリング処理 + DMA + I²S 送信処理
* **アップサンプリング構成**:
  * **Core0**: FIR による 8x 拡張
  * **Core1**: FIR による 2x 拡張 または BiQuad-IIR による 4x 拡張
* **USB制御**: LUFAベースの USB Audio Class 実装
* **タイミング制御**: timer 割り込み + バッファレートに応じたフィードバック制御

---

## 🔧 ビルド・使用方法
1. 機器の接続は**ドーターボード経由**で行ってください。
2. Visual Studio Code 上で Raspberry Pi Pico 拡張機能をインストールする。
3. 本リポジトリをクローンする。
4. VSCode上でビルドを実行する（`build/src` にバイナリが生成されます）。
5. Pico 2 の **BOOTSEL ボタン** を押しながらPCに接続し、ドライブ（`RP2350`）として認識させる。
6. 生成されたファームウェア（`.uf2`）をドラッグ＆ドロップして書き込む。
7. 接続後、OS標準のUSBオーディオデバイスとして自動認識されます。

---

## 🔧 コンフィグレーション
RP2350（Raspberry Pi Pico 2）上の仕様で可能な範囲で、出力ピンアサインおよびアップサンプリング設定を任意に変更できます。  
変更する場合は、`src/common.h` の `"User Configurable"` 項を編集してください。

### ピンアサイン詳細
* **I2S DATA** : `GP9`
* **I2S BCLK** : `GP10`
* **I2S LRCK** : `GP11`
* **I2C SDA**  : `GP6`
* **I2C SCL**  : `GP7`
* **DAC ENABLE** : `GP5`
* **POWERMODE SW** : `GP0`
* **Pico Display (SPI/Ctrl)** : `GP16` - `GP20`
* **Pico Display カラーLED** : ピンアサインの衝突を防ぐため**停止状態（非使用）**に設定
* **Pico Display ボタン (A, B, X, Y)** : **未使用**

### アップサンプリング設定
* `1536kHz / 1411.2kHz` → 8 / 4
* `768kHz / 705.6kHz`   → 8 / 2
* `384kHz / 352.8kHz`   → 8 / 1
* `192kHz / 176.4kHz`   → 4 / 1

---

### ESS DAC固有設定
* `USE_ESS_DAC` を `true` に設定する。
* `KIND_ESS_DAC` に `ES9038Q2M` または `ES9039Q2M` を指定する。  
  ※ `ES9039Q2M` は `1536kHz / 1411.2kHz` 非対応。

---

## 📚 ライセンス
本プロジェクトは MIT License のもとで公開されています。
* 原著作権: Copyright (c) 2025 ArqAlice
* 追加・改変部分: Copyright (c) 2025-2026 Toshimiyu (Pico2 / Pico Audio Pack / Pico Display Pack 2.0 / FFT統合等)

---

## 📝 参考文献
* Interface ラズパイPico DAC特設ページ
* USB Audio Class 1.0 Spec (USB.org)
