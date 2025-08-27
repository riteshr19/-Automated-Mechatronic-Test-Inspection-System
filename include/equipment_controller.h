/**
 * @file equipment_controller.h
 * @brief Main equipment controller interface for the Automated Mechatronic Test Inspection System
 * @author Automated Mechatronic Test System Team
 * @date 2024
 */

#ifndef EQUIPMENT_CONTROLLER_H
#define EQUIPMENT_CONTROLLER_H

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <thread>

namespace MechatronicTest {

/**
 * @brief Equipment status enumeration
 */
enum class EquipmentStatus {
    IDLE,
    RUNNING,
    PAUSED,
    ERROR,
    MAINTENANCE
};

/**
 * @brief Test result structure
 */
struct TestResult {
    std::string test_id;
    std::string device_id;
    bool passed;
    double measurement_value;
    std::string units;
    std::string timestamp;
    std::string notes;
};

/**
 * @brief Equipment configuration structure
 */
struct EquipmentConfig {
    std::string device_port;
    int baud_rate;
    double measurement_tolerance;
    int max_retry_attempts;
    bool enable_logging;
    std::string log_file_path;
};

/**
 * @brief Callback function type for status updates
 */
using StatusCallback = std::function<void(EquipmentStatus, const std::string&)>;

/**
 * @brief Main equipment controller class
 */
class EquipmentController {
public:
    /**
     * @brief Constructor
     */
    EquipmentController();
    
    /**
     * @brief Destructor
     */
    ~EquipmentController();

    /**
     * @brief Initialize the equipment controller
     * @param config Equipment configuration
     * @return true if initialization successful, false otherwise
     */
    bool initialize(const EquipmentConfig& config);

    /**
     * @brief Start the equipment
     * @return true if start successful, false otherwise
     */
    bool start();

    /**
     * @brief Stop the equipment
     * @return true if stop successful, false otherwise
     */
    bool stop();

    /**
     * @brief Pause the equipment
     * @return true if pause successful, false otherwise
     */
    bool pause();

    /**
     * @brief Resume the equipment
     * @return true if resume successful, false otherwise
     */
    bool resume();

    /**
     * @brief Run a test on a device
     * @param device_id Device identifier
     * @param test_parameters Test parameters
     * @return Test result
     */
    TestResult runTest(const std::string& device_id, const std::vector<std::string>& test_parameters);

    /**
     * @brief Get current equipment status
     * @return Current status
     */
    EquipmentStatus getStatus() const;

    /**
     * @brief Get last error message
     * @return Error message
     */
    std::string getLastError() const;

    /**
     * @brief Set status callback function
     * @param callback Callback function
     */
    void setStatusCallback(StatusCallback callback);

    /**
     * @brief Perform equipment calibration
     * @return true if calibration successful, false otherwise
     */
    bool calibrate();

    /**
     * @brief Get equipment health metrics
     * @return Health metrics as key-value pairs
     */
    std::vector<std::pair<std::string, double>> getHealthMetrics() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Hardware interface class
 */
class HardwareInterface {
public:
    virtual ~HardwareInterface() = default;
    virtual bool connect(const std::string& port, int baud_rate) = 0;
    virtual bool disconnect() = 0;
    virtual bool sendCommand(const std::string& command) = 0;
    virtual std::string receiveResponse(int timeout_ms = 1000) = 0;
    virtual bool isConnected() const = 0;
};

/**
 * @brief Factory function to create hardware interface
 * @param interface_type Type of interface ("serial", "ethernet", "usb")
 * @return Unique pointer to hardware interface
 */
std::unique_ptr<HardwareInterface> createHardwareInterface(const std::string& interface_type);

} // namespace MechatronicTest

#endif // EQUIPMENT_CONTROLLER_H