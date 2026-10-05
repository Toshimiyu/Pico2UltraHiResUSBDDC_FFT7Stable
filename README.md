Pico2UltraHiResUSBDDC (USB Digital Audio Device)

This project is based on the original work by ArqAlice (MIT License).
Originally developed for the PICO_AUDIO_PACK environment (RP2040), this firmware has been fully restructured for Raspberry Pi Pico2 (RP2350).
It does not support RP2040-based boards.
GPIO pin assignments have been remapped to GP9 / GP10 / GP11 to maintain compatibility with the original PICO_AUDIO_PACK hardware.
Further modifications were made to enhance audio quality, stability, and compatibility.
Operation has been verified with PCM5100 on the PICO_AUDIO_PACK hardware.

本プロジェクトは、RP2350(RaspberryPiPico2)上で動作する、USB Audio Class 1.0 に準拠した USBデジタルオーディオコンバータ（USB-DDC）です。  
USB経由で入力された2ch PCMオーディオ信号に対し、高品質なアップサンプリング処理を行い、I²Sインターフェイスを通じてDACチップへ出力します。

---

🔍 概要  
本デバイスは、PCなどのUSBホストからオーディオ信号を受け取り、リアルタイムで最大32倍のアップサンプリングを行い、DACチップへ32bit I²S形式で出力するUSB-DDC（Digital to Digital Converter）です。  
信号処理には Raspberry Pi Pico2（RP2350）を採用し、マルチコア構成・DMA・PIO を駆使して低遅延・高精度なオーディオ信号伝送を実現しています。
Pico Audio Pack用にピンアサイン変更と音質調整を行っています。
---

✨ 特長  
USB Audio Class 1.0 準拠（OS標準ドライバで動作）  
リアルタイム・FIR/IIRハイブリッドフィルタによる最大32倍アップサンプリング  
2ch ステレオ PCM 入力（16bit / 24bit）  

---

🎧 対応DACチップ  
TI PCM5102  
ESS ES9038Q2M  
ESS ES9039Q2M  

※ PCM5100 は **PICO_AUDIO_PACK 上でのみ動作確認済み**（Pico2 直結では未確認）

---

🔈 入力仕様（USB側）  
オーディオクラス: USB Audio Class 1.0  
チャンネル数: 2ch ステレオ  
ビット深度: 16bit / 24bit  
サンプルレート: 44.1kHz / 48kHz / 88.2kHz / 96kHz

---

🔊 出力仕様（I²S側）  
フォーマット: I²S 32bit（左右チャンネル交互）  
チャンネル数: 2ch ステレオ  
ビット深度: 固定 32bit  
サンプルレート: 最大 1536kHz / 1411.2kHz  
対応出力周波数: 1536kHz / 1411.2kHz, 768kHz / 705.6kHz, 384kHz / 352.8kHz, 192kHz / 176.4kHz

---

⚙ 使用技術・構成  
RP2350（Raspberry Pi Pico2）  
DMA + PIO による I²S 出力  
マルチコア処理  
Core0：USB通信処理 + アップサンプリング処理  
Core1：アップサンプリング処理 + DMA + I²S 送信処理  

アップサンプリング構成  
Core0: FIR による 8x 拡張  
Core1: FIR による 2x 拡張 または BiQuad-IIR による 4x 拡張  

USB制御  
LUFAベースの USB Audio Class 実装  

タイミング制御  
timer 割り込み + バッファレートに応じたフィードバック制御

---

🔧 ビルド・使用方法  
VisualStudioCode 上で RaspberryPiPico 拡張機能をインストール  
本リポジトリをクローン  
VSCode で Compile（build/src に生成）  
Pico2 を BOOTSEL 押しながら接続  
書き込み後、OS標準 USB オーディオデバイスとして認識

---

🔧 コンフィグレーション  
RP2350(RaspberryPiPico2)上の仕様で可能な範囲で、出力ピンアサインおよびアップサンプリング設定を任意に変更できます。  
変更する場合は、 src/common.h の "User Configurable" 項を編集してください。

ピンアサイン（Pico2 専用 / PICO_AUDIO_PACK 互換）  
I2S DATA : GP9  
I2S BCLK : GP10  
I2S LRCK : GP11  

I2C SDA : GP6  
I2C SCL : GP7  
DAC ENABLE : GP5  
POWERMODE SW : GP0  

アップサンプリング設定  
1536kHz/1411.2kHz → 8 / 4  
768kHz/705.6kHz → 8 / 2  
384kHz/352.8kHz → 8 / 1  
192kHz/176.4kHz → 4 / 1

---

ESS DAC Specific  
USE_ESS_DAC を true に設定  
KIND_ESS_DAC に ES9038Q2M または ES9039Q2M を指定  
※ ES9039Q2M は 1536kHz/1411.2kHz 非対応

---

📚 ライセンス  
MIT License  
Copyright 2025 ArqAlice

---

📝 参考文献  
Interface ラズパイPico DAC特設ページ  
USB Audio Class 1.0 Spec (USB.org)
