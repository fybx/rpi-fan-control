use std::fs;
use std::io;

pub struct TemperatureMonitor {
    temp_path: String,
}

impl TemperatureMonitor {
    pub fn new(temp_path: String) -> Self {
        TemperatureMonitor { temp_path }
    }

    pub fn read_celsius(&self) -> Result<f32, io::Error> {
        let temp_str = fs::read_to_string(&self.temp_path)?;
        let temp_millicelsius = temp_str.trim().parse::<u32>()
            .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))?;

        // Convert millidegrees to degrees
        Ok(temp_millicelsius as f32 / 1000.0)
    }
}