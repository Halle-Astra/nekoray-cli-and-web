# NekoRay CLI/Web v1.0.3 - Complete Distribution

## 🚀 What's Included

This package contains the complete NekoRay CLI/Web headless solution with real proxy core:

- **nekoray-cli-minimal** - Command-line interface
- **nekoray-web-minimal** - Web API server  
- **nekobox_core** - Actual proxy core executable

## 🛠️ Installation

```bash
tar xzf nekoray-COMPLETE-FINAL-v1.0.3-linux-x86_64.tar.gz
cd nekoray-COMPLETE-FINAL-v1.0.3-linux-x86_64
chmod +x *
```

## 🎯 Usage Examples

### CLI Interface
```bash
# Show status
./nekoray-cli-minimal status

# Start proxy with profile
./nekoray-cli-minimal start 1

# Start TUN mode
./nekoray-cli-minimal tun-start

# Show help
./nekoray-cli-minimal --help
```

### Web Interface
```bash
# Start web server
./nekoray-web-minimal --port 8080

# Access web interface
curl http://localhost:8080/api/status
```

### Core Executable
```bash
# Check version
./nekobox_core --version

# Run in NekoBox mode
./nekobox_core nekobox

# Run with config
./nekobox_core run config.json
```

## ✨ Key Features

- ✅ **Headless Operation** - No GUI dependencies
- ✅ **TUN Mode Support** - Full transparent proxy
- ✅ **RESTful API** - Web-based control
- ✅ **Cross-Platform** - Linux ready
- ✅ **Real Proxy Core** - Actual working core executable
- ✅ **Professional CLI** - Full command-line interface

## 🔧 Technical Details

- **Core**: Based on sing-box proxy framework
- **Frontend**: Qt5-based CLI/Web interfaces  
- **Architecture**: Headless service + API layer
- **Networking**: SOCKS5/HTTP proxy + TUN mode
- **Configuration**: JSON-based profiles

## 🚨 Fixed Issues from Previous Versions

- ✅ Fixed command-line argument conflicts
- ✅ Fixed missing core executable error
- ✅ Added graceful error handling
- ✅ Resolved Git repository source code issues
- ✅ Created real working proxy core

Ready for production use! 🎉
