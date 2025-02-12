#include "sgp30_i2c.h"
#include "sht4x_i2c.h"
#include "sensirion_i2c_hal.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>

#define NO_ERROR 0  // Define NO_ERROR as 0

std::string get_current_time() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%d/%m/%y %H:%M:%S", std::localtime(&now_time));
    return std::string(buffer);
}

bool file_exists(const std::string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

std::string get_access_token() {
    // Read the service account JSON key file
    std::ifstream key_file("your_service_account_key.json");
    nlohmann::json key_data;
    key_file >> key_data;

    // Prepare the JWT header and claim set
    nlohmann::json header = {
        {"alg", "RS256"},
        {"typ", "JWT"}
    };

    std::time_t now = std::time(nullptr);
    nlohmann::json claim_set = {
        {"iss", key_data["client_email"]},
        {"scope", "https://www.googleapis.com/auth/spreadsheets"},
        {"aud", "https://oauth2.googleapis.com/token"},
        {"exp", now + 3600},
        {"iat", now}
    };

    // Encode the header and claim set to Base64URL
    std::string header_b64 = nlohmann::json::to_bson(header);
    std::string claim_set_b64 = nlohmann::json::to_bson(claim_set);

    // Create the signature
    std::string unsigned_jwt = header_b64 + "." + claim_set_b64;
    std::string signed_jwt = "YOUR_SIGNED_JWT"; // Sign the JWT using your private key

    // Get the access token using the signed JWT
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, "https://oauth2.googleapis.com/token");
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, ("grant_type=urn:ietf:params:oauth:grant-type:jwt-bearer&assertion=" + signed_jwt).c_str());

        // Set up a callback to get the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
            ((std::string*)userdata)->append((char*)ptr, size * nmemb);
            return size * nmemb;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Perform the request
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
        }

        // Parse the response to get the access token
        nlohmann::json response_json = nlohmann::json::parse(response);
        std::string access_token = response_json["access_token"];

        // Cleanup
        curl_easy_cleanup(curl);

        return access_token;
    }

    return "";
}

void log_to_google_sheet(const std::string& timestamp, float tvoc, float co2, float h2, float ethanol, float temp, float humidity) {
    std::string access_token = get_access_token();
    std::string spreadsheet_id = "YOUR_SPREADSHEET_ID";

    // Prepare the JSON payload
    nlohmann::json payload = {
        {"values", {{timestamp, tvoc, co2, h2, ethanol, temp, humidity}}}
    };

    // Convert JSON payload to string
    std::string payload_str = payload.dump();

    // Initialize cURL
    CURL* curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token).c_str());

        // Set cURL options
        curl_easy_setopt(curl, CURLOPT_URL, ("https://sheets.googleapis.com/v4/spreadsheets/" + spreadsheet_id + "/values/Sheet1!A1:G1:append?valueInputOption=USER_ENTERED").c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());

        // Perform the request
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
        }

        // Cleanup
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
}

int main() {
    // Initialize I2C HAL
    std::cout << "Initializing I2C HAL..." << std::endl;
    sensirion_i2c_hal_init();

    // Initialize SGP30
    std::cout << "Initializing SGP30..." << std::endl;
    SGP30 sgp30;
    if (sgp30.init() != 0) {
        std::cerr << "Failed to initialize SGP30" << std::endl;
        return -1;
    }

    // Initialize SHT40
    std::cout << "Initializing SHT40..." << std::endl;
    sht4x_init(SHT40_I2C_ADDR_44);

    // Open CSV file for appending data
    std::ofstream data_file;
    bool file_exists_flag = file_exists("data_output.csv");
    data_file.open("data_output.csv", std::ios::out | std::ios::app);
    
    // Write header if file is created new
    if (!file_exists_flag) {
        data_file << "Timestamp,SGP30_tVOC_ppb,SGP30_CO2eq_ppm,SGP30_H2_raw,SGP30_Ethanol_raw,SHT40_Temperature,SHT40_Humidity" << std::endl;
    }

    // Measure temperature and humidity from SHT40, and raw signals from SGP30 in an infinite loop
    float temperature = 0.0;
    float humidity = 0.0;
    uint16_t co2_eq_ppm, tvoc_ppb;

    while (true) {
        std::string timestamp = get_current_time();

        // Measure temperature and humidity from SHT40
        if (sht4x_measure_high_precision(&temperature, &humidity) != NO_ERROR) {
            std::cerr << "Error measuring SHT40" << std::endl;
        }

        // Measure from SGP30 with compensation
        sgp30.set_relative_humidity(temperature, humidity);
        if (sgp30.measure()) {
            std::cerr << "Error reading SGP30 measurements" << std::endl;
        } else {
            std::cout << "SGP30 CO2eq: " << sgp30.getCO2() << ", TVOC: " << sgp30.getTVOC() << std::endl;
        }
        co2_eq_ppm = sgp30.getCO2();
        tvoc_ppb = sgp30.getTVOC();

        // Debugging raw H2 and Ethanol values
        if (sgp30.readRaw()) {
            std::cerr << "Error reading SGP30 raw measurements" << std::endl;
        } else {
            std::cout << "SGP30 Raw H2: " << sgp30.getH2_raw() << ", Raw Ethanol: " << sgp30.getEthanol_raw() << std::endl;
        }

        // Write data to CSV file
        data_file << timestamp << ","
                  << tvoc_ppb << "," << co2_eq_ppm << ","
                  << sgp30.getH2_raw() << "," << sgp30.getEthanol_raw() << ","
                  << temperature << "," << humidity
                  << std::endl;

        // Log data to Google Sheet
        log_to_google_sheet(timestamp, tvoc_ppb, co2_eq_ppm, sgp30.getH2_raw(), sgp30.getEthanol_raw(), temperature, humidity);

        // Add a delay between measurements
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Close the CSV file
    data_file.close();

    return 0;
}