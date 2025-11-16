// NekoRay CLI - 独立可执行版本 (不依赖Qt)
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>

class NekoRayCLI {
private:
    std::string configDir;
    std::string coreExecutable;
    pid_t corePid = -1;
    pid_t tunPid = -1;
    bool verbose = false;
    bool dryRun = false;
    
public:
    NekoRayCLI() {
        // 设置默认配置目录
        const char* home = getenv("HOME");
        if (home) {
            configDir = std::string(home) + "/.config/nekoray";
        } else {
            configDir = "/tmp/nekoray";
        }
        
        // 查找核心可执行文件
        std::vector<std::string> possibleNames = {"sing-box", "nekobox_core", "nekoray_core"};
        for (const auto& name : possibleNames) {
            if (fileExists("/usr/bin/" + name)) {
                coreExecutable = "/usr/bin/" + name;
                break;
            }
            if (fileExists("/usr/local/bin/" + name)) {
                coreExecutable = "/usr/local/bin/" + name;
                break;
            }
        }
    }
    
    bool fileExists(const std::string& path) {
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0);
    }
    
    void createConfigDir() {
        std::string cmd = "mkdir -p " + configDir + "/profiles " + configDir + "/groups";
        if (system(cmd.c_str()) != 0 && verbose) {
            std::cout << "Warning: Failed to create config directories" << std::endl;
        }
    }
    
    std::string generateConfig(int profileId) {
        // 生成基本的 sing-box 配置
        std::string configPath = configDir + "/config_" + std::to_string(profileId) + ".json";
        std::ofstream file(configPath);
        if (!file.is_open()) {
            return "";
        }
        
        // 基本配置模板
        file << R"({
  "log": {
    "level": "info"
  },
  "inbounds": [
    {
      "type": "socks",
      "tag": "socks-in",
      "listen": "127.0.0.1",
      "listen_port": 2080
    },
    {
      "type": "http",
      "tag": "http-in", 
      "listen": "127.0.0.1",
      "listen_port": 2081
    }
  ],
  "outbounds": [
    {
      "type": "direct",
      "tag": "direct"
    }
  ],
  "route": {
    "final": "direct"
  }
})";
        file.close();
        return configPath;
    }
    
    std::string generateTunConfig(int profileId) {
        // 生成 TUN 模式配置
        std::string configPath = configDir + "/tun_config_" + std::to_string(profileId) + ".json";
        std::ofstream file(configPath);
        if (!file.is_open()) {
            return "";
        }
        
        file << R"({
  "log": {
    "level": "info"
  },
  "dns": {
    "fakeip": {
      "enabled": true,
      "inet4_range": "198.18.0.0/15",
      "inet6_range": "fc00::/18"
    },
    "servers": [
      {
        "tag": "dns-remote",
        "address": "8.8.8.8",
        "detour": "proxy"
      },
      {
        "tag": "dns-direct",
        "address": "223.5.5.5",
        "detour": "direct"
      }
    ]
  },
  "inbounds": [
    {
      "type": "tun",
      "tag": "tun-in",
      "interface_name": "nekoray-tun",
      "inet4_address": "172.19.0.1/28",
      "mtu": 9000,
      "auto_route": true,
      "strict_route": true,
      "stack": "system",
      "endpoint_independent_nat": true,
      "sniff": true
    }
  ],
  "outbounds": [
    {
      "type": "socks",
      "tag": "proxy",
      "server": "127.0.0.1",
      "server_port": 2080,
      "udp_fragment": true
    },
    {
      "type": "direct",
      "tag": "direct"
    }
  ],
  "route": {
    "final": "proxy",
    "auto_detect_interface": true,
    "rules": [
      {
        "ip_cidr": ["224.0.0.0/3", "ff00::/8"],
        "outbound": "block"
      },
      {
        "port": 53,
        "outbound": "dns-out"
      }
    ]
  }
})";
        file.close();
        return configPath;
    }
    
    bool startCore(int profileId, bool tunMode = false) {
        if (coreExecutable.empty()) {
            std::cerr << "Error: Core executable not found. Please install sing-box or nekobox_core." << std::endl;
            return false;
        }
        
        createConfigDir();
        
        std::string configPath;
        if (tunMode) {
            configPath = generateTunConfig(profileId);
            if (verbose) std::cout << "Using TUN mode configuration" << std::endl;
        } else {
            configPath = generateConfig(profileId);
            if (verbose) std::cout << "Using standard configuration" << std::endl;
        }
        
        if (configPath.empty()) {
            std::cerr << "Error: Failed to generate configuration file" << std::endl;
            return false;
        }
        
        if (dryRun) {
            std::cout << "DRY RUN: Would execute: " << coreExecutable 
                     << " run -c " << configPath << std::endl;
            return true;
        }
        
        // 启动核心进程
        pid_t pid = fork();
        if (pid == 0) {
            // 子进程
            if (tunMode) {
                // TUN 模式需要 root 权限
                execlp("pkexec", "pkexec", coreExecutable.c_str(), 
                       "run", "-c", configPath.c_str(), nullptr);
            } else {
                execlp(coreExecutable.c_str(), coreExecutable.c_str(), 
                       "run", "-c", configPath.c_str(), nullptr);
            }
            std::cerr << "Error: Failed to start core process" << std::endl;
            exit(1);
        } else if (pid > 0) {
            if (tunMode) {
                tunPid = pid;
            } else {
                corePid = pid;
            }
            
            if (verbose) {
                std::cout << "Started " << (tunMode ? "TUN " : "") << "core process with PID: " << pid << std::endl;
                std::cout << "Config: " << configPath << std::endl;
                std::cout << "SOCKS proxy: 127.0.0.1:2080" << std::endl;
                std::cout << "HTTP proxy: 127.0.0.1:2081" << std::endl;
            }
            
            return true;
        } else {
            std::cerr << "Error: Failed to fork process" << std::endl;
            return false;
        }
    }
    
    bool stopCore(bool tunMode = false) {
        pid_t& targetPid = tunMode ? tunPid : corePid;
        
        if (targetPid <= 0) {
            if (verbose) std::cout << (tunMode ? "TUN " : "") << "Core is not running" << std::endl;
            return true;
        }
        
        if (dryRun) {
            std::cout << "DRY RUN: Would stop " << (tunMode ? "TUN " : "") 
                     << "process with PID: " << targetPid << std::endl;
            return true;
        }
        
        // 优雅停止
        kill(targetPid, SIGTERM);
        
        // 等待进程退出
        int status;
        if (waitpid(targetPid, &status, WNOHANG) == 0) {
            // 如果进程没有立即退出，等待一会
            sleep(2);
            if (waitpid(targetPid, &status, WNOHANG) == 0) {
                // 强制终止
                kill(targetPid, SIGKILL);
                waitpid(targetPid, &status, 0);
            }
        }
        
        if (verbose) {
            std::cout << "Stopped " << (tunMode ? "TUN " : "") << "core process" << std::endl;
        }
        
        targetPid = -1;
        return true;
    }
    
    void showStatus() {
        std::cout << "NekoRay CLI Status:" << std::endl;
        std::cout << "===================" << std::endl;
        std::cout << "Config Directory: " << configDir << std::endl;
        std::cout << "Core Executable: " << (coreExecutable.empty() ? "Not found" : coreExecutable) << std::endl;
        std::cout << "Standard Proxy: " << (corePid > 0 ? "Running" : "Stopped") 
                 << (corePid > 0 ? " (PID: " + std::to_string(corePid) + ")" : "") << std::endl;
        std::cout << "TUN Mode: " << (tunPid > 0 ? "Running" : "Stopped") 
                 << (tunPid > 0 ? " (PID: " + std::to_string(tunPid) + ")" : "") << std::endl;
        
        if (corePid > 0 || tunPid > 0) {
            std::cout << "\nProxy Endpoints:" << std::endl;
            std::cout << "  SOCKS5: 127.0.0.1:2080" << std::endl;
            std::cout << "  HTTP:   127.0.0.1:2081" << std::endl;
        }
    }
    
    void showHelp() {
        std::cout << "NekoRay CLI - Headless V2Ray Client v1.0" << std::endl;
        std::cout << "==========================================" << std::endl;
        std::cout << std::endl;
        std::cout << "USAGE:" << std::endl;
        std::cout << "  nekoray-cli <command> [options]" << std::endl;
        std::cout << std::endl;
        std::cout << "COMMANDS:" << std::endl;
        std::cout << "  start [profile_id]  Start proxy service" << std::endl;
        std::cout << "  stop                Stop proxy service" << std::endl;
        std::cout << "  restart [profile_id] Restart proxy service" << std::endl;
        std::cout << "  status              Show service status" << std::endl;
        std::cout << "  help                Show this help" << std::endl;
        std::cout << std::endl;
        std::cout << "OPTIONS:" << std::endl;
        std::cout << "  --tun               Enable TUN mode (requires root)" << std::endl;
        std::cout << "  --verbose, -v       Enable verbose output" << std::endl;
        std::cout << "  --dry-run, -n       Dry run mode (safe testing)" << std::endl;
        std::cout << "  --config-dir DIR    Set configuration directory" << std::endl;
        std::cout << std::endl;
        std::cout << "EXAMPLES:" << std::endl;
        std::cout << "  nekoray-cli start 1              # Start with profile 1" << std::endl;
        std::cout << "  nekoray-cli start 1 --tun        # Start with TUN mode" << std::endl;
        std::cout << "  nekoray-cli start --dry-run      # Test without executing" << std::endl;
        std::cout << "  nekoray-cli status                # Check status" << std::endl;
        std::cout << "  nekoray-cli stop                  # Stop all services" << std::endl;
        std::cout << std::endl;
        std::cout << "SAFETY FEATURES:" << std::endl;
        std::cout << "  ✓ Docker environment detection" << std::endl;
        std::cout << "  ✓ SSH connection awareness" << std::endl;
        std::cout << "  ✓ Dry-run mode for safe testing" << std::endl;
        std::cout << "  ✓ Graceful shutdown handling" << std::endl;
        std::cout << std::endl;
        std::cout << "For more information, visit: https://github.com/your-repo/nekoray-cli" << std::endl;
    }
    
    int run(int argc, char* argv[]) {
        if (argc < 2) {
            showHelp();
            return 1;
        }
        
        std::string command = argv[1];
        bool tunMode = false;
        int profileId = 1;
        
        // 解析参数
        for (int i = 2; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--tun") {
                tunMode = true;
            } else if (arg == "--verbose" || arg == "-v") {
                verbose = true;
            } else if (arg == "--dry-run" || arg == "-n") {
                dryRun = true;
            } else if (arg == "--config-dir" && i + 1 < argc) {
                configDir = argv[++i];
            } else if (arg[0] != '-') {
                profileId = std::stoi(arg);
            }
        }
        
        // 安全检查
        if (getenv("SSH_CLIENT") || getenv("SSH_CONNECTION")) {
            std::cout << "⚠️  Warning: SSH connection detected. Use --dry-run for safe testing." << std::endl;
        }
        
        if (fileExists("/.dockerenv")) {
            std::cout << "🐳 Docker environment detected." << std::endl;
        }
        
        // 执行命令
        if (command == "start") {
            if (startCore(profileId, false)) {
                if (tunMode) {
                    return startCore(profileId, true) ? 0 : 1;
                }
                return 0;
            }
            return 1;
        } else if (command == "stop") {
            bool success = stopCore(false);
            if (tunMode) {
                success = stopCore(true) && success;
            }
            return success ? 0 : 1;
        } else if (command == "restart") {
            stopCore(false);
            if (tunMode) stopCore(true);
            sleep(1);
            if (startCore(profileId, false)) {
                if (tunMode) {
                    return startCore(profileId, true) ? 0 : 1;
                }
                return 0;
            }
            return 1;
        } else if (command == "status") {
            showStatus();
            return 0;
        } else if (command == "help" || command == "--help" || command == "-h") {
            showHelp();
            return 0;
        } else {
            std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
            std::cerr << "Use 'nekoray-cli help' for usage information." << std::endl;
            return 1;
        }
    }
};

int main(int argc, char* argv[]) {
    NekoRayCLI cli;
    return cli.run(argc, argv);
}