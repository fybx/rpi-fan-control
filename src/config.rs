use serde::Deserialize;
use std::fs;
use std::path::Path;

#[derive(Debug, Deserialize)]
pub struct Config {
    #[serde(default = "default_temp_path")]
    pub temp_path: String,

    #[serde(default = "default_gpio_line")]
    pub gpio_line: u32,

    #[serde(default = "default_gpio_chip")]
    pub gpio_chip: String,

    #[serde(default = "default_log_path")]
    pub log_path: String,

    #[serde(default = "default_threshold")]
    pub threshold: f32,

    #[serde(default = "default_variance")]
    pub variance: f32,

    #[serde(default = "default_interval_ms")]
    pub interval_ms: u64,
}

fn default_temp_path() -> String {
    String::from("/sys/class/thermal/thermal_zone0/temp")
}

fn default_gpio_line() -> u32 { 17 }
fn default_gpio_chip() -> String { String::from("/dev/gpiochip0") }
fn default_log_path() -> String { String::from("/var/log/fan_control.log") }
fn default_threshold() -> f32 { 55.0 }
fn default_variance() -> f32 { 5.0 }
fn default_interval_ms() -> u64 { 1500 }

impl Config {
    pub fn load(config_path: &str) -> Result<Self, Box<dyn std::error::Error>> {
        if Path::new(config_path).exists() {
            let contents = fs::read_to_string(config_path)?;
            let config: Config = toml::from_str(&contents)?;
            Ok(config)
        } else {
            Ok(Config {
                temp_path: default_temp_path(),
                gpio_line: default_gpio_line(),
                gpio_chip: default_gpio_chip(),
                log_path: default_log_path(),
                threshold: default_threshold(),
                variance: default_variance(),
                interval_ms: default_interval_ms(),
            })
        }
    }
}