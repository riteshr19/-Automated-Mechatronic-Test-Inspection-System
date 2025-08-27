using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Text.Json;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging;

namespace MechatronicTestSystem.Core
{
    /// <summary>
    /// Equipment status enumeration
    /// </summary>
    public enum EquipmentStatus
    {
        Idle,
        Running,
        Paused,
        Error,
        Maintenance
    }

    /// <summary>
    /// Test result structure
    /// </summary>
    public class TestResult
    {
        public string TestId { get; set; } = string.Empty;
        public string DeviceId { get; set; } = string.Empty;
        public bool Passed { get; set; }
        public double MeasurementValue { get; set; }
        public string Units { get; set; } = string.Empty;
        public DateTime Timestamp { get; set; }
        public string Notes { get; set; } = string.Empty;
    }

    /// <summary>
    /// Equipment configuration
    /// </summary>
    public class EquipmentConfig
    {
        public string DevicePort { get; set; } = "COM1";
        public int BaudRate { get; set; } = 115200;
        public double MeasurementTolerance { get; set; } = 0.1;
        public int MaxRetryAttempts { get; set; } = 3;
        public bool EnableLogging { get; set; } = true;
        public string LogFilePath { get; set; } = "mechatronic_test.log";
    }

    /// <summary>
    /// Equipment controller interface
    /// </summary>
    public interface IEquipmentController
    {
        EquipmentStatus Status { get; }
        string LastError { get; }
        
        event EventHandler<EquipmentStatusEventArgs>? StatusChanged;
        
        bool Initialize(EquipmentConfig config);
        bool Start();
        bool Stop();
        bool Pause();
        bool Resume();
        Task<TestResult> RunTestAsync(string deviceId, List<string> testParameters);
        bool Calibrate();
        Dictionary<string, double> GetHealthMetrics();
    }

    /// <summary>
    /// Equipment status event arguments
    /// </summary>
    public class EquipmentStatusEventArgs : EventArgs
    {
        public EquipmentStatus Status { get; }
        public string Message { get; }

        public EquipmentStatusEventArgs(EquipmentStatus status, string message)
        {
            Status = status;
            Message = message;
        }
    }

    /// <summary>
    /// Main equipment controller implementation
    /// </summary>
    public class EquipmentController : IEquipmentController, IDisposable
    {
        private readonly ILogger<EquipmentController> _logger;
        private EquipmentConfig? _config;
        private SerialPort? _serialPort;
        private EquipmentStatus _status = EquipmentStatus.Idle;
        private string _lastError = string.Empty;
        private readonly object _lock = new object();

        public EquipmentStatus Status 
        { 
            get 
            { 
                lock (_lock) 
                { 
                    return _status; 
                } 
            } 
        }

        public string LastError 
        { 
            get 
            { 
                lock (_lock) 
                { 
                    return _lastError; 
                } 
            } 
        }

        public event EventHandler<EquipmentStatusEventArgs>? StatusChanged;

        public EquipmentController(ILogger<EquipmentController> logger)
        {
            _logger = logger;
        }

        public bool Initialize(EquipmentConfig config)
        {
            try
            {
                _config = config;
                
                // Try to establish serial connection
                if (!string.IsNullOrEmpty(config.DevicePort))
                {
                    _serialPort = new SerialPort(config.DevicePort, config.BaudRate)
                    {
                        DataBits = 8,
                        StopBits = StopBits.One,
                        Parity = Parity.None,
                        ReadTimeout = 1000,
                        WriteTimeout = 1000
                    };

                    try
                    {
                        _serialPort.Open();
                        _logger.LogInformation($"Connected to {config.DevicePort} at {config.BaudRate} baud");
                        SetStatus(EquipmentStatus.Idle, "Equipment initialized successfully");
                        return true;
                    }
                    catch (Exception ex)
                    {
                        _lastError = $"Failed to connect to device on port {config.DevicePort}: {ex.Message}";
                        _logger.LogWarning(_lastError);
                        SetStatus(EquipmentStatus.Idle, "Equipment initialized (simulation mode)");
                        return false;
                    }
                }

                SetStatus(EquipmentStatus.Idle, "Equipment initialized (simulation mode)");
                return true;
            }
            catch (Exception ex)
            {
                _lastError = $"Initialization failed: {ex.Message}";
                _logger.LogError(ex, "Equipment initialization failed");
                SetStatus(EquipmentStatus.Error, _lastError);
                return false;
            }
        }

        public bool Start()
        {
            lock (_lock)
            {
                if (_status != EquipmentStatus.Idle && _status != EquipmentStatus.Paused)
                {
                    _lastError = "Equipment must be in Idle or Paused state to start";
                    return false;
                }

                SetStatus(EquipmentStatus.Running, "Equipment started");
                return true;
            }
        }

        public bool Stop()
        {
            lock (_lock)
            {
                if (_status == EquipmentStatus.Idle)
                {
                    return true;
                }

                SetStatus(EquipmentStatus.Idle, "Equipment stopped");
                return true;
            }
        }

        public bool Pause()
        {
            lock (_lock)
            {
                if (_status != EquipmentStatus.Running)
                {
                    _lastError = "Equipment must be running to pause";
                    return false;
                }

                SetStatus(EquipmentStatus.Paused, "Equipment paused");
                return true;
            }
        }

        public bool Resume()
        {
            lock (_lock)
            {
                if (_status != EquipmentStatus.Paused)
                {
                    _lastError = "Equipment must be paused to resume";
                    return false;
                }

                SetStatus(EquipmentStatus.Running, "Equipment resumed");
                return true;
            }
        }

        public async Task<TestResult> RunTestAsync(string deviceId, List<string> testParameters)
        {
            var result = new TestResult
            {
                TestId = $"TEST_{DateTimeOffset.UtcNow.ToUnixTimeSeconds()}",
                DeviceId = deviceId,
                Timestamp = DateTime.Now,
                Passed = false
            };

            if (Status != EquipmentStatus.Running)
            {
                result.Notes = "Equipment not in running state";
                return result;
            }

            try
            {
                if (_serialPort?.IsOpen == true)
                {
                    // Send test command
                    var command = $"TEST:{deviceId}:{string.Join(":", testParameters)}\r\n";
                    _serialPort.Write(command);

                    // Wait for response
                    await Task.Delay(100); // Simulate processing time
                    var response = _serialPort.ReadLine();

                    if (!string.IsNullOrEmpty(response))
                    {
                        // Parse response (format: RESULT:value:units:status)
                        var parts = response.Split(':');
                        if (parts.Length >= 4 && parts[0] == "RESULT")
                        {
                            result.MeasurementValue = double.Parse(parts[1]);
                            result.Units = parts[2];
                            result.Passed = parts[3] == "PASS";
                            result.Notes = "Test completed successfully";
                        }
                        else
                        {
                            result.Notes = $"Invalid response format: {response}";
                        }
                    }
                    else
                    {
                        result.Notes = "No response from device";
                    }
                }
                else
                {
                    // Simulation mode
                    var random = new Random();
                    result.MeasurementValue = 5.0 + (random.NextDouble() - 0.5) * 0.2;
                    result.Units = "V";
                    result.Passed = Math.Abs(result.MeasurementValue - 5.0) < 0.1;
                    result.Notes = "Simulation mode test completed";
                }
            }
            catch (Exception ex)
            {
                result.Notes = $"Test execution error: {ex.Message}";
                _logger.LogError(ex, "Test execution failed");
            }

            return result;
        }

        public bool Calibrate()
        {
            lock (_lock)
            {
                if (_status != EquipmentStatus.Idle)
                {
                    _lastError = "Equipment must be idle for calibration";
                    return false;
                }

                SetStatus(EquipmentStatus.Maintenance, "Calibration in progress");

                try
                {
                    // Simulate calibration process
                    System.Threading.Thread.Sleep(2000);

                    if (_serialPort?.IsOpen == true)
                    {
                        _serialPort.WriteLine("CALIBRATE");
                        var response = _serialPort.ReadLine();

                        if (response?.Contains("CAL_OK") == true)
                        {
                            SetStatus(EquipmentStatus.Idle, "Calibration completed successfully");
                            return true;
                        }
                    }
                    else
                    {
                        // Simulation mode
                        SetStatus(EquipmentStatus.Idle, "Calibration completed (simulation)");
                        return true;
                    }

                    SetStatus(EquipmentStatus.Error, "Calibration failed");
                    return false;
                }
                catch (Exception ex)
                {
                    _lastError = $"Calibration error: {ex.Message}";
                    SetStatus(EquipmentStatus.Error, _lastError);
                    return false;
                }
            }
        }

        public Dictionary<string, double> GetHealthMetrics()
        {
            // Simulate health metrics
            var random = new Random();
            return new Dictionary<string, double>
            {
                ["Temperature"] = 23.5 + random.NextDouble() * 2.0,
                ["Vibration"] = 0.02 * random.NextDouble(),
                ["PowerConsumption"] = 125.3 + random.NextDouble() * 10.0,
                ["UptimeHours"] = DateTimeOffset.UtcNow.ToUnixTimeSeconds() / 3600.0,
                ["ErrorRate"] = 0.001 * random.NextDouble()
            };
        }

        private void SetStatus(EquipmentStatus status, string message)
        {
            lock (_lock)
            {
                _status = status;
                _logger.LogInformation($"Status changed to {status}: {message}");
                StatusChanged?.Invoke(this, new EquipmentStatusEventArgs(status, message));
            }
        }

        public void Dispose()
        {
            _serialPort?.Close();
            _serialPort?.Dispose();
        }
    }
}