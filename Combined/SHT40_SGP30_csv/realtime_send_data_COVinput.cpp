#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <curl/curl.h>
#include <unistd.h>

// Function to read access token from file
std::string readAccessToken(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open token file: " << filename << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

// Function to send data to Google Sheets
void sendDataToGoogleSheets(const std::string& accessToken, const std::string& spreadsheetId, const std::string& range, const std::string& data) {
    CURL* curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        std::string url = "https://sheets.googleapis.com/v4/spreadsheets/" + spreadsheetId + "/values/" + range + ":append?valueInputOption=RAW";
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        std::string jsonData = "{\"values\":[[" + data + "]]}";

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code == 404) {
                std::cerr << "Error: Requested entity was not found. Please check the spreadsheet ID and range." << std::endl;
            } else if (response_code != 200 && response_code != 201) {
                std::cerr << "Error: HTTP response code " << response_code << std::endl;
            } else {
                std::cout << "Data sent successfully!" << std::endl;
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

// Function to parse the CSV line
std::string parseCSVLine(const std::string& line) {
    std::istringstream ss(line);
    std::string token;
    std::string data;

    while (std::getline(ss, token, ',')) {
        if (!data.empty()) {
            data += ", ";
        }
        data += "\"" + token + "\"";
    }

    return data;
}

int main() {
    std::string accessToken = readAccessToken("access_token.txt");
    if (accessToken.empty()) {
        std::cerr << "Access token is empty." << std::endl;
        return 1;
    }

    std::string spreadsheetId = "1iIHs10dRot0b4QRTQeNo3rDXepH1v7tXOzFKDAw1wTg";
    std::string range = "input!A1"; // Update this if your sheet name or range is different

    // Start main.cpp with nohup
    system("nohup ./main > main_output.log 2>&1 &");

    // Now read from the log file
    while (true) {
        std::ifstream logFile("main_output.log");
        std::string line;

        if (logFile.is_open()) {
            while (getline(logFile, line)) {
                // Skip initialization lines
                if (line.find("Initializing") != std::string::npos || line.find("SGP30 CO2eq:") != std::string::npos || line.find("SGP30 Raw H2:") != std::string::npos) continue;

                // Parse the CSV line and send data to Google Sheets
                std::string data = parseCSVLine(line);
                if (!data.empty()) {
                    sendDataToGoogleSheets(accessToken, spreadsheetId, range, data);
                }
            }
            logFile.close();
        } else {
            std::cerr << "Unable to open log file" << std::endl;
        }

        // Wait for 1 second before the next reading
        sleep(1);
    }

    return 0;
}