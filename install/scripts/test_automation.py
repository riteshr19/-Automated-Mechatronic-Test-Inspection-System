#!/usr/bin/env python3
"""
Test Automation Framework for Mechatronic Test System
Provides comprehensive test automation capabilities
"""

import json
import time
import logging
import concurrent.futures
from typing import Dict, List, Optional, Callable, Any
from dataclasses import dataclass, asdict
from pathlib import Path
import matplotlib.pyplot as plt
import numpy as np
from equipment_controller import PythonEquipmentController, TestResult, EquipmentConfig

@dataclass
class TestSuite:
    """Test suite configuration"""
    name: str
    description: str
    tests: List[Dict[str, Any]]
    setup_commands: List[str]
    teardown_commands: List[str]

@dataclass
class TestBatch:
    """Batch test execution configuration"""
    devices: List[str]
    test_suite: TestSuite
    parallel: bool = False
    max_workers: int = 4

class TestAutomationFramework:
    """Main test automation framework"""
    
    def __init__(self, controller: PythonEquipmentController):
        self.controller = controller
        self.test_results: List[TestResult] = []
        self.logger = logging.getLogger(__name__)
        self.test_suites: Dict[str, TestSuite] = {}
        
    def load_test_suite(self, suite_file: str) -> bool:
        """Load test suite from JSON file"""
        try:
            with open(suite_file, 'r') as f:
                suite_data = json.load(f)
            
            suite = TestSuite(
                name=suite_data['name'],
                description=suite_data['description'],
                tests=suite_data['tests'],
                setup_commands=suite_data.get('setup_commands', []),
                teardown_commands=suite_data.get('teardown_commands', [])
            )
            
            self.test_suites[suite.name] = suite
            self.logger.info(f"Loaded test suite: {suite.name}")
            return True
            
        except Exception as e:
            self.logger.error(f"Failed to load test suite {suite_file}: {e}")
            return False
    
    def run_single_test(self, device_id: str, test_config: Dict[str, Any]) -> TestResult:
        """Run a single test"""
        test_name = test_config.get('name', 'Unknown Test')
        test_params = test_config.get('parameters', [])
        expected_range = test_config.get('expected_range', None)
        
        self.logger.info(f"Running test '{test_name}' on device {device_id}")
        
        result = self.controller.run_test(device_id, test_params)
        
        # Additional validation based on expected range
        if expected_range and result.passed:
            min_val, max_val = expected_range
            if not (min_val <= result.measurement_value <= max_val):
                result.passed = False
                result.notes += f" (Outside expected range: {min_val}-{max_val})"
        
        result.notes = f"Test: {test_name}. " + result.notes
        return result
    
    def run_test_suite(self, suite_name: str, device_id: str) -> List[TestResult]:
        """Run complete test suite on a device"""
        if suite_name not in self.test_suites:
            self.logger.error(f"Test suite '{suite_name}' not found")
            return []
        
        suite = self.test_suites[suite_name]
        results = []
        
        self.logger.info(f"Running test suite '{suite_name}' on device {device_id}")
        
        try:
            # Execute setup commands
            for cmd in suite.setup_commands:
                self.logger.info(f"Setup: {cmd}")
                # Here you would execute setup commands
                time.sleep(0.1)
            
            # Run tests
            for test_config in suite.tests:
                result = self.run_single_test(device_id, test_config)
                results.append(result)
                self.test_results.append(result)
                
                # Stop on critical failure if specified
                if not result.passed and test_config.get('critical', False):
                    self.logger.error(f"Critical test failed, stopping suite execution")
                    break
            
            # Execute teardown commands
            for cmd in suite.teardown_commands:
                self.logger.info(f"Teardown: {cmd}")
                time.sleep(0.1)
        
        except Exception as e:
            self.logger.error(f"Error running test suite: {e}")
        
        return results
    
    def run_batch_tests(self, batch: TestBatch) -> Dict[str, List[TestResult]]:
        """Run batch tests on multiple devices"""
        batch_results = {}
        
        if batch.parallel:
            # Parallel execution
            with concurrent.futures.ThreadPoolExecutor(max_workers=batch.max_workers) as executor:
                future_to_device = {
                    executor.submit(self.run_test_suite, batch.test_suite.name, device): device
                    for device in batch.devices
                }
                
                for future in concurrent.futures.as_completed(future_to_device):
                    device = future_to_device[future]
                    try:
                        results = future.result()
                        batch_results[device] = results
                    except Exception as e:
                        self.logger.error(f"Error testing device {device}: {e}")
                        batch_results[device] = []
        else:
            # Sequential execution
            for device in batch.devices:
                results = self.run_test_suite(batch.test_suite.name, device)
                batch_results[device] = results
        
        return batch_results
    
    def generate_test_report(self, output_file: str = None) -> Dict[str, Any]:
        """Generate comprehensive test report"""
        if not self.test_results:
            return {"error": "No test results available"}
        
        # Calculate statistics
        total_tests = len(self.test_results)
        passed_tests = sum(1 for result in self.test_results if result.passed)
        failed_tests = total_tests - passed_tests
        pass_rate = (passed_tests / total_tests) * 100 if total_tests > 0 else 0
        
        # Group results by device
        device_results = {}
        for result in self.test_results:
            if result.device_id not in device_results:
                device_results[result.device_id] = []
            device_results[result.device_id].append(result)
        
        # Calculate device-specific statistics
        device_stats = {}
        for device_id, results in device_results.items():
            device_passed = sum(1 for r in results if r.passed)
            device_total = len(results)
            device_stats[device_id] = {
                "total_tests": device_total,
                "passed": device_passed,
                "failed": device_total - device_passed,
                "pass_rate": (device_passed / device_total) * 100 if device_total > 0 else 0,
                "avg_measurement": np.mean([r.measurement_value for r in results if r.measurement_value is not None])
            }
        
        report = {
            "summary": {
                "total_tests": total_tests,
                "passed": passed_tests,
                "failed": failed_tests,
                "pass_rate": pass_rate,
                "generated_at": time.strftime("%Y-%m-%d %H:%M:%S")
            },
            "device_statistics": device_stats,
            "detailed_results": [asdict(result) for result in self.test_results]
        }
        
        if output_file:
            with open(output_file, 'w') as f:
                json.dump(report, f, indent=2)
            self.logger.info(f"Test report saved to {output_file}")
        
        return report
    
    def plot_test_results(self, output_dir: str = "plots"):
        """Generate plots of test results"""
        Path(output_dir).mkdir(exist_ok=True)
        
        if not self.test_results:
            self.logger.warning("No test results to plot")
            return
        
        # Pass/Fail distribution
        passed = sum(1 for r in self.test_results if r.passed)
        failed = len(self.test_results) - passed
        
        plt.figure(figsize=(8, 6))
        plt.pie([passed, failed], labels=['Passed', 'Failed'], autopct='%1.1f%%', colors=['green', 'red'])
        plt.title('Test Results Distribution')
        plt.savefig(f"{output_dir}/test_distribution.png")
        plt.close()
        
        # Measurement values over time
        timestamps = [r.timestamp for r in self.test_results if r.measurement_value is not None]
        values = [r.measurement_value for r in self.test_results if r.measurement_value is not None]
        
        if values:
            plt.figure(figsize=(12, 6))
            plt.plot(range(len(values)), values, 'b-o', markersize=3)
            plt.title('Measurement Values Over Time')
            plt.xlabel('Test Number')
            plt.ylabel('Measurement Value')
            plt.grid(True, alpha=0.3)
            plt.savefig(f"{output_dir}/measurement_trend.png")
            plt.close()
        
        # Device-wise results
        device_results = {}
        for result in self.test_results:
            if result.device_id not in device_results:
                device_results[result.device_id] = {"passed": 0, "failed": 0}
            
            if result.passed:
                device_results[result.device_id]["passed"] += 1
            else:
                device_results[result.device_id]["failed"] += 1
        
        if len(device_results) > 1:
            devices = list(device_results.keys())
            passed_counts = [device_results[d]["passed"] for d in devices]
            failed_counts = [device_results[d]["failed"] for d in devices]
            
            x = np.arange(len(devices))
            width = 0.35
            
            plt.figure(figsize=(12, 6))
            plt.bar(x - width/2, passed_counts, width, label='Passed', color='green', alpha=0.7)
            plt.bar(x + width/2, failed_counts, width, label='Failed', color='red', alpha=0.7)
            
            plt.xlabel('Devices')
            plt.ylabel('Number of Tests')
            plt.title('Test Results by Device')
            plt.xticks(x, devices, rotation=45)
            plt.legend()
            plt.tight_layout()
            plt.savefig(f"{output_dir}/device_results.png")
            plt.close()
        
        self.logger.info(f"Test plots saved to {output_dir}/")

class SimulationEnvironment:
    """Simulation environment for testing without hardware"""
    
    def __init__(self):
        self.simulated_devices = {}
        self.failure_probability = 0.1
        self.measurement_noise = 0.05
        
    def add_simulated_device(self, device_id: str, nominal_value: float, tolerance: float):
        """Add a simulated device"""
        self.simulated_devices[device_id] = {
            "nominal_value": nominal_value,
            "tolerance": tolerance,
            "failure_mode": False
        }
    
    def set_device_failure(self, device_id: str, failure: bool):
        """Set device failure mode"""
        if device_id in self.simulated_devices:
            self.simulated_devices[device_id]["failure_mode"] = failure
    
    def simulate_measurement(self, device_id: str) -> tuple:
        """Simulate a measurement"""
        if device_id not in self.simulated_devices:
            return 0.0, False, "Device not found in simulation"
        
        device = self.simulated_devices[device_id]
        
        if device["failure_mode"] or np.random.random() < self.failure_probability:
            # Simulate failure
            value = np.random.uniform(-1, 10)  # Random value outside normal range
            return value, False, "Simulated device failure"
        
        # Normal operation
        noise = np.random.normal(0, self.measurement_noise)
        value = device["nominal_value"] + noise
        
        # Check if within tolerance
        passed = abs(value - device["nominal_value"]) <= device["tolerance"]
        
        return value, passed, "Simulation successful"

def main():
    """Main function for test automation"""
    # Setup logging
    logging.basicConfig(level=logging.INFO)
    logger = logging.getLogger(__name__)
    
    # Create equipment controller
    controller = PythonEquipmentController()
    
    # Initialize with default config (simulation mode)
    config = EquipmentConfig(
        device_port="",  # Empty for simulation
        baud_rate=115200,
        measurement_tolerance=0.1,
        max_retry_attempts=3,
        enable_logging=True,
        log_file_path='test_automation.log'
    )
    
    controller.initialize(config)
    
    # Create test automation framework
    framework = TestAutomationFramework(controller)
    
    # Create a sample test suite
    sample_suite = TestSuite(
        name="Basic Electrical Tests",
        description="Basic electrical parameter validation",
        tests=[
            {
                "name": "Voltage Test",
                "parameters": ["voltage", "5.0"],
                "expected_range": [4.8, 5.2],
                "critical": True
            },
            {
                "name": "Current Test", 
                "parameters": ["current", "0.1"],
                "expected_range": [0.08, 0.12],
                "critical": False
            },
            {
                "name": "Resistance Test",
                "parameters": ["resistance", "1000"],
                "expected_range": [950, 1050],
                "critical": False
            }
        ],
        setup_commands=["power_on", "initialize"],
        teardown_commands=["power_off"]
    )
    
    framework.test_suites["basic_electrical"] = sample_suite
    
    # Run tests on multiple devices
    devices = ["device_001", "device_002", "device_003"]
    
    controller.start()
    
    try:
        for device in devices:
            logger.info(f"Testing device: {device}")
            results = framework.run_test_suite("basic_electrical", device)
            
            for result in results:
                logger.info(f"  {result.device_id}: {result.notes} - {'PASS' if result.passed else 'FAIL'}")
    
    finally:
        controller.stop()
        controller.close()
    
    # Generate report and plots
    report = framework.generate_test_report("test_report.json")
    framework.plot_test_results("test_plots")
    
    logger.info(f"Test automation completed. Pass rate: {report['summary']['pass_rate']:.1f}%")

if __name__ == "__main__":
    main()