import os
import glob
import numpy as np
import pandas as pd
from sklearn.preprocessing import LabelEncoder
from typing import List, Tuple, Optional

# Constants
DATA_DIR = "/content/drive/MyDrive/EmoRecData"
WINDOW_SIZE = 125
STEP_SIZE = 32

## Column definitions (WITH REMOVED ORIENTATION DATA)
## EDIT THIS DEPENDING ON IF USER WANTS TO REMOVE ORIENTATION DATA
all_cols = ['host_time_s', 't_ms', 'ax1', 'ay1', 'az1', 'gx1', 'gy1', 'gz1', 
            'ax2', 'ay2', 'az2', 'gx2', 'gy2', 'gz2', 'emg1', 'emg2']
time_cols = ["host_time_s", "t_ms"]
FEATURE_COLS = [c for c in all_cols if c not in time_cols]

# Label mapping
LABEL_MAP = {
    "baseline": "relaxed",
    "focus": "focused", 
    "distract": "distracted",
    "stress": "stressed"
}

def infer_label_from_filename(fname: str) -> str:
    """Infer label from filename using label_map."""
    lower = fname.lower()
    for key, lab in LABEL_MAP.items():
        if key in lower:
            return lab
    raise ValueError(f"Could not infer label for: {fname}")

def load_raw_features(fpath: str) -> np.ndarray:
    """Load CSV file and extract raw sensor features."""
    df = pd.read_csv(fpath)
    
    # Convert objects to numeric
    for col in FEATURE_COLS:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')
    
    # Return raw features only (n_rows, n_features)
    return df[FEATURE_COLS].dropna().values

def create_windows(data: np.ndarray, label: str) -> Tuple[List[np.ndarray], List[str]]:
    """per-window normalization."""
    X_windows = []
    y_windows = []
    
    start = 0
    while start + WINDOW_SIZE <= len(data):
        window = data[start:start + WINDOW_SIZE]  # (WINDOW_SIZE, n_features)
        
        # Per-window z-score normalization
        window_mean = np.mean(window, axis=0, keepdims=True)
        window_std = np.std(window, axis=0, keepdims=True)
        window_norm = (window - window_mean) / (window_std + 1e-8)
        
        X_windows.append(window_norm)
        y_windows.append(label)
        start += STEP_SIZE
    
    return X_windows, y_windows

def get_training_files(
    data_dir: str = DATA_DIR, 
    ignore_files: Optional[List[str]] = None
) -> List[str]:
    """Get list of training files, excluding specified test files and if input is not given, it just uses my default."""
    if ignore_files is None:
        ignore_files = [
            "focus_test.csv", "stress_w_20sec_baseline.csv", "adi_focused.csv",
            "louis_focused.csv", "louis_stressed.csv", "adi_stressed.csv",
            "adi_7_5min_stress.csv", "adi_7_5min_focus.csv", "adi_7_5min_baseline.csv",
            "adi_7_5min_distract.csv", "louis_7_5min_distract.csv", "louis_7_5min_baseline.csv",
            "louis_7_5min_stress.csv", "louis_7_5min_focus.csv", "emmanuel_7_5min_baseline.csv",
            "emmanuel_7_5min_distract.csv", "emmanuel_7_5min_stress.csv", "emmanuel_7_5min_focus.csv"
        ]
    
    all_files = glob.glob(os.path.join(data_dir, "*.csv"))
    training_files = []
    
    for fpath in all_files:
        fname = os.path.basename(fpath)
        
        if fname in ignore_files:
            print(f"Skipping test file: {fname}")
            continue
        
        if fname.endswith("_backup.csv") or "_backup" in fname:
            print(f"Skipping backup: {fname}")
            continue
        
        training_files.append(fpath)
    
    print(f"Using {len(training_files)} training files (ignored {len(all_files) - len(training_files)} test/backup files)")
    return training_files

def create_training_dataset(
    data_dir: str = DATA_DIR, 
    ignore_files: Optional[List[str]] = None
) -> Tuple[np.ndarray, np.ndarray, LabelEncoder]:
    """Create complete training dataset (X, y, label_encoder)."""
    X_list = []
    y_list = []
    
    training_files = get_training_files(data_dir, ignore_files)
    
    for fpath in training_files:
        fname = os.path.basename(fpath)
        print(f"Processing {fname}...")
        
        # Load raw features
        data = load_raw_features(fpath)
        label = infer_label_from_filename(fname)
        
        # Create normalized windows
        X_windows, y_windows = create_windows(data, label)
        X_list.extend(X_windows)
        y_list.extend(y_windows)
    
    # Stack into arrays
    X = np.stack(X_list, axis=0)
    y = np.array(y_list)
    
    print(f"TRAINING Dataset: X{X.shape}, y{y.shape}")
    print("Labels:", np.unique(y, return_counts=True))
    
    # Create label encoder
    label_encoder = LabelEncoder()
    y_int = label_encoder.fit_transform(y)
    print("Label mapping:", dict(zip(label_encoder.classes_, range(len(label_encoder.classes_)))))
    
    return X, y_int, label_encoder

def normalize_live_window(window_data: np.ndarray) -> np.ndarray:
    """Normalize a single live window (for real-time inference)."""
    window_mean = np.mean(window_data, axis=0, keepdims=True)
    window_std = np.std(window_data, axis=0, keepdims=True)
    return (window_data - window_mean) / (window_std + 1e-8)
