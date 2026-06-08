import os
import json
from typing import List, Optional
import joblib
import numpy as np
import pandas as pd
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from sqlalchemy import create_engine, text
import tensorflow as tf

MODEL_PATH = os.environ.get("MODEL_PATH", "model.h5")
SCALER_PATH = os.environ.get("SCALER_PATH", "scaler.joblib")
DB_URL = os.environ.get("DATABASE_URL")

app = FastAPI()

cors_origins_raw = os.environ.get("CORS_ORIGINS")
if cors_origins_raw:
    cors_origins = [origin.strip() for origin in cors_origins_raw.split(",") if origin.strip()]
else:
    cors_origins = ["http://localhost:5173", "http://127.0.0.1:5173"]

app.add_middleware(
    CORSMiddleware,
    allow_origins=cors_origins,
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)


class Sample(BaseModel):
    timestamp: str
    temperature_c: float
    humidity: float
    soil_percent: float


def load_model_and_scaler():
    model = tf.keras.models.load_model(MODEL_PATH)
    scaler = joblib.load(SCALER_PATH)
    return model, scaler


def fetch_latest_from_db(limit=200):
    if not DB_URL:
        raise RuntimeError("DATABASE_URL not configured")
    engine = create_engine(DB_URL)
    query = f"SELECT timestamp, temperature_c, humidity, soil_percent FROM sensor_data ORDER BY timestamp DESC LIMIT {limit}"
    df = pd.read_sql(text(query), engine)
    if df.empty:
        raise RuntimeError("No data in sensor_data")
    df = df.sort_values("timestamp")
    return df


@app.post("/predict")
def predict(samples: Optional[List[Sample]] = None, seq_len: int = 60, horizon: int = 30):
    model, scaler = load_model_and_scaler()
    if samples is None:
        if not DB_URL:
            raise HTTPException(status_code=400, detail="No samples provided and DATABASE_URL not configured")
        df = fetch_latest_from_db(limit=seq_len)
    else:
        df = pd.DataFrame([s.dict() for s in samples])
        df = df.sort_values("timestamp")

    feats = df[["temperature_c", "humidity", "soil_percent"]].to_numpy()
    scaled = scaler.transform(feats)
    if len(scaled) < seq_len:
        pad = np.repeat(scaled[0:1, :], seq_len - len(scaled), axis=0)
        scaled = np.vstack([pad, scaled])

    seq = scaled[-seq_len:]
    inp = np.expand_dims(seq, axis=0)
    preds = model.predict(inp)
    preds = preds.reshape(-1)
    last_ts = pd.to_datetime(df["timestamp"].iloc[-1])
    freq = (pd.to_datetime(df["timestamp"].iloc[-1]) - pd.to_datetime(df["timestamp"].iloc[-2])) if len(df) > 1 else pd.Timedelta(minutes=1)
    timestamps = [ (last_ts + (i+1)*freq).isoformat() for i in range(len(preds)) ]
    return {"timestamps": timestamps, "soil_percent_pred": preds.tolist()}
