# Automated Mechatronic Test Inspection System
## Technical Manual

### Table of Contents
1. [System Architecture](#system-architecture)
2. [Software Components](#software-components)
3. [Hardware Interfaces](#hardware-interfaces)
4. [API Documentation](#api-documentation)
5. [Configuration](#configuration)
6. [Development Guide](#development-guide)
7. [Testing Framework](#testing-framework)

---

## System Architecture

### Overview
The Automated Mechatronic Test Inspection System follows a modular architecture with the following key components:

```
┌─────────────────────────────────────────────────────────────┐
│                    User Interfaces                         │
├─────────────────┬─────────────────┬─────────────────────────┤
│   C# GUI App    │  Python Scripts │    C++ CLI Tool        │
├─────────────────┴─────────────────┴─────────────────────────┤
│                Equipment Controller                         │
├─────────────────┬─────────────────┬─────────────────────────┤
│  Hardware APIs  │  Test Engine    │   Data Management      │
├─────────────────┼─────────────────┼─────────────────────────┤
│ Serial/USB/Eth  │  Test Sequences │   Result Storage       │
│    Interfaces   │   & Validation  │   & Reporting          │
└─────────────────┴─────────────────┴─────────────────────────┘
```

### Design Principles
- **Modularity**: Each component is independently testable and replaceable
- **Scalability**: System can be extended to support additional devices and interfaces
- **Reliability**: Comprehensive error handling and recovery mechanisms
- **Performance**: Optimized for high-throughput testing scenarios

---

## Software Components

### Core C++ Library
**Location**: `src/cpp/`  
**Purpose**: High-performance equipment control and communication

#### Key Classes
- `EquipmentController`: Main controller interface
- `HardwareInterface`: Abstract interface for hardware communication
- `SerialInterface`: Serial communication implementation
- `TestResult`: Data structure for test results

#### Building
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Python Integration Layer
**Location**: `src/python/`  
**Purpose**: Automation scripting and LabVIEW integration

#### Key Modules
- `equipment_controller.py`: Python wrapper for equipment control
- `test_automation.py`: Test automation framework
- `data_analysis.py`: Result analysis and reporting

#### Dependencies
```bash
pip install -r requirements.txt
```

### C# GUI Application
**Location**: `src/csharp/`  
**Purpose**: User-friendly graphical interface

#### Components
- `EquipmentController.cs`: Core control logic
- `MainWindow.xaml`: Primary user interface
- `TestResultsView.xaml`: Results display and analysis

#### Building
```bash
dotnet build src/csharp/
```

---

## Hardware Interfaces

### Serial Communication
The system supports RS-232/RS-485 serial communication with test devices.

#### Configuration
```yaml
# config.yaml
device_port: "/dev/ttyUSB0"  # Linux
# device_port: "COM1"        # Windows
baud_rate: 115200
data_bits: 8
stop_bits: 1
parity: none
```

#### Protocol
The system uses a simple ASCII protocol:

**Command Format**: `COMMAND:PARAMETER1:PARAMETER2:...\r\n`  
**Response Format**: `RESULT:VALUE:UNITS:STATUS\r\n`

#### Example Commands
```
TEST:DEVICE_001:voltage:5.0     → RESULT:4.98:V:PASS
CALIBRATE                       → CAL_OK
STATUS                          → STATUS:RUNNING:OK
```

### USB Interface
For USB-connected devices, the system uses virtual COM port drivers.

### Ethernet Interface
TCP/IP communication for network-connected test equipment.

```cpp
// Example connection
auto interface = createHardwareInterface("ethernet");
interface->connect("192.168.1.100:502", 0);  // IP:Port
```

---

## API Documentation

### C++ API

#### EquipmentController Class

```cpp
class EquipmentController {
public:
    // Initialize with configuration
    bool initialize(const EquipmentConfig& config);
    
    // Control operations
    bool start();
    bool stop();
    bool pause();
    bool resume();
    
    // Testing
    TestResult runTest(const std::string& deviceId, 
                      const std::vector<std::string>& parameters);
    
    // Status and monitoring
    EquipmentStatus getStatus() const;
    std::vector<std::pair<std::string, double>> getHealthMetrics() const;
    
    // Calibration
    bool calibrate();
    
    // Event handling
    void setStatusCallback(StatusCallback callback);
};
```

#### Example Usage
```cpp
#include "equipment_controller.h"

int main() {
    EquipmentController controller;
    
    EquipmentConfig config;
    config.device_port = "/dev/ttyUSB0";
    config.baud_rate = 115200;
    
    if (controller.initialize(config)) {
        controller.start();
        
        std::vector<std::string> params = {"voltage", "5.0"};
        TestResult result = controller.runTest("DEVICE_001", params);
        
        std::cout << "Test " << (result.passed ? "PASSED" : "FAILED") << std::endl;
        
        controller.stop();
    }
    
    return 0;
}
```

### Python API

#### PythonEquipmentController Class

```python
class PythonEquipmentController:
    def initialize(self, config: EquipmentConfig) -> bool
    def start(self) -> bool
    def stop(self) -> bool
    def pause(self) -> bool
    def resume(self) -> bool
    def run_test(self, device_id: str, test_parameters: List[str]) -> TestResult
    def calibrate(self) -> bool
    def get_health_metrics(self) -> Dict[str, float]
```

#### Example Usage
```python
from equipment_controller import PythonEquipmentController, EquipmentConfig

controller = PythonEquipmentController()

config = EquipmentConfig(
    device_port="/dev/ttyUSB0",
    baud_rate=115200,
    measurement_tolerance=0.1,
    max_retry_attempts=3,
    enable_logging=True
)

if controller.initialize(config):
    controller.start()
    
    result = controller.run_test("DEVICE_001", ["voltage", "5.0"])
    print(f"Test result: {'PASS' if result.passed else 'FAIL'}")
    
    controller.stop()
```

### C# API

#### IEquipmentController Interface

```csharp
public interface IEquipmentController
{
    EquipmentStatus Status { get; }
    string LastError { get; }
    
    event EventHandler<EquipmentStatusEventArgs> StatusChanged;
    
    bool Initialize(EquipmentConfig config);
    bool Start();
    bool Stop();
    bool Pause();
    bool Resume();
    Task<TestResult> RunTestAsync(string deviceId, List<string> testParameters);
    bool Calibrate();
    Dictionary<string, double> GetHealthMetrics();
}
```

#### Example Usage
```csharp
using MechatronicTestSystem.Core;

var controller = new EquipmentController(logger);

var config = new EquipmentConfig
{
    DevicePort = "COM1",
    BaudRate = 115200,
    MeasurementTolerance = 0.1
};

if (controller.Initialize(config))
{
    controller.Start();
    
    var result = await controller.RunTestAsync("DEVICE_001", 
        new List<string> { "voltage", "5.0" });
    
    Console.WriteLine($"Test result: {(result.Passed ? "PASS" : "FAIL")}");
    
    controller.Stop();
}
```

---

## Configuration

### System Configuration File
**Location**: `config/config.yaml`

```yaml
# Equipment Configuration
device_port: "/dev/ttyUSB0"
baud_rate: 115200
measurement_tolerance: 0.1
max_retry_attempts: 3
enable_logging: true
log_file_path: "mechatronic_test.log"

# Test Configuration  
test_timeout_seconds: 30
calibration_timeout_seconds: 60
simulation_mode: false

# Health Monitoring
health_check_interval_seconds: 300
temperature_warning_threshold: 70.0
vibration_warning_threshold: 0.1

# Data Storage
results_directory: "test_results"
backup_enabled: true
max_result_files: 1000
```

### Test Suite Configuration
**Location**: `config/test_suites/`

```json
{
  "name": "Basic Electrical Tests",
  "description": "Basic electrical parameter validation",
  "tests": [
    {
      "name": "Voltage Test",
      "parameters": ["voltage", "5.0"],
      "expected_range": [4.8, 5.2],
      "critical": true
    },
    {
      "name": "Current Test",
      "parameters": ["current", "0.1"],
      "expected_range": [0.08, 0.12],
      "critical": false
    }
  ],
  "setup_commands": ["power_on", "initialize"],
  "teardown_commands": ["power_off"]
}
```

---

## Development Guide

### Building the Project

#### Prerequisites
- CMake 3.16+
- GCC 9+ or MSVC 2019+
- Python 3.8+
- .NET 6.0+

#### Full Build Process
```bash
# Clone repository
git clone <repository-url>
cd Automated-Mechatronic-Test-Inspection-System

# Build C++ components
mkdir build && cd build
cmake ..
make -j$(nproc)
cd ..

# Install Python dependencies
pip install -r requirements.txt

# Build C# components
dotnet build src/csharp/

# Run tests
cd build && make test
cd .. && python -m pytest tests/python/
```

### Code Style Guidelines

#### C++
- Follow Google C++ Style Guide
- Use RAII for resource management
- Prefer smart pointers over raw pointers
- Use const-correctness consistently

#### Python
- Follow PEP 8 style guide
- Use type hints where appropriate
- Write comprehensive docstrings
- Use Black for code formatting

#### C#
- Follow Microsoft C# conventions
- Use async/await for I/O operations
- Implement proper exception handling
- Use XML documentation comments

### Adding New Device Support

1. **Create Hardware Interface**
   ```cpp
   class NewDeviceInterface : public HardwareInterface {
       bool connect(const std::string& port, int baud_rate) override;
       bool sendCommand(const std::string& command) override;
       std::string receiveResponse(int timeout_ms) override;
   };
   ```

2. **Register in Factory**
   ```cpp
   std::unique_ptr<HardwareInterface> createHardwareInterface(const std::string& type) {
       if (type == "new_device") {
           return std::make_unique<NewDeviceInterface>();
       }
       // ... existing types
   }
   ```

3. **Add Configuration Support**
   ```yaml
   # config.yaml
   new_device:
     connection_string: "device_specific_config"
     parameters:
       param1: value1
   ```

---

## Testing Framework

### Unit Tests
**Location**: `tests/unit/`

#### C++ Tests
```bash
cd build
make test
```

#### Python Tests
```bash
python -m pytest tests/python/ -v
```

### Integration Tests
**Location**: `tests/integration/`

Integration tests verify end-to-end system functionality:
- Hardware communication
- Test sequence execution
- Data storage and retrieval
- Error handling and recovery

### Simulation Environment
The system includes a comprehensive simulation mode for:
- Training new operators
- Testing without hardware
- Validating software changes
- Automated CI/CD testing

#### Enabling Simulation Mode
```yaml
# config.yaml
simulation_mode: true
```

### Performance Testing
Performance benchmarks ensure the system meets throughput requirements:
- Test execution time
- Data processing speed
- Memory usage optimization
- Concurrent operation handling

---

## Troubleshooting

### Common Development Issues

#### Build Errors
```bash
# Missing dependencies
sudo apt-get install build-essential cmake libssl-dev

# CMake version too old
wget https://cmake.org/files/v3.20/cmake-3.20.0.tar.gz
# ... build and install
```

#### Runtime Issues
```bash
# Check logs
tail -f mechatronic_test.log

# Verify permissions
sudo chmod 666 /dev/ttyUSB0

# Test communication
python src/python/equipment_controller.py --status
```

### Debugging Tips
- Use debug builds for development: `cmake -DCMAKE_BUILD_TYPE=Debug ..`
- Enable verbose logging in configuration
- Use system monitoring tools to check resource usage
- Implement comprehensive error logging in custom code

---

*Document Version: 1.0*  
*Last Updated: 2024*  
*For technical support, contact the development team*