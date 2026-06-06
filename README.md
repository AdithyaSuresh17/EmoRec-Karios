# EmoRec-Karios

A wearable real-time cognitive state classifier. Two BNO055 IMUs and two EMG sensors feed a TinyVGG-style 1D CNN that classifies the wearer's state into one of four classes: **relaxed**, **focused**, **distracted**, or **stressed**.

## System Overview

```
┌─────────────────────┐   UART (38400)   ┌──────────────────────────┐
│   Arduino Uno       │ ───────────────► │  Arduino Nano 33 BLE     │
│   (sensor_hub)      │                  │  (ble_emotion_inference)  │
│                     │                  │                           │
│  2× BNO055 IMU      │                  │  TFLite int8 model        │
│  2× EMG sensor      │                  │  Circular buffer          │
│  EMG calibration    │                  │  Per-window z-score norm  │
└─────────────────────┘                  └──────────────────────────┘
                                                     │ USB serial
                                                     ▼
                                          Emotion + confidence
```

## Repository Structure

```
emorec/                  Python package — model, preprocessing, config
scripts/
  collect_data.py        Stream serial data to CSV  (data collection)
  quantize.py            8-bit quantization via MASE compiler
notebooks/
  train.ipynb            Model training
  live_inference.ipynb   Real-time laptop-side inference
  fft_analysis.ipynb     Signal analysis
  calibration_data.ipynb Generate TFLite calibration arrays
firmware/
  sensor_hub/            Arduino Uno — reads IMU + EMG, sends binary packets
  ble_data_logger/       Arduino Nano BLE Sense — forwards packets to PC (data collection mode)
  ble_emotion_inference/ Arduino Nano BLE Sense — runs on-device TFLite inference
  emg_calibration/       Standalone EMG test / calibration sketch
EmoRecData/              Raw CSV recordings (one file per person per condition)
Ml-Models/               Trained PyTorch checkpoints (.pth)
Previous-Model-Scripts/  Archived earlier experiments
```

## Features (14 channels)

Orientation data (roll/pitch/yaw) was removed — it degraded cross-person generalisation.

| Channel | Source |
|---------|--------|
| ax1, ay1, az1 | IMU 1 linear acceleration |
| gx1, gy1, gz1 | IMU 1 angular velocity |
| ax2, ay2, az2 | IMU 2 linear acceleration |
| gx2, gy2, gz2 | IMU 2 angular velocity |
| emg1, emg2    | Rectified + smoothed EMG |

## Setup

```bash
python -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
```

## Usage

**Collect a new recording session:**
```bash
python scripts/collect_data.py /dev/cu.usbmodem21201 <person> <baseline|focus|distract|stress>
```

**Train the model:**
Open `notebooks/train.ipynb` and run all cells.

**Run live inference (laptop):**
Open `notebooks/live_inference.ipynb`, set `COM_PORT`, and run all cells.

**On-device inference (Arduino):**
Flash `firmware/ble_emotion_inference/` to the Nano 33 BLE Sense and `firmware/sensor_hub/` to the Uno.

## Windowing & Normalisation

- Window size: **125 samples** (~4.4 s at 28.5 Hz)
- Step size: **25 samples** (80% overlap)
- Per-window, per-channel z-score normalisation — identical in Python training and C++ inference

## Model

`EmotionCNN` in `emorec/model.py` — two blocks of paired Conv1d → BatchNorm → ReLU → MaxPool1d, then a single Linear head. Trained with a cost-sensitive loss that penalises high-cost confusions (e.g. predicting stressed when the ground truth is relaxed).

Deployment path: PyTorch `.pth` → TFLite float32 → full int8 quantization → `xxd` C header.
