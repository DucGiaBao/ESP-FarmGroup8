import os
import argparse
import numpy as np
import pandas as pd
from datetime import datetime
from sklearn.preprocessing import MinMaxScaler
from sklearn.model_selection import train_test_split
import joblib
import tensorflow as tf
from tensorflow import keras
from sqlalchemy import create_engine


def load_data(csv_path=None, database_url=None, limit=None):
    if csv_path:
        df = pd.read_csv(csv_path)
    elif database_url:
        engine = create_engine(database_url)
        query = "SELECT timestamp, temperature_c, humidity, soil_percent FROM sensor_data ORDER BY timestamp"
        if limit:
            query = query + f" LIMIT {int(limit)}"
        df = pd.read_sql(query, engine)
    else:
        raise ValueError("Provide csv_path or database_url")
    df = df.dropna()
    df["timestamp"] = pd.to_datetime(df["timestamp"])
    df = df.sort_values("timestamp")
    return df


def make_sequences(values, seq_len, horizon):
    X, y = [], []
    for i in range(len(values) - seq_len - horizon + 1):
        X.append(values[i : i + seq_len])
        y.append(values[i + seq_len : i + seq_len + horizon, -1])
    return np.array(X), np.array(y)


def build_model(input_shape, horizon):
    model = keras.Sequential()
    model.add(keras.layers.LSTM(64, input_shape=input_shape, return_sequences=False))
    model.add(keras.layers.Dense(64, activation="relu"))
    model.add(keras.layers.Dense(horizon))
    model.compile(optimizer="adam", loss="mse")
    return model


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--csv", help="CSV file with sensor data (timestamp,temperature_c,humidity,soil_percent)")
    parser.add_argument("--db", help="Postgres DATABASE_URL to load sensor_data from")
    parser.add_argument("--seq", type=int, default=60, help="sequence length (timesteps)")
    parser.add_argument("--horizon", type=int, default=30, help="prediction horizon in timesteps")
    parser.add_argument("--model-out", default="model.h5")
    parser.add_argument("--scaler-out", default="scaler.joblib")
    parser.add_argument("--epochs", type=int, default=10)
    args = parser.parse_args()

    df = load_data(csv_path=args.csv, database_url=args.db)
    features = df[["temperature_c", "humidity", "soil_percent"]].to_numpy()

    scaler = MinMaxScaler()
    values = scaler.fit_transform(features)
    joblib.dump(scaler, args.scaler_out)

    X, y = make_sequences(values, args.seq, args.horizon)
    X_train, X_val, y_train, y_val = train_test_split(X, y, test_size=0.15, random_state=42)

    model = build_model((X_train.shape[1], X_train.shape[2]), args.horizon)
    model.fit(X_train, y_train, validation_data=(X_val, y_val), epochs=args.epochs, batch_size=64)
    model.save(args.model_out)
    print(f"Saved model to {args.model_out} and scaler to {args.scaler_out}")


if __name__ == "__main__":
    main()
