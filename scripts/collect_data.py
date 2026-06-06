"""Stream Arduino serial output to a timestamped CSV file.

Usage:
    python scripts/collect_data.py <port> <person> <task>

Example:
    python scripts/collect_data.py /dev/cu.usbmodem21201 louis focus
"""

import argparse
import csv
import time
from pathlib import Path

import serial

BAUD = 115200
OUT_DIR = Path('data/raw')
HEADER = [
    't_ms',
    'ax1', 'ay1', 'az1', 'roll1', 'pitch1', 'yaw1', 'gx1', 'gy1', 'gz1',
    'ax2', 'ay2', 'az2', 'roll2', 'pitch2', 'yaw2', 'gx2', 'gy2', 'gz2',
    'emg1', 'emg2',
]


def parse_args():
    p = argparse.ArgumentParser(description='Log Arduino sensor data to a CSV file.')
    p.add_argument('port', help='Serial port, e.g. /dev/cu.usbmodem21201')
    p.add_argument('person', help='Subject identifier, e.g. louis')
    p.add_argument('task', choices=['baseline', 'focus', 'distract', 'stress'],
                   help='Cognitive task being performed')
    return p.parse_args()


def main():
    args = parse_args()
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    out_path = OUT_DIR / f'{args.person}_7_5min_{args.task}.csv'

    print(f'Recording → {out_path}')
    print('Press Ctrl-C to stop.\n')

    with serial.Serial(args.port, BAUD, timeout=1) as ser, out_path.open('w', newline='') as f:
        try:
            ser.setDTR(False)
            time.sleep(0.2)
            ser.reset_input_buffer()
            ser.setDTR(True)
            time.sleep(2.0)
        except Exception:
            pass

        writer = csv.writer(f)
        writer.writerow(['host_time_s'] + HEADER)
        f.flush()

        try:
            while True:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if not line:
                    continue
                parts = [p.strip() for p in line.split(',')]
                if len(parts) != len(HEADER):
                    continue
                row = [f'{time.time():.6f}'] + parts
                writer.writerow(row)
                f.flush()
                print(','.join(row))
        except KeyboardInterrupt:
            pass

    print(f'\nSaved to {out_path}')


if __name__ == '__main__':
    main()
