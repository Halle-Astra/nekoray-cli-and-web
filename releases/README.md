# NekoRay CLI - Headless V2Ray Client

A headless command-line interface and web API server for V2Ray/sing-box proxy management on Linux.

## Binaries

- `nekoray-cli` - Command-line interface for direct proxy management
- `nekoray-web` - Web API server with embedded web interface

## Features

- ✅ Headless operation (no GUI required)
- ✅ Command-line interface for automation
- ✅ Web API with embedded HTML interface
- ✅ TUN mode support for transparent proxying
- ✅ Profile management
- ✅ Traffic monitoring
- ✅ Configuration management
- ✅ Safety checks for network operations

## Usage

### CLI Mode
```bash
# Start proxy with profile ID 1
./nekoray-cli start 1

# Enable TUN mode
./nekoray-cli start 1 --tun

# Show status
./nekoray-cli status

# Stop proxy
./nekoray-cli stop
```

### Web API Server
```bash
# Start web server on port 8080
./nekoray-web --port 8080

# Start with custom config directory
./nekoray-web --config-dir /path/to/config
```

Then open http://127.0.0.1:8080 in your browser for the web interface.

## API Endpoints

- `GET /api/status` - Get service status
- `POST /api/start` - Start proxy (JSON: {"profile_id": 1, "tun_mode": false})
- `POST /api/stop` - Stop proxy
- `POST /api/restart` - Restart proxy
- `GET /api/config` - Get current configuration
- `GET /api/traffic` - Get traffic statistics
- `GET /api/profiles` - List available profiles

## Requirements

- Linux x86_64
- Qt5 Core and Network libraries
- Root privileges for TUN mode operations

## Installation

1. Extract binaries to desired location
2. Install Qt5 dependencies:
   ```bash
   # Ubuntu/Debian
   sudo apt install libqt5core5a libqt5network5

   # CentOS/RHEL
   sudo yum install qt5-qtbase
   ```
3. Make binaries executable:
   ```bash
   chmod +x nekoray-cli nekoray-web
   ```

## Configuration

The application creates a configuration directory at `~/.config/nekoray/` by default.
You can specify a custom directory with `--config-dir` option.

## Safety Features

- Automatic VM/container detection
- SSH connection detection
- Production environment checks
- Network state backup capabilities
- Dry-run mode for testing

## Architecture

Built with modern C++17 and Qt5 framework:
- **NekoService**: Core proxy management
- **CoreManager**: Process lifecycle management
- **TunManager**: TUN mode operations
- **ConfigManager**: Configuration handling
- **SafetyUtils**: Security and safety checks

## Version

v1.0.0 - Initial headless release

For issues and updates, visit: https://github.com/your-repo/nekoray-cli