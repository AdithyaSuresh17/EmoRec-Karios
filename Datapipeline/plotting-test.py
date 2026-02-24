import os

import pandas as pd
import matplotlib.pyplot as plt
from sklearn.preprocessing import StandardScaler, MinMaxScaler
import os




def plot_data(df):
    fig, axs = plt.subplots(2, 4, figsize=(20, 20))
    # Plot IMU features
    axs[0, 0].plot(df['t_ms']/1000, df['ax1'], label='ax1')
    axs[0, 0].plot(df['t_ms']/1000, df['ay1'], label='ay1')
    axs[0, 0].plot(df['t_ms']/1000, df['az1'], label='az1')
    axs[0, 0].set_title('IMU1 Acceleration')

    axs[0, 1].plot(df['t_ms']/1000, df['gx1'], label='gx1')
    axs[0, 1].plot(df['t_ms']/1000, df['gy1'], label='gy1')
    axs[0, 1].plot(df['t_ms']/1000, df['gz1'], label='gz1')
    axs[0, 1].set_title('IMU1 Gyroscope')

    axs[0, 2].plot(df['yaw1'], label='yaw1')
    axs[0, 2].plot(df['pitch1'], label='pitch1')
    axs[0, 2].plot(df['roll1'], label='roll1')
    axs[0, 2].set_title('IMU1 Orientation')

    axs[0, 3].plot(df['t_ms']/1000, df['emg1'], label='emg1')
    axs[0, 3].set_title('EMG1 Signals')

    axs[1, 0].plot(df['t_ms']/1000, df['ax2'], label='ax2')
    axs[1, 0].plot(df['t_ms']/1000, df['ay2'], label='ay2')
    axs[1, 0].plot(df['t_ms']/1000, df['az2'], label='az2')
    axs[1, 0].set_title('IMU2 Acceleration')

    axs[1, 1].plot(df['t_ms']/1000, df['gx2'], label='gx2')
    axs[1, 1].plot(df['t_ms']/1000, df['gy2'], label='gy2')
    axs[1, 1].plot(df['t_ms']/1000, df['gz2'], label='gz2')
    axs[1, 1].set_title('IMU2 Gyroscope')    

    axs[1, 2].plot(df['yaw2'], label='yaw2')
    axs[1, 2].plot(df['pitch2'], label='pitch2')
    axs[1, 2].plot(df['roll2'], label='roll2')
    axs[1, 2].set_title('IMU2 Orientation')

    axs[1, 3].plot(df['t_ms']/1000, df['emg2'], label='emg2')
    axs[1, 3].set_title('EMG2 Signals')

def normalize_and_save(df, filename):
    for col in df.columns:
        df[col] = pd.to_numeric(df[col], errors='coerce')

    df = df.dropna().reset_index(drop=True)

    new_df = df.copy()

    imu_cols = [c for c in df.columns if any(x in c for x in ['ax', 'ay', 'az', 'gx', 'gy', 'gz', 'yaw', 'pitch', 'roll'])]
    scaler_z = StandardScaler()
    new_df[imu_cols] = scaler_z.fit_transform(df[imu_cols])

    emg_cols = ['emg1', 'emg2']
    scaler_minmax = MinMaxScaler()
    new_df[emg_cols] = scaler_minmax.fit_transform(new_df[emg_cols])
    new_df.to_csv('data/processed/' + filename + '_processed.csv', index=False)

    return new_df

def window_and_extract_features(df, condition_label, filename):
    window_size = 15
    step_size = 5

    feature_cols = [c for c in df.columns if any(x in c for x in ['ax', 'ay', 'az', 'gx', 'gy', 'gz', 'yaw', 'pitch', 'roll', 'emg'])]

    window = []

    for start in range(0, len(df) - window_size, step_size):
        end = start + window_size

        window_data = df.iloc[start:end]

        window_features = {}

        window_features['window_start_ms'] = window_data['t_ms'].iloc[0]
        for col in feature_cols:
            window_features[f'{col}_mean'] = window_data[col].mean()
            window_features[f'{col}_std'] = window_data[col].std()
            window_features[f'{col}_max'] = window_data[col].max()
            window_features[f'{col}_min'] = window_data[col].min()

        window_features['Condition'] = condition_label
        window.append(window_features)
    features_df = pd.DataFrame(window)
    features_df.to_csv('data/processed/' + filename + '_features.csv', index=False)
    return features_df

# Load your file
focused_filename = 'louis_focused'
stressed_filename = 'louis_stressed'
# startled_filename = 'louis_startled'

df_focused = pd.read_csv('data/raw/' + focused_filename + '.csv')
df_stressed = pd.read_csv('data/raw/' + stressed_filename + '.csv')
# df_startled = pd.read_csv('data/raw/' + startled_filename + '.csv')

df_focused_features = window_and_extract_features(normalize_and_save(df_focused, focused_filename), 0, focused_filename)
df_stressed_features = window_and_extract_features(normalize_and_save(df_stressed, stressed_filename), 1, stressed_filename)
# df_startled_features = window_and_extract_features(normalize_and_save(df_startled, startled_filename), 2, startled_filename)

master_df_1 = pd.concat([df_focused_features, df_stressed_features], ignore_index=True)
master_df_1.to_csv('data/processed/' + focused_filename.split('_')[0] + '_master.csv', index=False)


# Load your file
focused_filename = 'adi_focused'
stressed_filename = 'adi_stressed'
# startled_filename = 'adi_startled'

df_focused = pd.read_csv('data/raw/' + focused_filename + '.csv')
df_stressed = pd.read_csv('data/raw/' + stressed_filename + '.csv')
# df_startled = pd.read_csv('data/raw/' + startled_filename + '.csv')

df_focused_features = window_and_extract_features(normalize_and_save(df_focused, focused_filename), 0, focused_filename)
df_stressed_features = window_and_extract_features(normalize_and_save(df_stressed, stressed_filename), 1, stressed_filename)
# df_startled_features = window_and_extract_features(normalize_and_save(df_startled, startled_filename), 2, startled_filename)

master_df_2 = pd.concat([df_focused_features, df_stressed_features], ignore_index=True)
master_df_2.to_csv('data/processed/' + focused_filename.split('_')[0] + '_master.csv', index=False)



# Load your file
focused_filename = 'emmanuel_focused'
stressed_filename = 'emmanuel_stressed'
# startled_filename = 'emmanuel_startled'

df_focused = pd.read_csv('data/raw/' + focused_filename + '.csv')
df_stressed = pd.read_csv('data/raw/' + stressed_filename + '.csv')
# df_startled = pd.read_csv('data/raw/' + startled_filename + '.csv')

df_focused_features = window_and_extract_features(normalize_and_save(df_focused, focused_filename), 0, focused_filename)
df_stressed_features = window_and_extract_features(normalize_and_save(df_stressed, stressed_filename), 1, stressed_filename)
# df_startled_features = window_and_extract_features(normalize_and_save(df_startled, startled_filename), 2, startled_filename)

master_df_3 = pd.concat([df_focused_features, df_stressed_features], ignore_index=True)
master_df_3.to_csv('data/processed/' + focused_filename.split('_')[0] + '_master.csv', index=False)

global_master = pd.concat([master_df_1, master_df_2, master_df_3], ignore_index=True)
global_master.to_csv('data/processed/Global_Master_Dataset.csv', index=False)



# Load your file for testing
focused_filename = 'emmanuelchess_focused'
df_focused = pd.read_csv('data/raw/' + focused_filename + '.csv')

df_focused_features_chess = window_and_extract_features(normalize_and_save(df_focused, focused_filename), 0, focused_filename)

df_focused_features_chess.to_csv('data/processed/' + focused_filename.split('_')[0] + '_testchess.csv', index=False)


