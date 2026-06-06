import os
import glob
from typing import List, Optional, Tuple

import numpy as np
import pandas as pd
from sklearn.preprocessing import LabelEncoder

from .config import FEATURE_COLS, LABEL_MAP, STEP_SIZE, WINDOW_SIZE


def infer_label(fname: str) -> str:
    lower = os.path.basename(fname).lower()
    for key, label in LABEL_MAP.items():
        if key in lower:
            return label
    raise ValueError(f"Cannot infer label from filename: {fname}")


def load_features(fpath: str) -> np.ndarray:
    df = pd.read_csv(fpath)
    for col in FEATURE_COLS:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')
    return df[FEATURE_COLS].dropna().values


def make_windows(data: np.ndarray, label: str) -> Tuple[List[np.ndarray], List[str]]:
    """Slide a window over data and z-score normalise each window per channel."""
    X, y = [], []
    for start in range(0, len(data) - WINDOW_SIZE + 1, STEP_SIZE):
        window = data[start : start + WINDOW_SIZE]
        mean = window.mean(axis=0, keepdims=True)
        std = window.std(axis=0, keepdims=True)
        X.append((window - mean) / (std + 1e-8))
        y.append(label)
    return X, y


def build_dataset(
    data_dir: str,
    ignore_files: Optional[List[str]] = None,
) -> Tuple[np.ndarray, np.ndarray, LabelEncoder]:
    """Load all CSVs in data_dir, window + normalise them, and return (X, y_int, encoder)."""
    ignore_set = set(ignore_files or [])
    X_all, y_all = [], []

    for fpath in sorted(glob.glob(os.path.join(data_dir, '*.csv'))):
        fname = os.path.basename(fpath)
        if fname in ignore_set or '_backup' in fname:
            print(f'Skipping  {fname}')
            continue
        data = load_features(fpath)
        label = infer_label(fname)
        X_windows, y_windows = make_windows(data, label)
        X_all.extend(X_windows)
        y_all.extend(y_windows)
        print(f'Loaded    {fname}  →  {len(X_windows)} windows  ({label})')

    X = np.stack(X_all)
    y = np.array(y_all)
    encoder = LabelEncoder()
    y_int = encoder.fit_transform(y)
    print(f'\nDataset shape: {X.shape}')
    print(f'Class mapping: {dict(zip(encoder.classes_, range(len(encoder.classes_))))}')
    return X, y_int, encoder


def normalise_window(window: np.ndarray) -> np.ndarray:
    """Per-channel z-score normalisation for a single live inference window."""
    mean = window.mean(axis=0, keepdims=True)
    std = window.std(axis=0, keepdims=True)
    return (window - mean) / (std + 1e-8)
