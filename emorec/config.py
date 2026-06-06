WINDOW_SIZE = 125
STEP_SIZE = 25

ALL_COLS = [
    'host_time_s', 't_ms',
    'ax1', 'ay1', 'az1', 'gx1', 'gy1', 'gz1',
    'ax2', 'ay2', 'az2', 'gx2', 'gy2', 'gz2',
    'emg1', 'emg2',
]
TIME_COLS = ['host_time_s', 't_ms']
FEATURE_COLS = [c for c in ALL_COLS if c not in TIME_COLS]

# Columns as received from the BLE Sense / serial stream (includes orientation)
SERIAL_COLS = [
    't_ms',
    'ax1', 'ay1', 'az1', 'roll1', 'pitch1', 'yaw1', 'gx1', 'gy1', 'gz1',
    'ax2', 'ay2', 'az2', 'roll2', 'pitch2', 'yaw2', 'gx2', 'gy2', 'gz2',
    'emg1', 'emg2',
]

LABEL_MAP = {
    'baseline': 'relaxed',
    'focus': 'focused',
    'distract': 'distracted',
    'stress': 'stressed',
}

# Cost matrix rows = true class, cols = predicted class.
# Class order is sklearn LabelEncoder alphabetical: distracted=0, focused=1, relaxed=2, stressed=3.
# Higher values penalise high-cost confusions (e.g. relaxed misclassified as stressed).
PENALTY_GRID = [
    [0.0, 1.0, 1.0, 1.5],  # true: distracted
    [1.0, 0.0, 1.0, 1.5],  # true: focused
    [1.0, 1.5, 0.0, 1.8],  # true: relaxed
    [1.0, 1.0, 1.0, 0.0],  # true: stressed
]
