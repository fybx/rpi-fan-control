mod config;
mod temperature;
mod fan;

use config::Config;
use temperature::TemperatureMonitor;
use fan::FanController;
use log::{info, error, LevelFilter};
use simplelog::{WriteLogger, Config as LogConfig};
use std::{fs::File, thread, time::Duration};
use std::process;
use std::sync::{Arc, atomic::{AtomicBool, Ordering}};

fn main() {
    // Set up signal handling for clean shutdown
    let running = Arc::new(AtomicBool::new(true));
    let r = running.clone();

    ctrlc::set_handler(move || {
        info!("Received shutdown signal, exiting...");
        r.store(false, Ordering::SeqCst);
    }).expect("Error setting Ctrl-C handler");

    // Load configuration
    let config_path = "/var/fan_control/config.toml";
    let config = match Config::load(config_path) {
        Ok(config) => config,
        Err(e) => {
            eprintln!("Error loading configuration: {}", e);
            process::exit(1);
        }
    };

    // Initialize logging
    if let Err(e) = WriteLogger::init(
        LevelFilter::Info,
        LogConfig::default(),
        File::create(&config.log_path).unwrap_or_else(|_| {
            eprintln!("Failed to open log file, logging to stdout");
            process::exit(1);
        }),
    ) {
        eprintln!("Failed to initialize logger: {}", e);
        process::exit(1);
    }

    info!("Fan control daemon starting");
    info!("Configuration: temperature path={}, GPIO chip={}, GPIO line={}, threshold={}°C, variance={}°C",
        config.temp_path, config.gpio_chip, config.gpio_line, config.threshold, config.variance);

    // Initialize temperature monitor
    let temp_monitor = TemperatureMonitor::new(config.temp_path);

    // Initialize fan controller
    let mut fan_controller = match FanController::new(config.gpio_chip, config.gpio_line) {
        Ok(controller) => controller,
        Err(e) => {
            error!("Failed to initialize fan controller: {}", e);
            process::exit(1);
        }
    };

    // Main loop
    info!("Entering main control loop");
    while running.load(Ordering::SeqCst) {
        // Read temperature
        match temp_monitor.read_celsius() {
            Ok(temperature) => {
                // Update fan state based on temperature
                if let Err(e) = fan_controller.update(temperature, config.threshold, config.variance) {
                    error!("Failed to update fan state: {}", e);
                }
            },
            Err(e) => {
                error!("Failed to read temperature: {}", e);
            }
        }

        // Sleep for the configured interval
        thread::sleep(Duration::from_millis(config.interval_ms));
    }

    info!("Fan control daemon shutting down");
}