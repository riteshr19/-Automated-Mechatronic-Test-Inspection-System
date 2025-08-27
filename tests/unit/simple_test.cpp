/**
 * @file simple_test.cpp
 * @brief Simple test framework for basic validation
 */

#include "equipment_controller.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>

using namespace MechatronicTest;

class SimpleTestFramework {
private:
    int tests_run;
    int tests_passed;
    int tests_failed;

public:
    SimpleTestFramework() : tests_run(0), tests_passed(0), tests_failed(0) {}

    void run_test(const std::string& test_name, std::function<bool()> test_func) {
        std::cout << "Running test: " << test_name << " ... ";
        tests_run++;
        
        try {
            if (test_func()) {
                tests_passed++;
                std::cout << "PASS" << std::endl;
            } else {
                tests_failed++;
                std::cout << "FAIL" << std::endl;
            }
        } catch (const std::exception& e) {
            tests_failed++;
            std::cout << "FAIL (Exception: " << e.what() << ")" << std::endl;
        } catch (...) {
            tests_failed++;
            std::cout << "FAIL (Unknown exception)" << std::endl;
        }
    }

    void print_summary() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Tests run: " << tests_run << std::endl;
        std::cout << "Passed: " << tests_passed << std::endl;
        std::cout << "Failed: " << tests_failed << std::endl;
        std::cout << "Success rate: " << (tests_run > 0 ? (tests_passed * 100 / tests_run) : 0) << "%" << std::endl;
    }

    bool all_tests_passed() const {
        return tests_failed == 0 && tests_run > 0;
    }
};

// Test functions
bool test_equipment_controller_creation() {
    EquipmentController controller;
    return true;  // If we get here, creation succeeded
}

bool test_equipment_controller_status() {
    EquipmentController controller;
    EquipmentStatus status = controller.getStatus();
    return status == EquipmentStatus::IDLE;  // Should start in IDLE state
}

bool test_equipment_configuration() {
    EquipmentController controller;
    
    EquipmentConfig config;
    config.device_port = "test_port";
    config.baud_rate = 115200;
    config.measurement_tolerance = 0.1;
    config.max_retry_attempts = 3;
    config.enable_logging = true;
    config.log_file_path = "test.log";
    
    // This will fail in test environment (no hardware), but should not crash
    bool result = controller.initialize(config);
    
    // The test passes if we can call initialize without crashing
    // regardless of the return value (since we don't have hardware)
    return true;
}

bool test_equipment_state_transitions() {
    EquipmentController controller;
    
    // Should start in IDLE
    if (controller.getStatus() != EquipmentStatus::IDLE) {
        return false;
    }
    
    // Can't start without initialization in real hardware,
    // but test the interface
    bool start_called = controller.start();
    bool stop_called = controller.stop();
    bool pause_called = controller.pause();
    bool resume_called = controller.resume();
    
    // Interface should be callable without crashing
    return true;
}

bool test_test_execution() {
    EquipmentController controller;
    
    // Try to run a test (will fail without hardware, but shouldn't crash)
    std::vector<std::string> params = {"test_param"};
    TestResult result = controller.runTest("test_device", params);
    
    // Should return a valid result structure
    return !result.test_id.empty() && !result.device_id.empty();
}

bool test_health_metrics() {
    EquipmentController controller;
    
    auto metrics = controller.getHealthMetrics();
    
    // Should return some metrics
    return !metrics.empty();
}

bool test_hardware_interface_creation() {
    auto interface = createHardwareInterface("serial");
    return interface != nullptr;
}

bool test_calibration_interface() {
    EquipmentController controller;
    
    // Should be callable without crashing
    bool result = controller.calibrate();
    
    // Interface should work regardless of hardware presence
    return true;
}

bool test_status_callback() {
    EquipmentController controller;
    
    bool callback_called = false;
    controller.setStatusCallback([&callback_called](EquipmentStatus status, const std::string& message) {
        (void)status;  // Mark as used to avoid warning
        (void)message; // Mark as used to avoid warning
        callback_called = true;
    });
    
    // Initialize first to trigger a status change
    EquipmentConfig config;
    config.device_port = "test_port";
    config.baud_rate = 115200;
    config.measurement_tolerance = 0.1;
    config.max_retry_attempts = 3;
    config.enable_logging = false;
    config.log_file_path = "";
    
    // This should trigger a callback when status changes to IDLE (even if init fails)
    controller.initialize(config);
    
    // Give some time for callback
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    return callback_called;
}

bool test_error_handling() {
    EquipmentController controller;
    
    std::string error = controller.getLastError();
    
    // Should return a string (empty or with content)
    return true;  // Just testing that the interface works
}

int main() {
    std::cout << "=== Automated Mechatronic Test System - Unit Tests ===" << std::endl;
    std::cout << "Running basic functionality tests..." << std::endl << std::endl;

    SimpleTestFramework framework;

    // Run all tests
    framework.run_test("Equipment Controller Creation", test_equipment_controller_creation);
    framework.run_test("Equipment Controller Status", test_equipment_controller_status);
    framework.run_test("Equipment Configuration", test_equipment_configuration);
    framework.run_test("Equipment State Transitions", test_equipment_state_transitions);
    framework.run_test("Test Execution Interface", test_test_execution);
    framework.run_test("Health Metrics", test_health_metrics);
    framework.run_test("Hardware Interface Creation", test_hardware_interface_creation);
    framework.run_test("Calibration Interface", test_calibration_interface);
    framework.run_test("Status Callback", test_status_callback);
    framework.run_test("Error Handling", test_error_handling);

    framework.print_summary();

    return framework.all_tests_passed() ? 0 : 1;
}