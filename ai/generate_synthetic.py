import os
import argparse
from datetime import datetime, timedelta
import numpy as np
import pandas as pd
from sqlalchemy import create_engine, text


def generate_series(start, periods, freq_minutes=1, seed=0):
    np.random.seed(seed)
    idx = pd.date_range(start=start, periods=periods, freq=f"{freq_minutes}min")
    hours = (idx.hour + idx.minute / 60.0).to_numpy()
    base_temp = 22 + 6 * np.sin((hours - 6) / 24 * 2 * np.pi)
    temp = base_temp + np.random.normal(0, 0.8, size=periods)
    humidity = 80 - (temp - 15) * 1.2 + np.random.normal(0, 3.0, size=periods)
    humidity = np.clip(humidity, 10, 100)

    soil = np.empty(periods)
    soil[0] = 60 + np.random.normal(0, 2)
    for i in range(1, periods):
        evap = 0.0005 * np.maximum(0, temp[i] - 15) + 0.0003 * (100 - humidity[i])
        soil[i] = soil[i - 1] - evap * 1000 + np.random.normal(0, 0.02)
        if np.random.rand() < 0.001:
            soil[i] += 15 + np.random.rand() * 10
        soil[i] = np.clip(soil[i], 0, 100)

    soil_raw = 300 + (100 - soil) * 4 + np.random.normal(0, 5, size=periods)
    soil_percent = soil

    df = pd.DataFrame({
        "timestamp": idx.astype(str),
        "temperature_c": temp,
        "humidity": humidity,
        "soil_raw": soil_raw,
        "soil_percent": soil_percent,
    })
    return df


def push_to_db(df, database_url):
    engine = create_engine(database_url)
    with engine.begin() as conn:
        for _, row in df.iterrows():
            conn.execute(
                text(
                    "INSERT INTO sensor_data (timestamp, temperature_c, humidity, soil_raw, soil_percent) VALUES (:ts, :t, :h, :sr, :sp)"
                ),
                {"ts": row["timestamp"], "t": float(row["temperature_c"]), "h": float(row["humidity"]), "sr": float(row["soil_raw"]), "sp": float(row["soil_percent"])},
            )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--start", default=datetime.utcnow().isoformat(), help="start timestamp")
    parser.add_argument("--hours", type=int, default=48, help="how many hours to simulate")
    parser.add_argument("--freq", type=int, default=1, help="minutes between samples")
    parser.add_argument("--out", default="synthetic_sensor.csv", help="output CSV path")
    parser.add_argument("--db", default=os.environ.get("DATABASE_URL"), help="Postgres DATABASE_URL to push into")
    args = parser.parse_args()

    periods = int(args.hours * 60 / args.freq)
    df = generate_series(args.start, periods, freq_minutes=args.freq)
    df.to_csv(args.out, index=False)
    print(f"Wrote {len(df)} rows to {args.out}")
    if args.db:
        push_to_db(df, args.db)
        print("Pushed data to database")


if __name__ == "__main__":
    main()
