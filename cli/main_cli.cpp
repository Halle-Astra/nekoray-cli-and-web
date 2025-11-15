#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QTimer>
#include <QTextStream>

#include "../core/NekoService.hpp"
#include "../core/SafetyUtils.hpp"

class CliApplication : public QObject {
    Q_OBJECT

public:
    CliApplication(QObject *parent = nullptr) : QObject(parent), m_service(nullptr) {}

    int run(const QStringList &arguments) {
        QCommandLineParser parser;
        parser.setApplicationDescription("NekoRay CLI - Headless V2Ray Client");
        parser.addHelpOption();
        parser.addVersionOption();

        // Define options
        QCommandLineOption configDirOption({"c", "config"}, 
            "Configuration directory", "directory");
        QCommandLineOption daemonOption({"d", "daemon"}, 
            "Run as daemon (background service)");
        QCommandLineOption verboseOption({"v", "verbose"}, 
            "Enable verbose logging");
        QCommandLineOption tunOption({"t", "tun"}, 
            "Enable TUN mode");
        QCommandLineOption dryRunOption({"n", "dry-run"}, 
            "Validate operations without executing (safe mode)");
        QCommandLineOption forceOption({"f", "force"}, 
            "Force operations even if safety checks fail");
        QCommandLineOption safetyOption({"s", "safe"}, 
            "Enable additional safety checks");
        QCommandLineOption portOption({"p", "port"}, 
            "Web API server port (default: 8080)", "port", "8080");

        parser.addOption(configDirOption);
        parser.addOption(daemonOption);
        parser.addOption(verboseOption);
        parser.addOption(tunOption);
        parser.addOption(dryRunOption);
        parser.addOption(forceOption);
        parser.addOption(safetyOption);
        parser.addOption(portOption);

        // Define commands
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

        m_service = new NekoCore::NekoService(this);
        
        // Initialize service
        QString configDir = parser.value(configDirOption);
        if (!m_service->initialize(configDir)) {
            qCritical() << "Failed to initialize service";
            return 1;
        }

        // Connect signals for logging
        connect(m_service, &NekoCore::NekoService::logMessage,
                this, &CliApplication::onLogMessage);
        connect(m_service, &NekoCore::NekoService::errorOccurred,
                this, &CliApplication::onError);
        connect(m_service, &NekoCore::NekoService::statusChanged,
                this, &CliApplication::onStatusChanged);

        m_verbose = parser.isSet(verboseOption);
        m_dryRun = parser.isSet(dryRunOption);
        m_force = parser.isSet(forceOption);
        m_enableSafety = parser.isSet(safetyOption);

        // Safety checks
        if (m_enableSafety && !m_force) {
            if (!NekoCore::SafetyUtils::isSafeTestEnvironment()) {
                qCritical() << "Safety check failed: Not in safe environment";
                qCritical() << "Use --force to override, or run in VM/test environment";
                return 1;
            }
        }

        const QStringList positionalArgs = parser.positionalArguments();
        if (positionalArgs.isEmpty()) {
            parser.showHelp(1);
            return 1;
        }

        QString command = positionalArgs.first();
        
        if (command == "start") {
            return handleStart(positionalArgs);
        } else if (command == "stop") {
            return handleStop();
        } else if (command == "restart") {
            return handleRestart(positionalArgs);
        } else if (command == "status") {
            return handleStatus();
        } else if (command == "list") {
            return handleList();
        } else if (command == "daemon") {
            bool enableTun = parser.isSet(tunOption);
            int webPort = parser.value(portOption).toInt();
            return handleDaemon(enableTun, webPort);
        } else if (command == "tun-start") {
            return handleTunStart();
        } else if (command == "tun-stop") {
            return handleTunStop();
        } else if (command == "import") {
            return handleImport(positionalArgs);
        } else if (command == "config") {
            return handleConfig();
        } else {
            qCritical() << "Unknown command:" << command;
            parser.showHelp(1);
            return 1;
        }
    }

private slots:
    void onLogMessage(const QString &level, const QString &message) {
        if (m_verbose || level == "error" || level == "warn") {
            QTextStream out(stdout);
            out << QString("[%1] %2").arg(level.toUpper(), message) << Qt::endl;
        }
    }

    void onError(const QString &error) {
        QTextStream err(stderr);
        err << "ERROR: " << error << Qt::endl;
    }

    void onStatusChanged(NekoCore::ServiceStatus status) {
        Q_UNUSED(status)
        if (m_verbose) {
            QTextStream out(stdout);
            out << "Status: " << m_service->getStatusString() << Qt::endl;
        }
    }

private:
    int handleStart(const QStringList &args) {
        if (args.size() < 2) {
            qCritical() << "Profile ID required for start command";
            return 1;
        }

        bool ok;
        int profileId = args[1].toInt(&ok);
        if (!ok) {
            qCritical() << "Invalid profile ID:" << args[1];
            return 1;
        }

        // Dry run mode
        if (m_dryRun) {
            QVariantMap params;
            params["profile_id"] = profileId;
            if (NekoCore::SafetyUtils::validateOperationDryRun("start_proxy", params)) {
                QTextStream out(stdout);
                out << "DRY RUN: Would start proxy with profile " << profileId << Qt::endl;
                return 0;
            } else {
                qCritical() << "DRY RUN: Validation failed";
                return 1;
            }
        }

        if (!m_service->loadProfile(profileId)) {
            qCritical() << "Failed to load profile:" << profileId;
            return 1;
        }

        if (!m_service->startProxy()) {
            qCritical() << "Failed to start proxy";
            return 1;
        }

        QTextStream out(stdout);
        out << "Proxy started successfully with profile " << profileId << Qt::endl;
        out << "SOCKS5: " << m_service->getSocksAddress() << ":" << m_service->getSocksPort() << Qt::endl;
        out << "HTTP: " << m_service->getHttpAddress() << ":" << m_service->getHttpPort() << Qt::endl;

        return 0;
    }

    int handleStop() {
        if (!m_service->stopProxy()) {
            qCritical() << "Failed to stop proxy";
            return 1;
        }

        QTextStream out(stdout);
        out << "Proxy stopped successfully" << Qt::endl;
        return 0;
    }

    int handleRestart(const QStringList &args) {
        if (args.size() < 2) {
            qCritical() << "Profile ID required for restart command";
            return 1;
        }

        bool ok;
        int profileId = args[1].toInt(&ok);
        if (!ok) {
            qCritical() << "Invalid profile ID:" << args[1];
            return 1;
        }

        if (!m_service->loadProfile(profileId)) {
            qCritical() << "Failed to load profile:" << profileId;
            return 1;
        }

        if (!m_service->restartProxy()) {
            qCritical() << "Failed to restart proxy";
            return 1;
        }

        QTextStream out(stdout);
        out << "Proxy restarted successfully with profile " << profileId << Qt::endl;
        return 0;
    }

    int handleStatus() {
        QTextStream out(stdout);
        out << "Service Status: " << m_service->getStatusString() << Qt::endl;
        out << "Current Profile: " << m_service->getCurrentProfileId() << Qt::endl;
        out << "TUN Mode: " << (m_service->isTunModeRunning() ? "Running" : "Stopped") << Qt::endl;
        
        if (m_service->getStatus() == NekoCore::ServiceStatus::Running) {
            out << "SOCKS5: " << m_service->getSocksAddress() << ":" << m_service->getSocksPort() << Qt::endl;
            out << "HTTP: " << m_service->getHttpAddress() << ":" << m_service->getHttpPort() << Qt::endl;
            out << "Upload: " << formatBytes(m_service->getUploadBytes()) << Qt::endl;
            out << "Download: " << formatBytes(m_service->getDownloadBytes()) << Qt::endl;
        }
        
        return 0;
    }

    int handleList() {
        // TODO: Implement profile listing
        QTextStream out(stdout);
        out << "Profile listing not yet implemented" << Qt::endl;
        return 0;
    }

    int handleDaemon(bool enableTun, int webPort) {
        Q_UNUSED(enableTun)
        Q_UNUSED(webPort)
        
        QTextStream out(stdout);
        out << "Starting daemon mode..." << Qt::endl;
        out << "Web API will be available on port " << webPort << Qt::endl;
        
        // TODO: Start web server
        
        // Keep the application running
        QCoreApplication::exec();
        return 0;
    }

    int handleTunStart() {
        // Safety check for TUN mode
        if (m_enableSafety && !m_force) {
            if (!NekoCore::SafetyUtils::isTunOperationSafe()) {
                qCritical() << "TUN operation not safe in current environment";
                qCritical() << "Use --force to override, or run in safe environment";
                return 1;
            }
        }

        // Dry run mode
        if (m_dryRun) {
            QVariantMap params;
            if (NekoCore::SafetyUtils::validateOperationDryRun("start_tun", params)) {
                QTextStream out(stdout);
                out << "DRY RUN: Would start TUN mode" << Qt::endl;
                return 0;
            } else {
                qCritical() << "DRY RUN: TUN validation failed";
                return 1;
            }
        }

        if (!m_service->startTunMode()) {
            qCritical() << "Failed to start TUN mode";
            return 1;
        }

        QTextStream out(stdout);
        out << "TUN mode started successfully" << Qt::endl;
        return 0;
    }

    int handleTunStop() {
        if (!m_service->stopTunMode()) {
            qCritical() << "Failed to stop TUN mode";
            return 1;
        }

        QTextStream out(stdout);
        out << "TUN mode stopped successfully" << Qt::endl;
        return 0;
    }

    int handleImport(const QStringList &args) {
        if (args.size() < 2) {
            qCritical() << "URL or file path required for import command";
            return 1;
        }

        QString source = args[1];
        
        QTextStream out(stdout);
        out << "Import functionality not yet implemented for: " << source << Qt::endl;
        return 0;
    }

    int handleConfig() {
        QJsonObject config = m_service->getCurrentConfig();
        QJsonDocument doc(config);
        
        QTextStream out(stdout);
        out << doc.toJson() << Qt::endl;
        return 0;
    }

    QString formatBytes(qint64 bytes) {
        const QStringList units = {"B", "KB", "MB", "GB", "TB"};
        double size = bytes;
        int unitIndex = 0;
        
        while (size >= 1024 && unitIndex < units.size() - 1) {
            size /= 1024;
            unitIndex++;
        }
        
        return QString("%1 %2").arg(QString::number(size, 'f', 2), units[unitIndex]);
    }

    NekoCore::NekoService *m_service;
    bool m_verbose = false;
    bool m_dryRun = false;
    bool m_force = false;
    bool m_enableSafety = false;
};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("nekoray-cli");
    app.setApplicationVersion("1.0");

    CliApplication cliApp;
    int result = cliApp.run(app.arguments());
    
    if (result == 0 && app.arguments().contains("daemon")) {
        // If daemon mode, keep running
        return app.exec();
    }
    
    return result;
}

#include "main_cli.moc"