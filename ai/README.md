AI pipeline for sensor forecasting

Steps:

1) Install dependencies (prefer a virtualenv) and set `DATABASE_URL` if you want DB integration:

```bash
python -m venv .venv
source .venv/bin/activate  # or .venv\Scripts\activate on Windows
pip install -r ai/requirements.txt
```

2) Create table in Postgres (run `ai/create_table.sql`).

3) Generate synthetic data and optionally push to the DB:

```bash
python ai/generate_synthetic.py --hours 72 --out data.csv --db "$DATABASE_URL"
```

4) Train LSTM model:

```bash
python ai/train_lstm.py --csv data.csv --seq 60 --horizon 30 --epochs 5
```

5) Run FastAPI server:

```bash
export MODEL_PATH=model.h5
export SCALER_PATH=scaler.joblib
export DATABASE_URL="postgresql://user:pass@host:5432/db"
uvicorn ai.app:app --host 0.0.0.0 --port 8000
```

6) Call prediction endpoint by POSTing JSON samples or letting it read from DB.
