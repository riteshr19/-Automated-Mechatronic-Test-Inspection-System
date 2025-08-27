/**
 * @file main.cpp
 * @brief Main entry point for the Automated Mechatronic Test Inspection System
 */

#include "equipment_controller.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

using namespace MechatronicTest;

void statusCallback(EquipmentStatus status, const std::string& message) {
    std::string statusStr;
    switch (status) {
        case EquipmentStatus::IDLE: statusStr = "IDLE"; break;
        case EquipmentStatus::RUNNING: statusStr = "RUNNING"; break;
        case EquipmentStatus::PAUSED: statusStr = "PAUSED"; break;
        case EquipmentStatus::ERROR: statusStr = "ERROR"; break;
        case EquipmentStatus::MAINTENANCE: statusStr = "MAINTENANCE"; break;
    }
    std::cout << "[STATUS] " << statusStr << ": " << message << std::endl;
}

void printUsage() {
    std::cout << "Automated Mechatronic Test Inspection System\n";
    std::cout << "Usage: mechatronic_test_system [options]\n";
    std::cout << "Options:\n";
    std::cout << "  -p, --port <port>     Serial port (default: COM1 on Windows, /dev/ttyUSB0 on Linux)\n";
    std::cout << "  -b, --baud <rate>     Baud rate (default: 115200)\n";
    std::cout << "  -t, --test <device>   Run test on specified device\n";
    std::cout << "  -c, --calibrate       Perform equipment calibration\n";
    std::cout << "  -s, --status          Show equipment status\n";
    std::cout << "  -h, --help            Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::cout << "=== Automated Mechatronic Test Inspection System ===" << std::endl;
    std::cout << "Version 1.0.0" << std::endl;
    std::cout << "Copyright 2024 - Mechatronic Test System Team" << std::endl;
    std::cout << std::endl;

    // Default configuration
    EquipmentConfig config;
#ifdef _WIN32
    config.device_port = "COM1";
#else
    config.device_port = "/dev/ttyUSB0";
#endif
    config.baud_rate = 115200;
    config.measurement_tolerance = 0.1;
    config.max_retry_attempts = 3;
    config.enable_logging = true;
    config.log_file_path = "mechatronic_test.log";

    std::string test_device;
    bool run_calibration = false;
    bool show_status = false;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage();
            return 0;
        } else if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) {
                config.device_port = argv[++i];
            } else {
                std::cerr << "Error: Port argument requires a value" << std::endl;
                return 1;
            }
        } else if (arg == "-b" || arg == "--baud") {
            if (i + 1 < argc) {
                config.baud_rate = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: Baud rate argument requires a value" << std::endl;
                return 1;
            }
        } else if (arg == "-t" || arg == "--test") {
            if (i + 1 < argc) {
                test_device = argv[++i];
            } else {
                std::cerr << "Error: Test device argument requires a value" << std::endl;
                return 1;
            }
        } else if (arg == "-c" || arg == "--calibrate") {
            run_calibration = true;
        } else if (arg == "-s" || arg == "--status") {
            show_status = true;
        } else {
            std::cerr << "Error: Unknown argument: " << arg << std::endl;
            printUsage();
            return 1;
        }
    }

    // Create and initialize equipment controller
    EquipmentController controller;
    controller.setStatusCallback(statusCallback);

    std::cout << "Initializing equipment controller..." << std::endl;
    std::cout << "Port: " << config.device_port << std::endl;
    std::cout << "Baud Rate: " << config.baud_rate << std::endl;

    if (!controller.initialize(config)) {
        std::cerr << "Error: Failed to initialize equipment controller: " 
                  << controller.getLastError() << std::endl;
        std::cout << "Note: This is expected if no hardware is connected. Continuing in simulation mode." << std::endl;
    }

    // Show status if requested
    if (show_status) {
        std::cout << "\n=== Equipment Status ===" << std::endl;
        auto status = controller.getStatus();
        std::string statusStr;
        switch (status) {
            case EquipmentStatus::IDLE: statusStr = "IDLE"; break;
            case EquipmentStatus::RUNNING: statusStr = "RUNNING"; break;
            case EquipmentStatus::PAUSED: statusStr = "PAUSED"; break;
            case EquipmentStatus::ERROR: statusStr = "ERROR"; break;
            case EquipmentStatus::MAINTENANCE: statusStr = "MAINTENANCE"; break;
        }
        std::cout << "Status: " << statusStr << std::endl;
        
        auto metrics = controller.getHealthMetrics();
        std::cout << "\nHealth Metrics:" << std::endl;
        for (const auto& metric : metrics) {
            std::cout << "  " << metric.first << ": " << metric.second << std::endl;
        }
    }

    // Run calibration if requested
    if (run_calibration) {
        std::cout << "\n=== Equipment Calibration ===" << std::endl;
        if (controller.calibrate()) {
            std::cout << "Calibration completed successfully!" << std::endl;
        } else {
            std::cerr << "Calibration failed: " << controller.getLastError() << std::endl;
        }
    }

    // Run test if device specified
    if (!test_device.empty()) {
        std::cout << "\n=== Running Test ===" << std::endl;
        std::cout << "Device: " << test_device << std::endl;

        if (!controller.start()) {
            std::cerr << "Error: Failed to start equipment: " << controller.getLastError() << std::endl;
            return 1;
        }

        // Run test with sample parameters
        std::vector<std::string> test_params = {"voltage", "5.0", "current", "0.1"};
        TestResult result = controller.runTest(test_device, test_params);

        std::cout << "\nTest Results:" << std::endl;
        std::cout << "  Test ID: " << result.test_id << std::endl;
        std::cout << "  Device ID: " << result.device_id << std::endl;
        std::cout << "  Timestamp: " << result.timestamp << std::endl;
        std::cout << "  Result: " << (result.passed ? "PASS" : "FAIL") << std::endl;
        std::cout << "  Measurement: " << result.measurement_value << " " << result.units << std::endl;
        std::cout << "  Notes: " << result.notes << std::endl;

        controller.stop();
    }

    // If no specific action requested, run interactive mode
    if (!show_status && !run_calibration && test_device.empty()) {
        std::cout << "\n=== Interactive Mode ===" << std::endl;
        std::cout << "Commands: start, stop, pause, resume, test <device>, calibrate, status, quit" << std::endl;

        std::string command;
        while (true) {
            std::cout << "\n> ";
            std::getline(std::cin, command);

            if (command == "quit" || command == "exit") {
                break;
            } else if (command == "start") {
                if (controller.start()) {
                    std::cout << "Equipment started." << std::endl;
                } else {
                    std::cout << "Failed to start: " << controller.getLastError() << std::endl;
                }
            } else if (command == "stop") {
                if (controller.stop()) {
                    std::cout << "Equipment stopped." << std::endl;
                } else {
                    std::cout << "Failed to stop: " << controller.getLastError() << std::endl;
                }
            } else if (command == "pause") {
                if (controller.pause()) {
                    std::cout << "Equipment paused." << std::endl;
                } else {
                    std::cout << "Failed to pause: " << controller.getLastError() << std::endl;
                }
            } else if (command == "resume") {
                if (controller.resume()) {
                    std::cout << "Equipment resumed." << std::endl;
                } else {
                    std::cout << "Failed to resume: " << controller.getLastError() << std::endl;
                }
            } else if (command.substr(0, 4) == "test") {
                std::string device = "default_device";
                if (command.length() > 5) {
                    device = command.substr(5);
                }
                
                std::vector<std::string> params = {"default", "test"};
                TestResult result = controller.runTest(device, params);
                
                std::cout << "Test " << (result.passed ? "PASSED" : "FAILED") << std::endl;
                std::cout << "Notes: " << result.notes << std::endl;
            } else if (command == "calibrate") {
                if (controller.calibrate()) {
                    std::cout << "Calibration completed." << std::endl;
                } else {
                    std::cout << "Calibration failed: " << controller.getLastError() << std::endl;
                }
            } else if (command == "status") {
                auto status = controller.getStatus();
                std::cout << "Status: ";
                switch (status) {
                    case EquipmentStatus::IDLE: std::cout << "IDLE"; break;
                    case EquipmentStatus::RUNNING: std::cout << "RUNNING"; break;
                    case EquipmentStatus::PAUSED: std::cout << "PAUSED"; break;
                    case EquipmentStatus::ERROR: std::cout << "ERROR"; break;
                    case EquipmentStatus::MAINTENANCE: std::cout << "MAINTENANCE"; break;
                }
                std::cout << std::endl;
            } else if (!command.empty()) {
                std::cout << "Unknown command. Type 'quit' to exit." << std::endl;
            }
        }
    }

    std::cout << "\nShutting down..." << std::endl;
    controller.stop();
    
    return 0;
}