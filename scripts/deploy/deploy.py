#!/usr/bin/env python3
"""
Deployment script for the Automated Mechatronic Test Inspection System
Handles system deployment, configuration, and validation
"""

import os
import sys
import shutil
import subprocess
import argparse
import yaml
import json
from pathlib import Path
from typing import Dict, List, Optional

class DeploymentManager:
    """Manages system deployment and configuration"""
    
    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        self.install_dir = self.project_root / "install"
        self.config_dir = self.project_root / "config"
        
    def validate_prerequisites(self) -> bool:
        """Validate system prerequisites"""
        print("Validating deployment prerequisites...")
        
        # Check for required directories
        required_dirs = [
            self.project_root / "build",
            self.install_dir,
            self.config_dir
        ]
        
        for dir_path in required_dirs:
            if not dir_path.exists():
                print(f"Error: Required directory not found: {dir_path}")
                return False
        
        # Check for executables
        executables = [
            self.install_dir / "bin" / "mechatronic_test_system",
            self.install_dir / "scripts" / "equipment_controller.py"
        ]
        
        for exe in executables:
            if not exe.exists():
                print(f"Error: Executable not found: {exe}")
                return False
        
        print("✓ Prerequisites validated")
        return True
    
    def create_system_config(self, target_dir: str, port: str = "/dev/ttyUSB0") -> bool:
        """Create system configuration for deployment"""
        target_path = Path(target_dir)
        
        try:
            # Create target directory
            target_path.mkdir(parents=True, exist_ok=True)
            
            # Load template configuration
            template_file = self.config_dir / "config.template.yaml"
            with open(template_file, 'r') as f:
                config = yaml.safe_load(f)
            
            # Update configuration for deployment
            config['device_port'] = port
            config['log_file_path'] = str(target_path / "logs" / "mechatronic_test.log")
            config['results_directory'] = str(target_path / "results")
            
            # Create subdirectories
            (target_path / "logs").mkdir(exist_ok=True)
            (target_path / "results").mkdir(exist_ok=True)
            (target_path / "backup").mkdir(exist_ok=True)
            
            # Write configuration file
            config_file = target_path / "config.yaml"
            with open(config_file, 'w') as f:
                yaml.dump(config, f, default_flow_style=False)
            
            print(f"✓ Configuration created: {config_file}")
            return True
            
        except Exception as e:
            print(f"Error creating configuration: {e}")
            return False
    
    def deploy_binaries(self, target_dir: str) -> bool:
        """Deploy binaries to target directory"""
        target_path = Path(target_dir)
        
        try:
            # Create target structure
            bin_dir = target_path / "bin"
            lib_dir = target_path / "lib"
            scripts_dir = target_path / "scripts"
            
            for dir_path in [bin_dir, lib_dir, scripts_dir]:
                dir_path.mkdir(parents=True, exist_ok=True)
            
            # Copy binaries
            shutil.copy2(self.install_dir / "bin" / "mechatronic_test_system", bin_dir)
            shutil.copy2(self.install_dir / "lib" / "libmechatronic_test_lib.a", lib_dir)
            
            # Copy Python scripts
            for script in (self.install_dir / "scripts").glob("*.py"):
                shutil.copy2(script, scripts_dir)
            
            # Copy wrapper scripts
            shutil.copy2(self.install_dir / "bin" / "mechatronic-test", bin_dir)
            shutil.copy2(self.install_dir / "bin" / "mechatronic-python", bin_dir)
            
            # Make scripts executable
            for script in bin_dir.glob("*"):
                script.chmod(0o755)
            
            print(f"✓ Binaries deployed to: {target_path}")
            return True
            
        except Exception as e:
            print(f"Error deploying binaries: {e}")
            return False
    
    def deploy_documentation(self, target_dir: str) -> bool:
        """Deploy documentation to target directory"""
        target_path = Path(target_dir)
        
        try:
            docs_dir = target_path / "docs"
            docs_dir.mkdir(parents=True, exist_ok=True)
            
            # Copy documentation
            shutil.copytree(self.install_dir / "docs", docs_dir, dirs_exist_ok=True)
            
            print(f"✓ Documentation deployed to: {docs_dir}")
            return True
            
        except Exception as e:
            print(f"Error deploying documentation: {e}")
            return False
    
    def create_service_scripts(self, target_dir: str) -> bool:
        """Create system service scripts"""
        target_path = Path(target_dir)
        
        try:
            # Create systemd service file (Linux)
            service_content = f"""[Unit]
Description=Automated Mechatronic Test Inspection System
After=network.target

[Service]
Type=simple
User=mechatronic
WorkingDirectory={target_path}
ExecStart={target_path}/bin/mechatronic_test_system --port /dev/ttyUSB0
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
"""
            
            service_file = target_path / "mechatronic-test.service"
            with open(service_file, 'w') as f:
                f.write(service_content)
            
            # Create startup script
            startup_script = f"""#!/bin/bash
# Startup script for Mechatronic Test System

cd {target_path}

# Check dependencies
if [ ! -f config.yaml ]; then
    echo "Error: Configuration file not found"
    exit 1
fi

# Start the system
exec ./bin/mechatronic-test --config config.yaml
"""
            
            start_script = target_path / "start_system.sh"
            with open(start_script, 'w') as f:
                f.write(startup_script)
            start_script.chmod(0o755)
            
            print(f"✓ Service scripts created in: {target_path}")
            return True
            
        except Exception as e:
            print(f"Error creating service scripts: {e}")
            return False
    
    def validate_deployment(self, target_dir: str) -> bool:
        """Validate deployment"""
        target_path = Path(target_dir)
        
        print("Validating deployment...")
        
        # Check required files
        required_files = [
            "bin/mechatronic_test_system",
            "bin/mechatronic-test",
            "scripts/equipment_controller.py",
            "config.yaml",
            "start_system.sh"
        ]
        
        for file_path in required_files:
            full_path = target_path / file_path
            if not full_path.exists():
                print(f"Error: Required file missing: {file_path}")
                return False
        
        # Test executables
        try:
            result = subprocess.run(
                [target_path / "bin" / "mechatronic_test_system", "--help"],
                capture_output=True, text=True, timeout=10
            )
            if result.returncode != 0:
                print("Error: C++ executable test failed")
                return False
        except Exception as e:
            print(f"Error testing C++ executable: {e}")
            return False
        
        try:
            result = subprocess.run(
                ["python3", target_path / "scripts" / "equipment_controller.py", "--help"],
                capture_output=True, text=True, timeout=10
            )
            if result.returncode != 0:
                print("Error: Python script test failed")
                return False
        except Exception as e:
            print(f"Error testing Python script: {e}")
            return False
        
        print("✓ Deployment validation successful")
        return True
    
    def full_deployment(self, target_dir: str, port: str = "/dev/ttyUSB0") -> bool:
        """Perform full system deployment"""
        print(f"Deploying Mechatronic Test System to: {target_dir}")
        
        if not self.validate_prerequisites():
            return False
        
        steps = [
            (self.create_system_config, "Creating configuration"),
            (self.deploy_binaries, "Deploying binaries"),
            (self.deploy_documentation, "Deploying documentation"),
            (self.create_service_scripts, "Creating service scripts"),
            (self.validate_deployment, "Validating deployment")
        ]
        
        for step_func, step_name in steps:
            print(f"\n{step_name}...")
            if step_name == "Creating configuration":
                if not step_func(target_dir, port):
                    return False
            else:
                if not step_func(target_dir):
                    return False
        
        print(f"\n✓ Deployment completed successfully!")
        print(f"\nTo start the system:")
        print(f"  cd {target_dir}")
        print(f"  ./start_system.sh")
        print(f"\nOr manually:")
        print(f"  {target_dir}/bin/mechatronic-test --help")
        
        return True

def main():
    parser = argparse.ArgumentParser(description="Deploy Mechatronic Test System")
    parser.add_argument("target_dir", help="Target deployment directory")
    parser.add_argument("--port", default="/dev/ttyUSB0", help="Serial port for device communication")
    parser.add_argument("--validate-only", action="store_true", help="Only validate existing deployment")
    
    args = parser.parse_args()
    
    # Find project root
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent
    
    if not (project_root / "CMakeLists.txt").exists():
        print("Error: Cannot find project root directory")
        sys.exit(1)
    
    deployer = DeploymentManager(str(project_root))
    
    if args.validate_only:
        if deployer.validate_deployment(args.target_dir):
            print("Deployment validation successful")
            sys.exit(0)
        else:
            print("Deployment validation failed")
            sys.exit(1)
    else:
        if deployer.full_deployment(args.target_dir, args.port):
            sys.exit(0)
        else:
            print("Deployment failed")
            sys.exit(1)

if __name__ == "__main__":
    main()