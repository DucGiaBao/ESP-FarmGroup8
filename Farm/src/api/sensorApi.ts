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

let cachedTimeColumn: 'created_at' | 'timestamp' | null = null

function normalizeTimestamp(value: string): string {
  const trimmed = value.trim()
  if (!trimmed) {
    return trimmed
  }

  return trimmed.replace(/(\.\d{3})\d+(?=(Z|[+-]\d\d:\d\d)$)/, '$1')
}

function mapSensorRow(row: SensorRow): SensorReading {
  const temperature = row.temperature_c ?? row.temperature ?? row.temp
  const humidity = row.humidity ?? row.hum
  const soilRaw = row.soil_raw ?? null
  const soilPercent = row.soil_percent ?? null
  const createdAtRaw = row.created_at ?? row.timestamp

  if (!createdAtRaw) {
    throw new Error('invalid-row')
  }

  return {
    temperature,
    humidity,
    soilRaw,
    soilPercent,
    createdAt: normalizeTimestamp(createdAtRaw),
  }
}

function toSensorReadingOrNull(row: SensorRow): SensorReading | null {
  try {
    return mapSensorRow(row)
  } catch {
    return null
  }
}

async function getTimeColumn(): Promise<'created_at' | 'timestamp'> {
  if (cachedTimeColumn) {
    return cachedTimeColumn
  }

  const supabase = getSupabaseClient()
  const { data, error } = await supabase.from(SENSOR_TABLE).select('*').limit(1)

  if (error) {
    cachedTimeColumn = 'created_at'
    return cachedTimeColumn
  }

  const row = (data?.[0] as Record<string, unknown> | undefined) ?? undefined
  if (row && 'created_at' in row) {
    cachedTimeColumn = 'created_at'
    return cachedTimeColumn
  }

  if (row && 'timestamp' in row) {
    cachedTimeColumn = 'timestamp'
    return cachedTimeColumn
  }

  cachedTimeColumn = 'created_at'
  return cachedTimeColumn
}

export async function fetchLatestSensorReading(): Promise<SensorReading | null> {
  const supabase = getSupabaseClient()
  const timeColumn = await getTimeColumn()

  const { data, error } = await supabase
    .from(SENSOR_TABLE)
    .select('*')
    .order(timeColumn, { ascending: false })
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
  const timeColumn = await getTimeColumn()

  const { data, error } = await supabase
    .from(SENSOR_TABLE)
    .select('*')
    .order(timeColumn, { ascending: false })
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
    .sort((a, b) => new Date(a.createdAt).getTime() - new Date(b.createdAt).getTime())
}
