/**
 * @file equipment_controller.cpp
 * @brief Implementation of the equipment controller
 */

#include "equipment_controller.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace MechatronicTest {

/**
 * @brief Serial hardware interface implementation
 */
class SerialInterface : public HardwareInterface {
private:
#ifdef _WIN32
    HANDLE hSerial;
#else
    int serial_fd;
#endif
    bool connected;

public:
    SerialInterface() : connected(false) {
#ifdef _WIN32
        hSerial = INVALID_HANDLE_VALUE;
#else
        serial_fd = -1;
#endif
    }

    ~SerialInterface() {
        disconnect();
    }

    bool connect(const std::string& port, int baud_rate) override {
#ifdef _WIN32
        std::string portName = "\\\\.\\" + port;
        hSerial = CreateFileA(portName.c_str(),
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);
        
        if (hSerial == INVALID_HANDLE_VALUE) {
            return false;
        }

        DCB dcbSerialParams = {0};
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        
        if (!GetCommState(hSerial, &dcbSerialParams)) {
            CloseHandle(hSerial);
            return false;
        }

        dcbSerialParams.BaudRate = baud_rate;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;

        if (!SetCommState(hSerial, &dcbSerialParams)) {
            CloseHandle(hSerial);
            return false;
        }

        COMMTIMEOUTS timeouts = {0};
        timeouts.ReadIntervalTimeout = 50;
        timeouts.ReadTotalTimeoutConstant = 50;
        timeouts.ReadTotalTimeoutMultiplier = 10;
        timeouts.WriteTotalTimeoutConstant = 50;
        timeouts.WriteTotalTimeoutMultiplier = 10;

        if (!SetCommTimeouts(hSerial, &timeouts)) {
            CloseHandle(hSerial);
            return false;
        }
#else
        serial_fd = open(port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
        if (serial_fd < 0) {
            return false;
        }

        struct termios tty;
        if (tcgetattr(serial_fd, &tty) != 0) {
            close(serial_fd);
            return false;
        }

        cfsetospeed(&tty, B115200);
        cfsetispeed(&tty, B115200);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
        tty.c_iflag &= ~IGNBRK;
        tty.c_lflag = 0;
        tty.c_oflag = 0;
        tty.c_cc[VMIN] = 0;
        tty.c_cc[VTIME] = 5;

        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_cflag |= (CLOCAL | CREAD);
        tty.c_cflag &= ~(PARENB | PARODD);
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
            close(serial_fd);
            return false;
        }
#endif
        connected = true;
        return true;
    }

    bool disconnect() override {
        if (!connected) return true;

#ifdef _WIN32
        if (hSerial != INVALID_HANDLE_VALUE) {
            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;
        }
#else
        if (serial_fd >= 0) {
            close(serial_fd);
            serial_fd = -1;
        }
#endif
        connected = false;
        return true;
    }

    bool sendCommand(const std::string& command) override {
        if (!connected) return false;

        std::string cmd = command + "\r\n";
#ifdef _WIN32
        DWORD bytesWritten;
        return WriteFile(hSerial, cmd.c_str(), cmd.length(), &bytesWritten, NULL) && 
               bytesWritten == cmd.length();
#else
        return write(serial_fd, cmd.c_str(), cmd.length()) == static_cast<ssize_t>(cmd.length());
#endif
    }

    std::string receiveResponse(int timeout_ms) override {
        if (!connected) return "";

        std::string response;
        char buffer[256];
        auto start = std::chrono::steady_clock::now();

        while (std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - start).count() < timeout_ms) {
#ifdef _WIN32
            DWORD bytesRead;
            if (ReadFile(hSerial, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                response += buffer;
                if (response.find('\n') != std::string::npos) break;
            }
#else
            int bytesRead = read(serial_fd, buffer, sizeof(buffer) - 1);
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                response += buffer;
                if (response.find('\n') != std::string::npos) break;
            }
#endif
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // Remove trailing whitespace
        response.erase(std::find_if(response.rbegin(), response.rend(),
                      [](unsigned char ch) { return !std::isspace(ch); }).base(),
                      response.end());

        return response;
    }

    bool isConnected() const override {
        return connected;
    }
};

/**
 * @brief Implementation details for EquipmentController
 */
class EquipmentController::Impl {
public:
    EquipmentStatus status;
    EquipmentConfig config;
    std::string lastError;
    StatusCallback statusCallback;
    std::unique_ptr<HardwareInterface> hardware;
    mutable std::mutex statusMutex;
    std::thread workerThread;
    bool shouldStop;

    Impl() : status(EquipmentStatus::IDLE), shouldStop(false) {}

    ~Impl() {
        if (workerThread.joinable()) {
            shouldStop = true;
            workerThread.join();
        }
    }

    void setStatus(EquipmentStatus newStatus, const std::string& message = "") {
        std::lock_guard<std::mutex> lock(statusMutex);
        status = newStatus;
        if (statusCallback) {
            statusCallback(status, message);
        }
    }

    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

// EquipmentController implementation
EquipmentController::EquipmentController() : pImpl(std::make_unique<Impl>()) {}

EquipmentController::~EquipmentController() = default;

bool EquipmentController::initialize(const EquipmentConfig& config) {
    pImpl->config = config;
    pImpl->hardware = createHardwareInterface("serial");
    
    if (!pImpl->hardware) {
        pImpl->lastError = "Failed to create hardware interface";
        pImpl->setStatus(EquipmentStatus::ERROR, "Failed to create hardware interface");
        return false;
    }

    if (!pImpl->hardware->connect(config.device_port, config.baud_rate)) {
        pImpl->lastError = "Failed to connect to device on port " + config.device_port;
        pImpl->setStatus(EquipmentStatus::IDLE, "Equipment initialized (simulation mode)");
        return false;  // Return false but still set status for callback
    }

    pImpl->setStatus(EquipmentStatus::IDLE, "Equipment initialized successfully");
    return true;
}

bool EquipmentController::start() {
    if (pImpl->status != EquipmentStatus::IDLE && pImpl->status != EquipmentStatus::PAUSED) {
        pImpl->lastError = "Equipment must be in IDLE or PAUSED state to start";
        return false;
    }

    pImpl->setStatus(EquipmentStatus::RUNNING, "Equipment started");
    return true;
}

bool EquipmentController::stop() {
    if (pImpl->status == EquipmentStatus::IDLE) {
        return true;
    }

    pImpl->setStatus(EquipmentStatus::IDLE, "Equipment stopped");
    return true;
}

bool EquipmentController::pause() {
    if (pImpl->status != EquipmentStatus::RUNNING) {
        pImpl->lastError = "Equipment must be running to pause";
        return false;
    }

    pImpl->setStatus(EquipmentStatus::PAUSED, "Equipment paused");
    return true;
}

bool EquipmentController::resume() {
    if (pImpl->status != EquipmentStatus::PAUSED) {
        pImpl->lastError = "Equipment must be paused to resume";
        return false;
    }

    pImpl->setStatus(EquipmentStatus::RUNNING, "Equipment resumed");
    return true;
}

TestResult EquipmentController::runTest(const std::string& device_id, 
                                       const std::vector<std::string>& test_parameters) {
    TestResult result;
    result.device_id = device_id;
    result.test_id = "TEST_" + pImpl->getCurrentTimestamp();
    result.timestamp = pImpl->getCurrentTimestamp();

    if (pImpl->status != EquipmentStatus::RUNNING) {
        result.passed = false;
        result.notes = "Equipment not in running state";
        return result;
    }

    if (!pImpl->hardware || !pImpl->hardware->isConnected()) {
        result.passed = false;
        result.notes = "Hardware not connected";
        return result;
    }

    try {
        // Send test command
        std::string command = "TEST:" + device_id;
        for (const auto& param : test_parameters) {
            command += ":" + param;
        }

        if (!pImpl->hardware->sendCommand(command)) {
            result.passed = false;
            result.notes = "Failed to send test command";
            return result;
        }

        // Receive response
        std::string response = pImpl->hardware->receiveResponse(5000);
        if (response.empty()) {
            result.passed = false;
            result.notes = "No response from device";
            return result;
        }

        // Parse response (simplified format: "RESULT:value:units:status")
        std::istringstream iss(response);
        std::string token;
        std::vector<std::string> tokens;
        
        while (std::getline(iss, token, ':')) {
            tokens.push_back(token);
        }

        if (tokens.size() >= 4 && tokens[0] == "RESULT") {
            result.measurement_value = std::stod(tokens[1]);
            result.units = tokens[2];
            result.passed = (tokens[3] == "PASS");
            result.notes = "Test completed successfully";
        } else {
            result.passed = false;
            result.notes = "Invalid response format: " + response;
        }

    } catch (const std::exception& e) {
        result.passed = false;
        result.notes = "Test execution error: " + std::string(e.what());
    }

    return result;
}

EquipmentStatus EquipmentController::getStatus() const {
    std::lock_guard<std::mutex> lock(pImpl->statusMutex);
    return pImpl->status;
}

std::string EquipmentController::getLastError() const {
    return pImpl->lastError;
}

void EquipmentController::setStatusCallback(StatusCallback callback) {
    pImpl->statusCallback = callback;
}

bool EquipmentController::calibrate() {
    if (pImpl->status != EquipmentStatus::IDLE) {
        pImpl->lastError = "Equipment must be idle for calibration";
        return false;
    }

    pImpl->setStatus(EquipmentStatus::MAINTENANCE, "Calibration in progress");

    // Simulate calibration process
    std::this_thread::sleep_for(std::chrono::seconds(2));

    if (pImpl->hardware && pImpl->hardware->isConnected()) {
        pImpl->hardware->sendCommand("CALIBRATE");
        std::string response = pImpl->hardware->receiveResponse(10000);
        
        if (response.find("CAL_OK") != std::string::npos) {
            pImpl->setStatus(EquipmentStatus::IDLE, "Calibration completed successfully");
            return true;
        }
    }

    pImpl->setStatus(EquipmentStatus::ERROR, "Calibration failed");
    return false;
}

std::vector<std::pair<std::string, double>> EquipmentController::getHealthMetrics() const {
    std::vector<std::pair<std::string, double>> metrics;
    
    // Simulate health metrics
    metrics.push_back({"Temperature", 23.5});
    metrics.push_back({"Vibration", 0.02});
    metrics.push_back({"Power_Consumption", 125.3});
    metrics.push_back({"Uptime_Hours", 1234.5});
    metrics.push_back({"Error_Rate", 0.001});

    return metrics;
}

// Factory function implementation
std::unique_ptr<HardwareInterface> createHardwareInterface(const std::string& interface_type) {
    if (interface_type == "serial") {
        return std::make_unique<SerialInterface>();
    }
    // Add other interface types as needed
    return nullptr;
}

} // namespace MechatronicTest