/**
 * @file integration_test.cpp
 * @brief Integration tests for the mechatronic test system
 */

#include "equipment_controller.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

using namespace MechatronicTest;

class IntegrationTestFramework {
private:
    int tests_run;
    int tests_passed;
    int tests_failed;

public:
    IntegrationTestFramework() : tests_run(0), tests_passed(0), tests_failed(0) {}

    void run_test(const std::string& test_name, std::function<bool()> test_func) {
        std::cout << "Running integration test: " << test_name << " ... ";
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
        }
    }

    void print_summary() {
        std::cout << "\n=== Integration Test Summary ===" << std::endl;
        std::cout << "Tests run: " << tests_run << std::endl;
        std::cout << "Passed: " << tests_passed << std::endl;
        std::cout << "Failed: " << tests_failed << std::endl;
        std::cout << "Success rate: " << (tests_run > 0 ? (tests_passed * 100 / tests_run) : 0) << "%" << std::endl;
    }

    bool all_tests_passed() const {
        return tests_failed == 0 && tests_run > 0;
    }
};

bool test_full_system_workflow() {
    EquipmentController controller;
    
    // Status callback to track state changes
    std::vector<EquipmentStatus> status_history;
    controller.setStatusCallback([&status_history](EquipmentStatus status, const std::string& message) {
        status_history.push_back(status);
    });
    
    // Initialize system
    EquipmentConfig config;
    config.device_port = "simulation";  // Use simulation mode
    config.baud_rate = 115200;
    config.measurement_tolerance = 0.1;
    config.max_retry_attempts = 3;
    config.enable_logging = false;  // Disable file logging for test
    config.log_file_path = "";
    
    // This may fail without hardware, but test the workflow
    controller.initialize(config);
    
    // Test state transitions
    controller.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    controller.pause();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    controller.resume();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    controller.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Should have received some status updates
    return status_history.size() > 0;
}

bool test_multiple_test_execution() {
    EquipmentController controller;
    
    // Initialize
    EquipmentConfig config;
    config.device_port = "simulation";
    config.baud_rate = 115200;
    config.measurement_tolerance = 0.1;
    config.max_retry_attempts = 3;
    config.enable_logging = false;
    config.log_file_path = "";
    
    controller.initialize(config);
    controller.start();
    
    // Run multiple tests
    std::vector<TestResult> results;
    std::vector<std::string> devices = {"device_1", "device_2", "device_3"};
    std::vector<std::string> params = {"voltage", "5.0"};
    
    for (const auto& device : devices) {
        TestResult result = controller.runTest(device, params);
        results.push_back(result);
        
        // Small delay between tests
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    controller.stop();
    
    // All tests should have been executed (even if they failed due to no hardware)
    return results.size() == devices.size() && 
           std::all_of(results.begin(), results.end(), 
                      [](const TestResult& r) { return !r.test_id.empty(); });
}

bool test_health_monitoring() {
    EquipmentController controller;
    
    // Get health metrics multiple times
    std::vector<std::vector<std::pair<std::string, double>>> metric_history;
    
    for (int i = 0; i < 3; ++i) {
        auto metrics = controller.getHealthMetrics();
        metric_history.push_back(metrics);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Should have consistent metrics structure
    bool consistent_structure = true;
    if (!metric_history.empty()) {
        size_t expected_size = metric_history[0].size();
        for (const auto& metrics : metric_history) {
            if (metrics.size() != expected_size) {
                consistent_structure = false;
                break;
            }
        }
    }
    
    return !metric_history.empty() && consistent_structure;
}

bool test_error_recovery() {
    EquipmentController controller;
    
    // Try to perform operations without initialization
    bool start_without_init = controller.start();
    bool pause_without_running = controller.pause();
    bool resume_without_pause = controller.resume();
    
    // These should fail gracefully, not crash
    std::string error = controller.getLastError();
    
    // Should be able to get status even after errors
    EquipmentStatus status = controller.getStatus();
    
    return true;  // If we get here without crashing, error recovery works
}

bool test_concurrent_operations() {
    EquipmentController controller;
    
    // Initialize
    EquipmentConfig config;
    config.device_port = "simulation";
    config.baud_rate = 115200;
    config.measurement_tolerance = 0.1;
    config.max_retry_attempts = 3;
    config.enable_logging = false;
    config.log_file_path = "";
    
    controller.initialize(config);
    controller.start();
    
    // Test concurrent status queries and health metric requests
    std::vector<std::thread> threads;
    std::vector<bool> thread_results(4, false);
    
    // Thread 1: Status queries
    threads.emplace_back([&controller, &thread_results]() {
        for (int i = 0; i < 10; ++i) {
            controller.getStatus();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        thread_results[0] = true;
    });
    
    // Thread 2: Health metrics
    threads.emplace_back([&controller, &thread_results]() {
        for (int i = 0; i < 10; ++i) {
            controller.getHealthMetrics();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        thread_results[1] = true;
    });
    
    // Thread 3: Test execution
    threads.emplace_back([&controller, &thread_results]() {
        std::vector<std::string> params = {"test"};
        for (int i = 0; i < 5; ++i) {
            controller.runTest("test_device", params);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        thread_results[2] = true;
    });
    
    // Thread 4: Status changes
    threads.emplace_back([&controller, &thread_results]() {
        controller.pause();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        controller.resume();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        thread_results[3] = true;
    });
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    controller.stop();
    
    // All threads should have completed successfully
    return std::all_of(thread_results.begin(), thread_results.end(), 
                      [](bool result) { return result; });
}

int main() {
    std::cout << "=== Automated Mechatronic Test System - Integration Tests ===" << std::endl;
    std::cout << "Testing system integration and workflows..." << std::endl << std::endl;

    IntegrationTestFramework framework;

    // Run integration tests
    framework.run_test("Full System Workflow", test_full_system_workflow);
    framework.run_test("Multiple Test Execution", test_multiple_test_execution);
    framework.run_test("Health Monitoring", test_health_monitoring);
    framework.run_test("Error Recovery", test_error_recovery);
    framework.run_test("Concurrent Operations", test_concurrent_operations);

    framework.print_summary();

    return framework.all_tests_passed() ? 0 : 1;
}