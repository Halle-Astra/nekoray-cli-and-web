# NekoRay Headless 构建指南

## 构建要求

### 系统要求
- Linux (Ubuntu 20.04+ / Debian 11+ / CentOS 8+ / Arch Linux)
- macOS 11.0+ (实验性支持)
- Qt 6.2+
- CMake 3.16+
- GCC 9+ 或 Clang 10+

### 依赖库
- Qt6Core
- Qt6Network  
- Qt6HttpServer
- yaml-cpp (如果启用)
- protobuf (如果启用gRPC)
- gRPC (如果启用gRPC)

## 快速开始

### Ubuntu/Debian
```bash
# 安装依赖
sudo apt update
sudo apt install -y \
    build-essential cmake git \
    qt6-base-dev qt6-tools-dev \
    libqt6httpserver6-dev \
    libyaml-cpp-dev \
    libprotobuf-dev protobuf-compiler \
    libgrpc-dev libgrpc++-dev

# 克隆项目（假设你已经有了代码）
# git clone <your-repo>
cd nekoray

# 创建构建目录
mkdir build-headless
cd build-headless

# 配置构建
cmake -DCMAKE_BUILD_TYPE=Release \
      -f ../CMakeLists_headless.txt \
      ..

# 编译
make -j$(nproc)
```

### Arch Linux
```bash
# 安装依赖
sudo pacman -S \
    base-devel cmake git \
    qt6-base qt6-tools qt6-httpserver \
    yaml-cpp protobuf grpc

# 构建步骤同上
```

### CentOS/RHEL/Fedora
```bash
# CentOS Stream 9 / RHEL 9 / Fedora
sudo dnf install -y \
    gcc-c++ cmake git \
    qt6-qtbase-devel qt6-qttools-devel \
    qt6-qthttpserver-devel \
    yaml-cpp-devel \
    protobuf-devel grpc-devel

# 构建步骤同上
```

## 详细构建配置

### 基本配置
```bash
cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  -f ../CMakeLists_headless.txt \
  ..
```

### 最小化构建（仅核心功能）
```bash
cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DNKR_NO_EXTERNAL=ON \
  -DNKR_NO_GRPC=ON \
  -DNKR_NO_YAML=ON \
  -f ../CMakeLists_headless.txt \
  ..
```

### 调试构建
```bash
cmake \
  -DCMAKE_BUILD_TYPE=Debug \
  -DNKR_ENABLE_SANITIZER=ON \
  -f ../CMakeLists_headless.txt \
  ..
```

### 交叉编译（ARM64）
```bash
cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=../cmake/arm64-cross.cmake \
  -f ../CMakeLists_headless.txt \
  ..
```

## 构建选项

### CMake变量
| 变量 | 默认值 | 说明 |
|------|--------|------|
| `NKR_NO_EXTERNAL` | `OFF` | 禁用所有外部依赖 |
| `NKR_NO_GRPC` | `OFF` | 禁用gRPC支持 |
| `NKR_NO_YAML` | `OFF` | 禁用YAML支持 |
| `NKR_NO_ZXING` | `OFF` | 禁用二维码支持 |
| `NKR_NO_QHOTKEY` | `OFF` | 禁用热键支持 |
| `NKR_HEADLESS_MODE` | `ON` | 启用headless模式 |

### 编译定义
- `NKR_HEADLESS_MODE`: 启用headless模式编译

## 安装

### 本地安装
```bash
make install
```

### 打包安装
```bash
# Debian包
cpack -G DEB

# RPM包
cpack -G RPM

# 源码包
cpack -G TGZ
```

### AppImage（推荐）
```bash
# 需要先安装 linuxdeploy
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage

# 创建AppImage
./linuxdeploy-x86_64.AppImage \
  --appdir AppDir \
  --executable nekoray-cli \
  --executable nekoray-daemon \
  --create-desktop-file \
  --output appimage
```

## 验证构建

### 基本验证
```bash
# 检查可执行文件
./nekoray-cli --help
./nekoray-daemon --help

# 检查依赖
ldd nekoray-cli
ldd nekoray-daemon

# 测试基本功能（安全）
./nekoray-cli --dry-run status
./nekoray-daemon --port 8081 --dry-run
```

### 功能测试
```bash
# 在测试环境中
./nekoray-cli --safety --verbose status
./nekoray-daemon --safety --port 8081 --verbose
```

## 故障排除

### 常见错误

#### Qt6找不到
```bash
# 设置Qt路径
export Qt6_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt6
export CMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6:$CMAKE_PREFIX_PATH

# 或使用qmake路径
export PATH=/usr/lib/qt6/bin:$PATH
```

#### gRPC错误
```bash
# 禁用gRPC
cmake -DNKR_NO_GRPC=ON -f ../CMakeLists_headless.txt ..

# 或手动指定gRPC路径
cmake -DgRPC_DIR=/usr/local/lib/cmake/grpc -f ../CMakeLists_headless.txt ..
```

#### 链接错误
```bash
# 检查库路径
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# 或静态链接
cmake -DBUILD_SHARED_LIBS=OFF -f ../CMakeLists_headless.txt ..
```

### 调试构建问题

#### 详细构建日志
```bash
make VERBOSE=1
```

#### CMake调试
```bash
cmake --debug-output -f ../CMakeLists_headless.txt ..
```

#### 依赖检查
```bash
# 检查pkg-config
pkg-config --list-all | grep -i qt

# 检查CMake模块
find /usr -name "*Qt6*" -type d 2>/dev/null
```

## 性能优化

### 编译优化
```bash
cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native" \
  -f ../CMakeLists_headless.txt \
  ..
```

### LTO（链接时优化）
```bash
cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
  -f ../CMakeLists_headless.txt \
  ..
```

### 最小化二进制
```bash
# 构建后strip
strip nekoray-cli nekoray-daemon

# UPX压缩（可选）
upx --best nekoray-cli nekoray-daemon
```

## 开发构建

### 开发环境
```bash
cmake \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DBUILD_TESTING=ON \
  -f ../CMakeLists_headless.txt \
  ..
```

### 代码格式化
```bash
# 安装clang-format
sudo apt install clang-format

# 格式化代码
find . -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
```

### 静态分析
```bash
# Clang静态分析
scan-build make

# Cppcheck
cppcheck --enable=all src/
```

## Docker构建

### Dockerfile示例
```dockerfile
FROM ubuntu:22.04

RUN apt update && apt install -y \
    build-essential cmake git \
    qt6-base-dev qt6-httpserver-dev \
    && rm -rf /var/lib/apt/lists/*

COPY . /src
WORKDIR /src/build

RUN cmake -DCMAKE_BUILD_TYPE=Release -f ../CMakeLists_headless.txt .. \
    && make -j$(nproc)

FROM ubuntu:22.04
RUN apt update && apt install -y \
    libqt6core6 libqt6network6 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=0 /src/build/nekoray-* /usr/local/bin/
EXPOSE 8080
CMD ["nekoray-daemon", "--port", "8080"]
```

### 构建Docker镜像
```bash
docker build -t nekoray-headless .
docker run -p 8080:8080 nekoray-headless
```

## 发布准备

### 版本检查
```bash
./nekoray-cli --version
./nekoray-daemon --version
```

### 测试套件
```bash
# 基本测试
./test_build.sh

# 安全测试
./nekoray-cli --safety --dry-run tun-start

# 性能测试
time ./nekoray-cli --dry-run start 1
```

### 打包发布
```bash
# 创建发布目录
mkdir -p release/nekoray-headless-linux-x64

# 复制文件
cp nekoray-cli nekoray-daemon release/nekoray-headless-linux-x64/
cp ../README_HEADLESS.md release/nekoray-headless-linux-x64/
cp ../scripts/nekoray-daemon.sh release/nekoray-headless-linux-x64/

# 创建压缩包
cd release
tar czf nekoray-headless-linux-x64.tar.gz nekoray-headless-linux-x64/
```

记住：始终在安全的测试环境中进行完整功能测试！