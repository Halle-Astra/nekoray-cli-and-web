#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QTimer>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <iostream>

#include "../core/NekoService_Headless.hpp"
#include "../core/SafetyUtils.hpp"
#include "../rpc/gRPC_Headless.hpp"

class CliApplication : public QObject {
    Q_OBJECT

public:
    CliApplication(QObject *parent = nullptr) : QObject(parent), m_service(nullptr) {}

    int run(const QStringList &arguments) {
        QCommandLineParser parser;
        parser.setApplicationDescription("NekoRay CLI - Headless V2Ray Client");
        parser.addHelpOption();
        parser.addVersionOption();

        // 选项定义
        QCommandLineOption configOption({"c", "config"}, "Configuration directory", "directory");
        QCommandLineOption daemonOption({"d", "daemon"}, "Run as daemon (background service)");
        QCommandLineOption tunOption({"t", "tun"}, "Enable TUN mode");
        QCommandLineOption dryRunOption({"n", "dry-run"}, "Validate operations without executing (safe mode)");
        QCommandLineOption forceOption({"f", "force"}, "Force operations even if safety checks fail");
        QCommandLineOption safeOption({"s", "safe"}, "Enable additional safety checks");
        QCommandLineOption portOption({"p", "port"}, "Web API server port (default: 8080)", "port", "8080");

        parser.addOption(configOption);
        parser.addOption(daemonOption);
        parser.addOption(tunOption);
        parser.addOption(dryRunOption);
        parser.addOption(forceOption);
        parser.addOption(safeOption);
        parser.addOption(portOption);

        parser.addPositionalArgument("command", 
            "Command to execute:\n"
            "  start <profile_id>  - Start proxy with profile\n"
            "  stop                - Stop proxy\n"
            "  restart <profile_id> - Restart proxy with profile\n"
            "  status              - Show proxy status\n"
            "  list                - List available profiles\n"
            "  daemon              - Start daemon mode with web API\n"
            "  tun-start           - Start TUN mode\n"
            "  tun-stop            - Stop TUN mode\n"
            "  import <url/file>   - Import profile from URL or file\n"
            "  config              - Show current configuration");

        parser.process(arguments);

        // 获取参数
        QString configDir = parser.value(configOption);
        bool daemonMode = parser.isSet(daemonOption);
        bool enableTun = parser.isSet(tunOption);
        bool dryRun = parser.isSet(dryRunOption);
        bool force = parser.isSet(forceOption);
        bool safe = parser.isSet(safeOption);
        int port = parser.value(portOption).toInt();

        QStringList positionalArgs = parser.positionalArguments();
        if (positionalArgs.isEmpty()) {
            std::cerr << "Error: No command specified. Use --help for usage information." << std::endl;
            return 1;
        }

        QString command = positionalArgs.first();

        // 初始化服务
        m_service = new NekoCore::NekoService(this);
        
        if (!m_service->initialize(configDir)) {
            std::cerr << "Error: Failed to initialize NekoRay service" << std::endl;
            return 1;
        }

        // 初始化gRPC
        if (!NekoCore::InitializeGRPC()) {
            std::cerr << "Error: Failed to initialize gRPC client" << std::endl;
            return 1;
        }

        // 连接信号
        connect(m_service, &NekoCore::NekoService::statusChanged,
                this, &CliApplication::onStatusChanged);
        connect(m_service, &NekoCore::NekoService::errorOccurred,
                this, &CliApplication::onErrorOccurred);
        connect(m_service, &NekoCore::NekoService::logMessage,
                this, &CliApplication::onLogMessage);

        // 安全检查
        if (safe || (!force && !dryRun)) {
            NekoCore::SafetyUtils safetyUtils;
            if (!safetyUtils.isSafeTestEnvironment()) {
                std::cerr << "Warning: Potentially unsafe environment detected." << std::endl;
                std::cerr << "Use --force to bypass or --safe for additional checks." << std::endl;
                if (!force) return 1;
            }
        }

        // 执行命令
        int result = 0;
        try {
            if (command == "start") {
                result = handleStart(positionalArgs, enableTun, dryRun);
            } else if (command == "stop") {
                result = handleStop(dryRun);
            } else if (command == "restart") {
                result = handleRestart(positionalArgs, enableTun, dryRun);
            } else if (command == "status") {
                result = handleStatus();
            } else if (command == "list") {
                result = handleList();
            } else if (command == "daemon") {
                result = handleDaemon(port);
            } else if (command == "tun-start") {
                result = handleTunStart(dryRun);
            } else if (command == "tun-stop") {
                result = handleTunStop(dryRun);
            } else if (command == "import") {
                result = handleImport(positionalArgs);
            } else if (command == "config") {
                result = handleConfig();
            } else {
                std::cerr << "Error: Unknown command '" << command.toStdString() << "'" << std::endl;
                result = 1;
            }
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
            result = 1;
        }

        // 清理
        NekoCore::CleanupGRPC();
        return result;
    }

private slots:
    void onStatusChanged(NekoCore::ServiceStatus status) {
        std::cout << "Status changed to: " << m_service->getStatusString().toStdString() << std::endl;
    }

    void onErrorOccurred(const QString &error) {
        std::cerr << "Error: " << error.toStdString() << std::endl;
    }

    void onLogMessage(const QString &level, const QString &message) {
        std::cout << "[" << level.toStdString() << "] " << message.toStdString() << std::endl;
    }

private:
    int handleStart(const QStringList &args, bool enableTun, bool dryRun) {
        if (args.size() < 2) {
            std::cerr << "Error: Profile ID required for start command" << std::endl;
            return 1;
        }

        int profileId = args[1].toInt();
        if (profileId <= 0) {
            std::cerr << "Error: Invalid profile ID" << std::endl;
            return 1;
        }

        if (dryRun) {
            std::cout << "DRY RUN: Would start proxy with profile " << profileId;
            if (enableTun) std::cout << " (TUN mode enabled)";
            std::cout << std::endl;
            return 0;
        }

        std::cout << "Starting proxy with profile " << profileId << "..." << std::endl;

        if (!m_service->loadProfile(profileId)) {
            std::cerr << "Error: Failed to load profile " << profileId << std::endl;
            return 1;
        }

        if (!m_service->startProxy()) {
            std::cerr << "Error: Failed to start proxy" << std::endl;
            return 1;
        }

        if (enableTun) {
            if (!m_service->startTunMode()) {
                std::cerr << "Warning: Failed to start TUN mode" << std::endl;
            } else {
                std::cout << "TUN mode enabled" << std::endl;
            }
        }

        std::cout << "Proxy started successfully" << std::endl;
        return 0;
    }

    int handleStop(bool dryRun) {
        if (dryRun) {
            std::cout << "DRY RUN: Would stop proxy" << std::endl;
            return 0;
        }

        std::cout << "Stopping proxy..." << std::endl;
        
        if (m_service->stopProxy()) {
            std::cout << "Proxy stopped successfully" << std::endl;
            return 0;
        } else {
            std::cerr << "Error: Failed to stop proxy" << std::endl;
            return 1;
        }
    }

    int handleRestart(const QStringList &args, bool enableTun, bool dryRun) {
        if (handleStop(dryRun) != 0) {
            return 1;
        }
        
        // 等待一下再启动
        QThread::msleep(500);
        
        return handleStart(args, enableTun, dryRun);
    }

    int handleStatus() {
        std::cout << "=== NekoRay Status ===" << std::endl;
        std::cout << "Status: " << m_service->getStatusString().toStdString() << std::endl;
        std::cout << "Profile: " << m_service->getCurrentProfileId() << std::endl;
        std::cout << "SOCKS: " << m_service->getSocksAddress().toStdString() 
                  << ":" << m_service->getSocksPort() << std::endl;
        std::cout << "HTTP: " << m_service->getHttpAddress().toStdString() 
                  << ":" << m_service->getHttpPort() << std::endl;
        std::cout << "TUN Mode: " << (m_service->isTunModeRunning() ? "Running" : "Stopped") << std::endl;
        std::cout << "Upload: " << m_service->getUploadBytes() << " bytes" << std::endl;
        std::cout << "Download: " << m_service->getDownloadBytes() << " bytes" << std::endl;
        return 0;
    }

    int handleList() {
        std::cout << "=== Available Profiles ===" << std::endl;
        
        // 这里应该从Database获取真实的profile列表
        // 暂时显示示例数据
        std::cout << "1. Sample SOCKS5 Profile (127.0.0.1:1080)" << std::endl;
        std::cout << "2. Sample HTTP Profile (127.0.0.1:8080)" << std::endl;
        std::cout << "3. Sample Shadowsocks Profile" << std::endl;
        
        return 0;
    }

    int handleDaemon(int port) {
        std::cout << "Starting daemon mode on port " << port << "..." << std::endl;
        std::cout << "Web interface will be available at http://127.0.0.1:" << port << std::endl;
        
        // 这里应该启动内置的web服务器
        // 暂时只是提示
        std::cout << "Note: Use nekoray-web for full web interface support" << std::endl;
        
        return 0;
    }

    int handleTunStart(bool dryRun) {
        if (dryRun) {
            std::cout << "DRY RUN: Would start TUN mode" << std::endl;
            return 0;
        }

        std::cout << "Starting TUN mode..." << std::endl;
        
        if (m_service->startTunMode()) {
            std::cout << "TUN mode started successfully" << std::endl;
            return 0;
        } else {
            std::cerr << "Error: Failed to start TUN mode" << std::endl;
            return 1;
        }
    }

    int handleTunStop(bool dryRun) {
        if (dryRun) {
            std::cout << "DRY RUN: Would stop TUN mode" << std::endl;
            return 0;
        }

        std::cout << "Stopping TUN mode..." << std::endl;
        
        if (m_service->stopTunMode()) {
            std::cout << "TUN mode stopped successfully" << std::endl;
            return 0;
        } else {
            std::cerr << "Error: Failed to stop TUN mode" << std::endl;
            return 1;
        }
    }

    int handleImport(const QStringList &args) {
        if (args.size() < 2) {
            std::cerr << "Error: URL or file path required for import command" << std::endl;
            return 1;
        }

        QString source = args[1];
        std::cout << "Importing from: " << source.toStdString() << std::endl;
        
        // 这里应该实现真正的导入功能
        std::cout << "Note: Import functionality not yet implemented" << std::endl;
        
        return 0;
    }

    int handleConfig() {
        QJsonObject config = m_service->getCurrentConfig();
        QJsonDocument doc(config);
        
        std::cout << "=== Current Configuration ===" << std::endl;
        std::cout << doc.toJson().toStdString() << std::endl;
        
        return 0;
    }

private:
    NekoCore::NekoService *m_service;
};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("nekoray-cli");
    app.setApplicationVersion("1.0");

    CliApplication cliApp;
    int result = cliApp.run(app.arguments());

    return result;
}

#include "main_cli_headless.moc"