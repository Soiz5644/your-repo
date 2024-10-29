int main() {
    log_debug_info("Program started.");
    sensirion_i2c_hal_init();

    rpi_tca9548a tca9548a;
    if (tca9548a.init(0x70) != 0) {
        std::cerr << "Failed to initialize TCA9548A" << std::endl;
        log_debug_info("Failed to initialize TCA9548A");
        return -1;
    }

    // Initialize SHT35 on channel 3
    std::cout << "Initializing SHT35 on channel 3..." << std::endl;
    log_debug_info("Initializing SHT35 on channel 3...");
    tca9548a.set_channel(3);
    sht3x_init(SHT35_I2C_ADDR_45);
    sht3x_soft_reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Initialize SGP30 on channel 2
    std::cout << "Initializing SGP30 on channel 2..." << std::endl;
    log_debug_info("Initializing SGP30 on channel 2...");
    tca9548a.set_channel(2);
    if (sgp30_init() != 0) {
        std::cerr << "Failed to initialize SGP30" << std::endl;
        log_debug_info("Failed to initialize SGP30");
        return -1;
    }

    uint16_t co2_baseline, tvoc_baseline;
    if (read_baseline(co2_baseline, tvoc_baseline)) {
        sgp30_set_baseline(co2_baseline, tvoc_baseline);
        log_debug_info("Baseline read from file: CO2 = " + std::to_string(co2_baseline) + ", TVOC = " + std::to_string(tvoc_baseline));
    } else {
        log_debug_info("No baseline file found, skipping baseline setting.");
    }

    auto last_baseline_time = std::chrono::steady_clock::now();
    int minutes_elapsed = 0;

    while (minutes_elapsed < HOURS_TO_RUN * 60) { // Change to minutes for testing
        // Measure temperature and humidity from SHT35
        float temperature = 0.0;
        float humidity = 0.0;
        tca9548a.set_channel(3);
        if (sht3x_measure_single_shot(REPEATABILITY_MEDIUM, false, &temperature, &humidity) != NO_ERROR) {
            std::cerr << "Error measuring SHT35" << std::endl;
            log_debug_info("Error measuring SHT35");
        }

        // Set the relative humidity for SGP30
        tca9548a.set_channel(2);
        sgp30_set_relative_humidity(temperature, humidity);
        std::string debug_message = "Temperature: " + std::to_string(temperature) +
                                    " °C, Humidity: " + std::to_string(humidity) + " %";
        std::cout << debug_message << std::endl;
        log_debug_info(debug_message);

        uint16_t co2_eq_ppm, tvoc_ppb;
        if (sgp30_read_measurements(&co2_eq_ppm, &tvoc_ppb) != 0) {
            std::cerr << "Error reading SGP30 measurements" << std::endl;
            log_debug_info("Error reading SGP30 measurements");
        } else {
            std::cout << "SGP30 tVOC: " << tvoc_ppb << " ppb, CO2eq: " << co2_eq_ppm << " ppm" << std::endl;
            log_debug_info("SGP30 tVOC: " + std::to_string(tvoc_ppb) + " ppb, CO2eq: " + std::to_string(co2_eq_ppm) + " ppm");
        }

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::minutes>(now - last_baseline_time).count() >= 1) { // Change to minutes for testing
            uint16_t co2, tvoc;
            if (sgp30_get_baseline(&co2, &tvoc) == 0) {
                log_debug_info("Baseline retrieved: CO2 = " + std::to_string(co2) + ", TVOC = " + std::to_string(tvoc));
                write_baseline(co2, tvoc);
                last_baseline_time = now;
                minutes_elapsed++;
            } else {
                log_debug_info("Failed to retrieve baseline.");
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Test duration elapsed. Shutting down the system." << std::endl;
    log_debug_info("Test duration elapsed. Shutting down the system.");
    system("sudo shutdown -h now");

    return 0;
}