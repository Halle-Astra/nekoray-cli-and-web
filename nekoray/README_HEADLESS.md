# NekoRay Headless - CLI and Web Interface

这是NekoRay的headless版本，支持CLI命令行界面和Web管理界面，完全无需图形界面即可使用所有功能，包括TUN模式。

## 功能特性

- ✅ 完全headless运行，无GUI依赖
- ✅ CLI命令行界面，支持所有基本操作
- ✅ Web API和管理界面
- ✅ 完整TUN模式支持（Linux root权限）
- ✅ 后台daemon模式
- ✅ 支持所有原版代理协议
- ✅ 配置文件兼容原版NekoRay
- ✅ systemd服务集成

## 构建

### 前置要求

- Qt6 (Core, Network, HttpServer)
- CMake 3.16+
- C++17编译器
- 与原版NekoRay相同的依赖库（yaml-cpp, protobuf, gRPC等）

### 编译步骤

```bash
# 在nekoray项目根目录
mkdir build-headless
cd build-headless
cmake -DCMAKE_BUILD_TYPE=Release -f ../CMakeLists_headless.txt ..
make -j$(nproc)

# 可选：安装到系统
sudo make install
```

## 使用方法

### CLI命令行模式

#### 基本命令

```bash
# 显示帮助
nekoray-cli --help

# 启动代理（需要先有配置好的profile）
nekoray-cli start <profile_id>

# 停止代理
nekoray-cli stop

# 查看状态
nekoray-cli status

# 重启代理
nekoray-cli restart <profile_id>

# 列出可用profiles
nekoray-cli list

# 启动/停止TUN模式
nekoray-cli tun-start
nekoray-cli tun-stop

# 导入配置
nekoray-cli import <url_or_file>

# 查看当前配置
nekoray-cli config
```

#### 使用示例

```bash
# 使用自定义配置目录启动
nekoray-cli --config ~/.config/nekoray start 1

# 启动并显示详细日志
nekoray-cli --verbose start 1

# 启动TUN模式（需要root权限）
sudo nekoray-cli tun-start
```

### Web界面 + Daemon模式

#### 启动daemon

```bash
# 默认启动（端口8080）
nekoray-daemon

# 自定义端口和配置
nekoray-daemon --port 8080 --config ~/.config/nekoray --verbose

# 自动启动指定profile并启用TUN
nekoray-daemon --auto-start 1 --tun --verbose

# 后台运行
nekoray-daemon --daemon --port 8080
```

#### 使用启动脚本

```bash
# 使用便捷脚本启动
./nekoray-daemon.sh

# 自定义参数
./nekoray-daemon.sh --port 8080 --auto-start 1 --tun --daemon

# 查看脚本帮助
./nekoray-daemon.sh --help
```

#### Web界面访问

打开浏览器访问：`http://localhost:8080`

Web界面功能：
- 实时查看服务状态
- 启动/停止/重启代理
- 选择不同profile
- 控制TUN模式
- 查看流量统计
- 实时日志显示

### systemd服务

#### 安装服务

```bash
# 编译时会自动安装服务文件，或手动复制
sudo cp nekoray-daemon.service /etc/systemd/system/

# 重载systemd配置
sudo systemctl daemon-reload

# 启用开机自启
sudo systemctl enable nekoray-daemon

# 启动服务
sudo systemctl start nekoray-daemon

# 查看状态
sudo systemctl status nekoray-daemon

# 查看日志
sudo journalctl -u nekoray-daemon -f
```

## TUN模式使用

### Linux权限配置

TUN模式需要特殊权限，有以下几种方式：

#### 方式1：root运行

```bash
sudo nekoray-daemon --tun
```

#### 方式2：capabilities设置

```bash
# 给可执行文件设置capabilities
sudo setcap cap_net_admin+ep /usr/bin/nekoray-daemon

# 然后普通用户也可以使用TUN
nekoray-daemon --tun
```

#### 方式3：用户组配置

```bash
# 创建nekoray用户组
sudo groupadd nekoray

# 将用户添加到组
sudo usermod -a -G nekoray $USER

# 配置udev规则（可选）
echo 'SUBSYSTEM=="net", GROUP="nekoray", MODE="0660"' | sudo tee /etc/udev/rules.d/99-nekoray.rules
```

### TUN模式配置

TUN模式的配置文件兼容原版NekoRay的VPN设置：

- TUN接口名称
- MTU设置
- 路由规则
- DNS配置
- 进程/IP白名单/黑名单

## API文档

### REST API端点

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/status` | 获取服务状态 |
| POST | `/api/start` | 启动代理 |
| POST | `/api/stop` | 停止代理 |
| POST | `/api/restart` | 重启代理 |
| GET | `/api/profiles` | 列出profiles |
| GET | `/api/config` | 获取配置 |
| POST | `/api/config` | 更新配置 |
| GET | `/api/traffic` | 获取流量统计 |
| POST | `/api/tun/start` | 启动TUN模式 |
| POST | `/api/tun/stop` | 停止TUN模式 |
| GET | `/api/logs` | 获取日志 |
| POST | `/api/import` | 导入配置 |

### 示例API调用

```bash
# 获取状态
curl http://localhost:8080/api/status

# 启动代理
curl -X POST -H "Content-Type: application/json" \
     -d '{"profile_id": 1}' \
     http://localhost:8080/api/start

# 停止代理
curl -X POST http://localhost:8080/api/stop

# 启动TUN模式
curl -X POST http://localhost:8080/api/tun/start

# 获取流量统计
curl http://localhost:8080/api/traffic
```

## 配置文件

### 配置目录结构

```
~/.config/nekoray/
├── groups/
│   └── nekobox.json     # 主配置文件（兼容原版）
├── profiles/            # 配置文件目录
├── routes/             # 路由规则
└── temp/              # 临时文件
```

### 配置兼容性

- 完全兼容原版NekoRay的配置文件
- 可以直接使用原版的配置目录
- 支持所有原版的代理协议和设置

## 故障排除

### 常见问题

1. **端口占用**
   ```bash
   # 检查端口占用
   netstat -tuln | grep :8080
   
   # 使用不同端口
   nekoray-daemon --port 8081
   ```

2. **TUN权限问题**
   ```bash
   # 检查当前用户权限
   id
   
   # 使用sudo运行
   sudo nekoray-daemon --tun
   ```

3. **配置文件权限**
   ```bash
   # 检查配置目录权限
   ls -la ~/.config/nekoray/
   
   # 修复权限
   chmod -R u+rw ~/.config/nekoray/
   ```

4. **依赖库问题**
   ```bash
   # 检查库依赖
   ldd nekoray-daemon
   
   # 确保Qt6和相关库已安装
   ```

### 日志调试

```bash
# 启用详细日志
nekoray-daemon --verbose

# 查看systemd日志
sudo journalctl -u nekoray-daemon -f

# 自定义日志级别
export QT_LOGGING_RULES="*.debug=true"
nekoray-daemon
```

## 开发和扩展

### 架构说明

- `NekoCore::NekoService`: 核心服务类，管理代理生命周期
- `NekoCore::CoreManager`: 核心进程管理
- `NekoCore::TunManager`: TUN模式管理
- `NekoWeb::WebApiServer`: Web API服务器
- 完全分离GUI依赖，可独立运行

### 自定义扩展

可以基于核心服务类开发自定义界面或集成到其他系统中。

## 许可证

与原版NekoRay使用相同的许可证。