#run file while keeping in mind the COM port to which arduino is connected to 
#python3 src/log_serial_to_csv_live.py <USB_Com_Port>

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
    out_path = OUT_DIR / time.strftime("data_log_%d%m%Y_%H%M%S.csv")

    with serial.Serial(port, BAUD, timeout=1) as ser, out_path.open("w", newline="") as f:
        # Reset/settle helps on macOS + Uno
        
        try:
            ser.setDTR(False)
            time.sleep(0.2)
            ser.reset_input_buffer()
            ser.setDTR(True)
            time.sleep(2.0)  
            print(f"Connected to {port}, waiting for data...")  # Add this
        

        except Exception as e:
            print(f"DTR reset failed: {e}")  # Add this
            pass
        writer = csv.writer(f)  # Initialize writer
        header = header = ["t_ms", "ax", "ay", "az", "gx", "gy", "gz", "yaw", "pitch", "roll", "emg_adc"]  # Hardcoded  
        writer.writerow(["host_time_s"] + header)
        f.flush()
        print(",".join(["host_time_s"] + header))


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
