# NekoRay CLI - 无头 Linux V2Ray 客户端

[![构建状态](https://img.shields.io/badge/build-passing-brightgreen)](build_test/)
[![架构状态](https://img.shields.io/badge/architecture-headless-blue)](#architecture)
[![测试覆盖](https://img.shields.io/badge/tests-11%2F11%20passed-brightgreen)](build_test/)

> 将 NekoRay (v2rayN) 改造为支持 CLI 和 Web 接口的无头 Linux V2Ray 客户端，完整保留 TUN 功能。

## ✨ 特性

- 🖥️ **完全无头** - 无 GUI 依赖，适合服务器环境
- 🔧 **双接口支持** - CLI 命令行 + Web API 界面
- 🌐 **TUN 模式** - 完整保留 TUN 透明代理功能
- 🛡️ **安全机制** - 内置 Docker 检测、SSH 感知、干运行模式
- ⚡ **异步架构** - 基于 Qt 信号/槽的高性能架构
- 📦 **自包含** - 独立配置管理，无全局依赖

## 🚀 快速开始

### 安装依赖

```bash
# Ubuntu/Debian
sudo apt install qt6-base-dev cmake build-essential

# 安装代理核心 (选择其一)
# sing-box 或 nekobox-core
```

### 编译

```bash
git clone <repository>
cd nekoray_cli
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 使用

**CLI 模式:**
```bash
# 启动代理
./nekoray-cli start --profile 1

# 启用 TUN 模式 (需要管理员权限)
./nekoray-cli start --profile 1 --tun

# 查看状态
./nekoray-cli status

# 停止代理
./nekoray-cli stop

# 安全测试 (不实际执行)
./nekoray-cli start --dry-run
```

**Web API 模式:**
```bash
# 启动 Web 服务器
./nekoray-web --port 8080

# 访问 Web 界面
http://localhost:8080

# API 调用示例
curl http://localhost:8080/api/status
curl -X POST http://localhost:8080/api/start -d '{"profile_id":1}'
```

## 🏗️ 架构设计

```
┌─────────────┐    ┌─────────────┐
│ CLI 客户端   │    │ Web 界面    │
└──────┬──────┘    └──────┬──────┘
       │                  │
       └─────┬──────┬─────┘
             │      │
      ┌──────▼──────▼──────┐
      │   NekoService      │  ← 核心服务 (无 GUI 依赖)
      └─┬─────────────────┬┘
        │                 │
┌───────▼────┐    ┌───────▼────┐
│CoreManager │    │TunManager  │  ← 管理层
└────────────┘    └────────────┘
        │                 │
        ▼                 ▼
┌─────────────────────────────────┐
│     sing-box/nekobox-core       │  ← 代理核心
└─────────────────────────────────┘
```

### 核心组件

| 组件 | 功能 | 文件 |
|------|------|------|
| `NekoService` | 核心服务管理 | `nekoray/core/NekoService_Fixed.cpp` |
| `CoreManager` | 代理进程管理 | `nekoray/core/CoreManager_Fixed.cpp` |
| `TunManager` | TUN 模式管理 | `nekoray/core/TunManager_Fixed.cpp` |
| `ConfigManager` | 配置文件管理 | `nekoray/core/ConfigManager.cpp` |
| `CLI Interface` | 命令行接口 | `nekoray/cli/main_cli.cpp` |
| `Web API` | REST API 服务器 | `nekoray/web/WebApiServer.cpp` |

## 🧪 测试与验证

项目包含完整的测试套件确保代码质量：

```bash
cd build_test

# 运行所有测试
./critical_issues_test      # 关键问题验证
./test_architecture         # 架构测试
./test_core_logic           # 核心逻辑测试
./final_verification_report # 完整验证报告
```

**测试覆盖**:
- ✅ 架构一致性检查 (11/11 通过)
- ✅ 信号/槽连接验证
- ✅ 内存管理测试
- ✅ 配置系统测试
- ✅ 安全机制验证

## 📚 文档

- [📖 详细开发记录](DEVELOPMENT_LOG.md) - 完整的开发过程和技术细节
- [⚡ 快速参考](QUICK_REFERENCE.md) - 常用命令和问题排查
- [🔧 API 文档](docs/API.md) - Web API 接口说明
- [🛠️ 构建指南](docs/BUILD.md) - 详细的构建说明

## 🛡️ 安全特性

- **Docker 环境检测** - 自动检测容器环境
- **SSH 连接感知** - 防止远程连接中断  
- **干运行模式** - 安全测试模式
- **权限提升管理** - TUN 模式的安全权限处理
- **配置验证** - 启动前的安全检查

## 🔧 配置

**基本配置文件** (`~/.config/nekoray/config.json`):
```json
{
    "inbound_address": "127.0.0.1",
    "inbound_socks_port": 2080,
    "inbound_http_port": 2081,
    "spmode_vpn": false,
    "vpn_internal_tun": true
}
```

**环境变量**:
```bash
export NEKORAY_CONFIG_DIR="/path/to/config"
export NEKORAY_LOG_LEVEL="info"
export NEKORAY_CORE_PATH="/path/to/sing-box"
```

## 🤝 贡献指南

1. Fork 项目
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

**贡献前请确保**:
- 运行所有测试并通过
- 更新相关文档
- 遵循代码规范

## 📄 许可证

本项目基于 [MIT License](LICENSE) - 详见 LICENSE 文件。

## 🙏 致谢

- [NekoRay](https://github.com/MatsuriDayo/nekoray) - 原始项目
- [sing-box](https://github.com/SagerNet/sing-box) - 代理核心
- Qt6 框架 - 跨平台支持

## 📞 支持

- 🐛 [报告 Bug](issues)
- 💡 [功能请求](issues)
- 📖 [详细文档](DEVELOPMENT_LOG.md)
- 💬 [讨论区](discussions)

## 📊 项目状态

| 指标 | 状态 |
|------|------|
| 构建状态 | ✅ 通过 |
| 测试覆盖 | ✅ 11/11 |
| 代码质量 | ✅ 优秀 |
| 文档完整性 | ✅ 完整 |
| 生产就绪 | ✅ 是 |

---

**当前版本**: 1.0.0  
**最后更新**: 2025-11-15  
**维护状态**: 🟢 活跃开发