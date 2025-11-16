# NekoRay Headless 使用示例

## 安全提醒 ⚠️

**在测试网络代理软件时请注意：**
- 不要在生产环境或重要服务器上测试
- 建议在虚拟机或专用测试机器上运行
- TUN模式会修改系统路由，可能影响网络连接
- 测试前请确保有备用网络连接方式

## 基本使用流程

### 1. 准备工作

```bash
# 1. 确保有nekobox_core可执行文件
ls -la nekobox_core*

# 2. 创建配置目录
mkdir -p ~/.config/nekoray/{profiles,groups,routes}

# 3. 复制现有配置（如果有）
cp -r /path/to/existing/nekoray/config/* ~/.config/nekoray/
```

### 2. CLI模式使用

```bash
# 查看状态（安全，不会改变网络）
./nekoray-cli --config ~/.config/nekoray status

# 列出可用配置文件（安全）
./nekoray-cli --config ~/.config/nekoray list

# 启动代理（谨慎使用）
./nekoray-cli --config ~/.config/nekoray start 1

# 查看代理信息
./nekoray-cli status

# 停止代理
./nekoray-cli stop
```

### 3. Web界面模式

```bash
# 启动Web服务（默认端口8080）
./nekoray-daemon --config ~/.config/nekoray --port 8080 --verbose

# 在浏览器中访问（从其他机器或用端口转发）
# http://localhost:8080
```

### 4. 测试配置（不启动代理）

```bash
# 只验证配置文件，不启动网络服务
./nekoray-cli --config ~/.config/nekoray config

# 验证特定profile配置
./nekoray-cli --config ~/.config/nekoray --profile-id 1 --dry-run start
```

## 安全的测试步骤

### 第一步：配置验证
```bash
# 检查配置文件完整性
./nekoray-cli config

# 验证profiles
./nekoray-cli list
```

### 第二步：离线测试
```bash
# 启动daemon但不自动启动代理
./nekoray-daemon --port 8081 --no-auto-start

# 通过Web界面检查状态（不启动代理）
curl http://localhost:8081/api/status
```

### 第三步：受控测试
```bash
# 在测试环境启动代理
./nekoray-cli --test-mode start 1

# 立即检查连接状态
curl --proxy socks5://127.0.0.1:2080 https://httpbin.org/ip

# 如果出问题立即停止
./nekoray-cli stop
```

## TUN模式安全使用

### 权限准备
```bash
# 方法1：设置capabilities（推荐）
sudo setcap cap_net_admin+ep ./nekoray-daemon

# 方法2：创建专用用户（更安全）
sudo useradd -r -s /bin/false nekoray
sudo usermod -a -G nekoray $USER
```

### 安全测试TUN模式
```bash
# 1. 备份当前路由表
ip route save > /tmp/route_backup.txt

# 2. 在虚拟机或测试环境启动TUN
./nekoray-daemon --tun --config /tmp/test_config

# 3. 测试连接（谨慎）
ping 8.8.8.8

# 4. 如果有问题，恢复路由
sudo ip route restore < /tmp/route_backup.txt
```

## 配置文件示例

### 基本配置结构
```json
{
  "version": "4.0",
  "groups": [
    {
      "id": 1,
      "name": "Default",
      "profiles": [
        {
          "id": 1,
          "name": "Test Server",
          "type": "vmess",
          "server": "example.com",
          "port": 443,
          "security": "tls"
        }
      ]
    }
  ],
  "settings": {
    "inbound_socks_port": 2080,
    "inbound_http_port": 2081,
    "inbound_address": "127.0.0.1"
  }
}
```

### TUN配置示例
```json
{
  "vpn_internal_tun": true,
  "vpn_implementation": 0,
  "vpn_mtu": 9000,
  "vpn_ipv6": false,
  "vpn_strict_route": true,
  "vpn_rule_white": false,
  "vpn_rule_cidr": "10.0.0.0/8\n172.16.0.0/12\n192.168.0.0/16",
  "vpn_rule_process": "chrome\nfirefox"
}
```

## API使用示例

### 获取状态
```bash
curl http://localhost:8080/api/status | jq
```

### 启动代理
```bash
curl -X POST -H "Content-Type: application/json" \
     -d '{"profile_id": 1}' \
     http://localhost:8080/api/start
```

### 获取流量统计
```bash
curl http://localhost:8080/api/traffic | jq
```

### 控制TUN模式
```bash
# 启动TUN（需要权限）
curl -X POST http://localhost:8080/api/tun/start

# 停止TUN
curl -X POST http://localhost:8080/api/tun/stop
```

## 故障排除

### 连接问题
```bash
# 检查端口占用
netstat -tulpn | grep :8080

# 检查进程状态
ps aux | grep nekoray

# 检查日志
./nekoray-daemon --verbose 2>&1 | tee nekoray.log
```

### TUN问题
```bash
# 检查TUN接口
ip addr show

# 检查路由表
ip route show

# 检查DNS
systemd-resolve --status
```

### 权限问题
```bash
# 检查capabilities
getcap ./nekoray-daemon

# 检查用户组
groups $USER

# 测试权限
./nekoray-cli tun-start --dry-run
```

## 生产环境部署

### systemd服务
```bash
# 安装服务文件
sudo cp nekoray-daemon.service /etc/systemd/system/

# 重载配置
sudo systemctl daemon-reload

# 启用服务
sudo systemctl enable nekoray-daemon

# 启动服务
sudo systemctl start nekoray-daemon

# 查看状态
sudo systemctl status nekoray-daemon
```

### 反向代理配置（nginx）
```nginx
server {
    listen 80;
    server_name nekoray.example.com;
    
    location / {
        proxy_pass http://127.0.0.1:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

## 安全建议

1. **网络隔离**：在独立的网络环境中测试
2. **权限最小化**：只给必要的最小权限
3. **监控日志**：密切关注系统日志
4. **备份配置**：测试前备份网络配置
5. **回滚方案**：准备快速恢复方案

## 不要在生产环境做的事

❌ 直接在SSH连接的服务器上测试TUN模式
❌ 没有备份的情况下修改路由表
❌ 在关键业务服务器上运行测试
❌ 使用root用户运行服务（除非必要）
❌ 暴露Web界面到公网（没有认证的情况下）