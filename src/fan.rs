use log::info;
use std::io;
use libgpiod::line;
use libgpiod::line::{Direction, Value};
use libgpiod::request::Request;

pub struct FanController {
    gpio_line: Request,
    line_num: u32,
    is_running: bool,
}

#[allow(clippy::wrong_self_convention)]
impl FanController {
    // Helper method to convert libgpiod errors to io::Error
    fn to_io_error<E>(err: E) -> io::Error 
    where 
        E: std::error::Error + Send + Sync + 'static
    {
        io::Error::new(io::ErrorKind::Other, err)
    }

    pub fn new(chip_path: String, line_num: u32) -> Result<Self, io::Error> {
        let mut settings = line::Settings::new()
            .map_err(Self::to_io_error)?;
        settings
            .set_direction(Direction::Output)
            .map_err(Self::to_io_error)?
            .set_output_value(Value::InActive)
            .map_err(Self::to_io_error)?; // Ensure fan is off initially

        let mut lconfig = line::Config::new()
            .map_err(Self::to_io_error)?;
        lconfig.add_line_settings(&[line_num], settings)
            .map_err(Self::to_io_error)?;

        let mut rconfig = libgpiod::request::Config::new()
            .map_err(Self::to_io_error)?;
        rconfig.set_consumer("toggle-line-value")
            .map_err(Self::to_io_error)?;

        let chip = libgpiod::chip::Chip::open(&chip_path)
            .map_err(Self::to_io_error)?;
        let request = chip.request_lines(Some(&rconfig), &lconfig)
            .map_err(Self::to_io_error)?;

        Ok(FanController {
            gpio_line: request,
            line_num,
            is_running: false,
        })
    }

    pub fn update(&mut self, temperature: f32, threshold: f32, variance: f32) -> Result<(), io::Error> {
        if !self.is_running && temperature >= threshold + variance {
            self.gpio_line.set_value(self.line_num, Value::Active)
                .map_err(Self::to_io_error)?;
            self.is_running = true;
            info!("Setting pin GPIO17 to HIGH");
        } else if self.is_running {
            if temperature <= threshold - variance {
                self.gpio_line.set_value(self.line_num, Value::InActive)
                    .map_err(Self::to_io_error)?;
                self.is_running = false;
                info!("Setting pin GPIO17 to LOW");
            } else {
                info!("Pin GPIO17 is already HIGH");
            }
        }

        Ok(())
    }
}

impl Drop for FanController {
    fn drop(&mut self) {
        // Ensure fan is turned off when program exits
        if let Err(e) = self.gpio_line.set_value(self.line_num, Value::InActive) {
            eprintln!("Failed to turn off fan during shutdown: {}", e);
        }
    }
}
