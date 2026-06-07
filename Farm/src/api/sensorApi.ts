import { getSupabaseClient } from '../lib/supabase'
import type { SensorReading } from '../types/sensor'

interface SensorRow {
  temperature_c?: number | null
  temperature?: number | null
  humidity?: number | null
  soil_raw?: number | null
  soil_percent?: number | null
  temp?: number | null
  hum?: number | null
  created_at?: string | null
  timestamp?: string | null
}

const SENSOR_TABLE = 'sensor_events'

function mapSensorRow(row: SensorRow): SensorReading {
  const temperature = row.temperature_c ?? row.temperature ?? row.temp
  const humidity = row.humidity ?? row.hum
  const soilRaw = row.soil_raw ?? null
  const soilPercent = row.soil_percent ?? null
  const createdAt = row.created_at ?? row.timestamp

  if (!createdAt) {
    throw new Error('invalid-row')
  }

  return {
    temperature,
    humidity,
    soilRaw,
    soilPercent,
    createdAt,
  }
}

function toSensorReadingOrNull(row: SensorRow): SensorReading | null {
  try {
    return mapSensorRow(row)
  } catch {
    return null
  }
}

export async function fetchLatestSensorReading(): Promise<SensorReading | null> {
  const supabase = getSupabaseClient()

  const { data, error } = await supabase
    .from(SENSOR_TABLE)
    .select('temperature_c,humidity,soil_raw,soil_percent,created_at')
    .order('created_at', { ascending: false })
    .limit(1)

  if (error) {
    throw new Error(error.message)
  }

  if (!data || data.length === 0) {
    return null
  }

  return toSensorReadingOrNull(data[0] as SensorRow)
}

export async function fetchSensorHistory(limit = 40): Promise<SensorReading[]> {
  const supabase = getSupabaseClient()

  const { data, error } = await supabase
    .from(SENSOR_TABLE)
    .select('temperature_c,humidity,soil_raw,soil_percent,created_at')
    .order('created_at', { ascending: false })
    .limit(limit)

  if (error) {
    throw new Error(error.message)
  }

  if (!data || data.length === 0) {
    return []
  }

  return (data as SensorRow[])
    .map(toSensorReadingOrNull)
    .filter((row): row is SensorReading => row !== null)
    .reverse()
}
