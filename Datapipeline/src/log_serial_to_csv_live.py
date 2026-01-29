import csv
import sys
import time
from pathlib import Path

import serial

BAUD = 115200
OUT_DIR = Path("data/raw")


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 src/log_serial_to_csv_live.py <serial_port>")
        sys.exit(1)

    port = sys.argv[1]
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    out_path = OUT_DIR / time.strftime("imu_log_%Y%m%d_%H%M%S.csv")

    with serial.Serial(port, BAUD, timeout=1) as ser, out_path.open("w", newline="") as f:
        # Reset/settle helps on macOS + Uno
        try:
            ser.setDTR(False)
            time.sleep(0.2)
            ser.reset_input_buffer()
            ser.setDTR(True)
            time.sleep(1.0)
        except Exception:
            pass

        writer = csv.writer(f)

        # Wait for header
        header = None
        start = time.time()
        while header is None:
            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if not line:
                if time.time() - start > 8:
                    raise RuntimeError("No header received. Check baud/port and that nothing else is using the port.")
                continue
            if "t_ms" in line and "," in line:
                header = [h.strip() for h in line.split(",")]
                writer.writerow(["host_time_s"] + header)
                f.flush()
                print(",".join(["host_time_s"] + header))
                break

        # Stream rows
        try:
            while True:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                if not line:
                    continue

                parts = [p.strip() for p in line.split(",")]
                if len(parts) != len(header):
                    continue

                row = [f"{time.time():.6f}"] + parts
                writer.writerow(row)
                f.flush()

                # Live view (acts like your serial monitor)
                print(",".join(row))

        except KeyboardInterrupt:
            pass

    print(f"Saved CSV to: {out_path}")


if __name__ == "__main__":
    main()
