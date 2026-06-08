CREATE TABLE IF NOT EXISTS sensor_data (
  id SERIAL PRIMARY KEY,
  timestamp TIMESTAMPTZ NOT NULL,
  temperature_c DOUBLE PRECISION,
  humidity DOUBLE PRECISION,
  soil_raw DOUBLE PRECISION,
  soil_percent DOUBLE PRECISION
);
