# save as: log_serial_to_csv.py
# usage:
#   pip install pyserial
#   python3 log_serial_to_csv.py /dev/tty.usbmodemXXXX
# or on Windows:
#   python log_serial_to_csv.py COM3

import csv
import sys
import time
from pathlib import Path

import serial


BAUD = 115200
OUT_DIR = Path("data/raw")  # relative to where you run this script


def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: python3 log_serial_to_csv.py <serial_port> [output_csv_name]")
        sys.exit(1)

    port = sys.argv[1]
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    if len(sys.argv) >= 3:
        out_path = OUT_DIR / sys.argv[2]
    else:
        out_path = OUT_DIR / time.strftime("imu_emg_%Y%m%d_%H%M%S.csv")

    with serial.Serial(port, BAUD, timeout=1) as ser, out_path.open("w", newline="") as f:
        writer = csv.writer(f)

        # 1) Read header from Arduino (it prints once in setup)
        header = None
        start = time.time()
        while header is None:
            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if not line:
                if time.time() - start > 5:
                    raise RuntimeError("No header received. Check port/baud and that Arduino is running.")
                continue
            if "t_ms" in line and "," in line:
                header = [h.strip() for h in line.split(",")]
                writer.writerow(["host_unix_s"] + header)
                f.flush()
                break

        # 2) Stream rows forever until Ctrl+C
        try:
            while True:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                if not line:
                    continue

                parts = [p.strip() for p in line.split(",")]
                if len(parts) != len(header):
                    continue  # skip malformed lines

                writer.writerow([f"{time.time():.6f}"] + parts)
                f.flush()
        except KeyboardInterrupt:
            pass

    print(f"Saved: {out_path}")


if __name__ == "__main__":
    main()
