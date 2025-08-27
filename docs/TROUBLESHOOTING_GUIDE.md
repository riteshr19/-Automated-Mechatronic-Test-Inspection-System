# Troubleshooting Guide
## Automated Mechatronic Test Inspection System

### Quick Reference

| Issue Type | Quick Solution | Detailed Guide |
|------------|----------------|----------------|
| Build Errors | Check dependencies and CMake version | [Build Issues](#build-issues) |
| Connection Errors | Verify hardware connections and permissions | [Hardware Issues](#hardware-connection-issues) |
| Test Failures | Review test logs and calibration | [Test Issues](#test-execution-issues) |
| Performance Issues | Check system resources and configuration | [Performance Issues](#performance-optimization) |

### Build Issues

#### CMake Configuration Errors

![Build Process](images/build/build_process.png)

**Problem:** CMake fails during configuration
```
CMake Error: The source directory does not exist
```

**Solution:**
```bash
# Ensure you're in the correct directory
cd /path/to/-Automated-Mechatronic-Test-Inspection-System
mkdir build && cd build
cmake ..
```

**Problem:** CMake version too old
```
CMake Error: CMake 3.16 or higher is required
```

**Solution:**
```bash
# Ubuntu/Debian - Update CMake
sudo apt update
sudo apt install cmake

# Or install latest CMake manually
wget https://cmake.org/files/v3.20/cmake-3.20.0.tar.gz
tar -xzf cmake-3.20.0.tar.gz
cd cmake-3.20.0
./configure
make -j$(nproc)
sudo make install
```

#### Compilation Errors

**Problem:** Compiler not found or incompatible
```
The CXX compiler identification is unknown
```

**Solution:**
```bash
# Install development tools
# Ubuntu/Debian:
sudo apt install build-essential

# CentOS/RHEL:
sudo yum groupinstall "Development Tools"

# Set specific compiler if needed
export CXX=g++
export CC=gcc
cmake ..
```

**Problem:** Missing dependencies
```
fatal error: 'thread' file not found
```

**Solution:**
```bash
# Install threading support
sudo apt install libpthread-stubs0-dev

# For OpenCV support (optional)
sudo apt install libopencv-dev
```

### Hardware Connection Issues

#### Serial/USB Device Problems

![CLI Status Output](images/cli/cli_help_status.png)

**Problem:** Permission denied accessing device
```
Error: Failed to initialize equipment controller: Failed to connect to device on port /dev/ttyUSB0
```

**Solution:**
```bash
# Check device permissions
ls -la /dev/ttyUSB*

# Fix permissions temporarily
sudo chmod 666 /dev/ttyUSB0

# Fix permissions permanently
sudo usermod -a -G dialout $USER
# Log out and log back in
```

**Problem:** Device not detected
```
Error: Device not found on any available ports
```

**Solution:**
```bash
# List available devices
ls /dev/tty*

# Check USB devices
lsusb

# Test with different port
mechatronic_test_system --port /dev/ttyUSB1 --status

# Windows - check Device Manager for COM port assignments
```

#### Network Connection Issues

**Problem:** Ethernet device not responding
```
Error: Timeout connecting to device at IP 192.168.1.100
```

**Solution:**
```bash
# Test network connectivity
ping 192.168.1.100

# Check if port is open
telnet 192.168.1.100 502

# Verify network configuration
ifconfig  # Linux/macOS
ipconfig  # Windows
```

### Test Execution Issues

![Test Execution Output](images/testing/test_execution_output.png)

#### Test Failures

**Problem:** Test fails with calibration errors
```
Test failed: Calibration out of range
Expected: 23.5 ± 0.1°C
Actual: 25.2°C
```

**Solution:**
```bash
# Run calibration sequence
mechatronic_test_system --calibrate

# Check calibration certificates
ls -la /etc/mechatronic/calibration/

# Verify environmental conditions
# Temperature should be stable within ±1°C
# Humidity should be within specified range
```

**Problem:** Intermittent test failures
```
Test passed: 85%
Test failed: 15%
```

**Solution:**
```bash
# Check for loose connections
# Verify power supply stability
# Review test logs for patterns

# Run extended diagnostics
mechatronic_test_system --test DEVICE_001 --verbose

# Check system resources
top
df -h
```

#### Performance Issues

**Problem:** Tests running slowly
```
Test duration: 120 seconds (expected: 45 seconds)
```

**Solution:**
```bash
# Check CPU usage
top

# Check memory usage
free -h

# Check disk space
df -h

# Optimize test sequence
# - Reduce measurement averaging
# - Parallel testing where possible
# - Check for memory leaks
```

### System Diagnostics

#### Log Analysis

**Error Log Locations:**
- Linux: `/var/log/mechatronic_test.log`
- Windows: `%TEMP%\mechatronic_test.log`
- macOS: `/tmp/mechatronic_test.log`

**Common Log Patterns:**

```bash
# View recent errors
tail -f /var/log/mechatronic_test.log | grep ERROR

# Search for specific device issues
grep "DEVICE_001" /var/log/mechatronic_test.log

# Monitor in real-time
tail -f /var/log/mechatronic_test.log
```

#### System Health Monitoring

**Check System Status:**
```bash
# Overall system status
mechatronic_test_system --status

# Detailed health metrics
mechatronic_test_system --status --verbose

# Generate system report
mechatronic_test_system --report > system_health_$(date +%Y%m%d).txt
```

**Monitor Resource Usage:**
```bash
# CPU and memory
htop

# Disk I/O
iotop

# Network activity
netstat -an | grep :502
```

### Environment-Specific Issues

#### Cleanroom Environment

**Problem:** Electrostatic discharge affecting measurements
```
Measurement noise increased significantly
Standard deviation > 3σ threshold
```

**Solution:**
- Verify proper grounding of all equipment
- Check anti-static procedures are being followed
- Ensure relative humidity is within 40-60% range
- Use ionizers to neutralize static charges

**Problem:** Temperature variations affecting calibration
```
Temperature drift detected: >0.5°C/hour
Calibration validity compromised
```

**Solution:**
- Allow 30-minute warm-up period after equipment power-on
- Verify HVAC system is maintaining stable conditions
- Consider thermal insulation for sensitive equipment
- Schedule calibration after thermal equilibrium

#### Production Environment

**Problem:** High-volume testing causing system overload
```
Queue overflow: 150 devices pending
Average processing time: 3.2 minutes/device
```

**Solution:**
```bash
# Implement batch processing
mechatronic_test_system --batch-size 10

# Parallel testing (if hardware supports)
mechatronic_test_system --parallel-tests 4

# Optimize test sequences
# - Remove redundant measurements
# - Implement smart test ordering
# - Use statistical sampling for non-critical parameters
```

### Advanced Troubleshooting

#### Debug Mode

Enable debug mode for detailed logging:
```bash
export MECHATRONIC_DEBUG=1
export MECHATRONIC_LOG_LEVEL=DEBUG
mechatronic_test_system --status
```

#### Memory Leak Detection

For long-running operations:
```bash
# Linux - use valgrind
valgrind --leak-check=full ./mechatronic_test_system --test DEVICE_001

# Monitor memory usage over time
while true; do
    ps aux | grep mechatronic_test_system
    sleep 60
done
```

#### Performance Profiling

Profile application performance:
```bash
# Linux - use perf
perf record ./mechatronic_test_system --test DEVICE_001
perf report

# Profile specific functions
gprof mechatronic_test_system gmon.out > profile_analysis.txt
```

### Recovery Procedures

#### Emergency Stop Recovery

If emergency stop is activated:
1. Identify and resolve the safety issue
2. Reset emergency stop button
3. Power cycle the system
4. Run full diagnostic sequence
5. Verify all safety systems before resuming operation

#### System Recovery After Crash

```bash
# Check for core dumps
ls -la core*

# Restart services
sudo systemctl restart mechatronic-service

# Verify system integrity
mechatronic_test_system --self-test

# Run abbreviated test sequence
mechatronic_test_system --test VERIFICATION_DEVICE
```

#### Data Recovery

For corrupted test data:
```bash
# Check filesystem integrity
fsck /dev/sdb1

# Recover from backup
cp /backup/test_data/* /var/lib/mechatronic/data/

# Validate recovered data
mechatronic_test_system --validate-data
```

### Getting Additional Help

#### Support Escalation Process

1. **Level 1**: Check this troubleshooting guide
2. **Level 2**: Review system logs and error messages
3. **Level 3**: Contact development team with:
   - System configuration details
   - Error logs
   - Steps to reproduce the issue
   - Expected vs. actual behavior

#### Information to Collect

When reporting issues, include:
```bash
# System information
uname -a
mechatronic_test_system --version

# Hardware configuration
lshw -short
lsusb
lspci

# Software environment
gcc --version
cmake --version
python3 --version

# Log files (last 100 lines)
tail -100 /var/log/mechatronic_test.log
```

### Preventive Measures

#### Regular Maintenance

- **Daily**: Check system status and error logs
- **Weekly**: Run calibration verification
- **Monthly**: Review performance metrics and trends
- **Quarterly**: Update software and perform comprehensive system check

#### Monitoring Setup

```bash
# Set up automated monitoring
crontab -e

# Add monitoring jobs
0 6 * * * /usr/local/bin/mechatronic_test_system --calibrate
0 */4 * * * /usr/local/bin/check_system_health.sh
```

---

*Troubleshooting Guide Version: 1.0*  
*Last Updated: 2024*  
*For additional support, contact the development team*