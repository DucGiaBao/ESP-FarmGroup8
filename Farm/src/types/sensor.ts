export interface SensorReading {
  temperature: number | null
  humidity: number | null
  soilRaw: number | null
  soilPercent: number | null
  createdAt: string
}
