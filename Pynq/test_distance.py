import time
import board
import busio
import adafruit_vl53l0x

SAMPLE_INTERVAL_S = 0.2
TIMING_BUDGET_US = 50000


def main():
    i2c = busio.I2C(board.SCL, board.SDA)
    sensor = adafruit_vl53l0x.VL53L0X(i2c)
    sensor.measurement_timing_budget = TIMING_BUDGET_US

    print("VL53L0X distance sensor test")
    print(f"Timing budget: {sensor.measurement_timing_budget} us")
    print("Press Ctrl+C to stop.\n")

    count = 0
    total = 0
    dmin = float("inf")
    dmax = 0

    try:
        while True:
            try:
                d = sensor.range
            except OSError as e:
                print(f"I2C read error: {e}")
                time.sleep(SAMPLE_INTERVAL_S)
                continue

            count += 1
            total += d
            if d < dmin:
                dmin = d
            if d > dmax:
                dmax = d
            avg = total / count

            print(
                f"#{count:04d}  distance={d:4d} mm  "
                f"min={dmin:4d}  max={dmax:4d}  avg={avg:6.1f}"
            )
            time.sleep(SAMPLE_INTERVAL_S)
    except KeyboardInterrupt:
        print("\nStopped.")
        if count:
            print(f"Samples: {count}, min={dmin} mm, max={dmax} mm, avg={total/count:.1f} mm")


if __name__ == "__main__":
    main()
