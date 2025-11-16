#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QTimer>
#include <QTextStream>
#include <QSignalMapper>
#include <QThread>

#include "../core/NekoService.hpp"
#include "../web/WebApiServer.hpp"

#include <csignal>

class DaemonApplication : public QCoreApplication {
    Q_OBJECT

public:
    DaemonApplication(int &argc, char **argv) : QCoreApplication(argc, argv) {
        setApplicationName("nekoray-daemon");
        setApplicationVersion("1.0");
        
        // Setup signal handling for graceful shutdown
        signal(SIGINT, DaemonApplication::signalHandler);
        signal(SIGTERM, DaemonApplication::signalHandler);
        
        m_service = nullptr;
        m_webServer = nullptr;
    }

    static void signalHandler(int signal) {
        Q_UNUSED(signal)
        qDebug() << "Received shutdown signal, cleaning up...";
        QCoreApplication::quit();
    }

    int run() {
        QCommandLineParser parser;
        parser.setApplicationDescription("NekoRay Daemon - Headless V2Ray Service with Web API");
        parser.addHelpOption();
        parser.addVersionOption();

        // Define options
        QCommandLineOption configDirOption({"c", "config"}, 
            "Configuration directory", "directory");
        QCommandLineOption portOption({"p", "port"}, 
            "Web API server port (default: 8080)", "port", "8080");
        QCommandLineOption verboseOption({"v", "verbose"}, 
            "Enable verbose logging");
        QCommandLineOption autoStartOption({"a", "auto-start"}, 
            "Auto-start proxy with profile ID", "profile_id");
        QCommandLineOption tunOption({"t", "tun"}, 
            "Auto-start TUN mode");
        QCommandLineOption bindOption({"b", "bind"}, 
            "Bind address for web API (default: 0.0.0.0)", "address", "0.0.0.0");

        parser.addOption(configDirOption);
        parser.addOption(portOption);
        parser.addOption(verboseOption);
        parser.addOption(autoStartOption);
        parser.addOption(tunOption);
        parser.addOption(bindOption);

        parser.process(arguments());

        m_verbose = parser.isSet(verboseOption);

        // Initialize service
        m_service = new NekoCore::NekoService(this);
        
        QString configDir = parser.value(configDirOption);
        if (!m_service->initialize(configDir)) {
            qCritical() << "Failed to initialize service";
            return 1;
        }

        // Connect signals
        connect(m_service, &NekoCore::NekoService::logMessage,
                this, &DaemonApplication::onLogMessage);
        connect(m_service, &NekoCore::NekoService::errorOccurred,
                this, &DaemonApplication::onError);
        connect(m_service, &NekoCore::NekoService::statusChanged,
                this, &DaemonApplication::onStatusChanged);

        // Initialize web server
        m_webServer = new NekoWeb::WebApiServer(this);
        int webPort = parser.value(portOption).toInt();
        
        if (!m_webServer->start(webPort, m_service)) {
            qCritical() << "Failed to start web server";
            return 1;
        }

        QTextStream out(stdout);
        out << "NekoRay Daemon started successfully" << Qt::endl;
        out << "Web interface: http://localhost:" << webPort << Qt::endl;
        out << "Configuration directory: " << (configDir.isEmpty() ? "default" : configDir) << Qt::endl;

        // Auto-start proxy if requested
        if (parser.isSet(autoStartOption)) {
            bool ok;
            int profileId = parser.value(autoStartOption).toInt(&ok);
            if (ok) {
                QTimer::singleShot(1000, [this, profileId]() {
                    autoStartProxy(profileId);
                });
            } else {
                qWarning() << "Invalid profile ID for auto-start:" << parser.value(autoStartOption);
            }
        }

        // Auto-start TUN if requested
        if (parser.isSet(tunOption)) {
            QTimer::singleShot(2000, [this]() {
                autoStartTun();
            });
        }

        // Setup cleanup on application quit
        connect(this, &QCoreApplication::aboutToQuit, this, &DaemonApplication::cleanup);

        return exec();
    }

private slots:
    void onLogMessage(const QString &level, const QString &message) {
        if (m_verbose || level == "error" || level == "warn") {
            QTextStream out(stdout);
            QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
            out << QString("[%1] [%2] %3").arg(timestamp, level.toUpper(), message) << Qt::endl;
        }
    }

    void onError(const QString &error) {
        QTextStream err(stderr);
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        err << QString("[%1] ERROR: %2").arg(timestamp, error) << Qt::endl;
    }

    void onStatusChanged(NekoCore::ServiceStatus status) {
        if (m_verbose) {
            QTextStream out(stdout);
            QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
            out << QString("[%1] Status changed to: %2").arg(timestamp, m_service->getStatusString()) << Qt::endl;
        }

        if (status == NekoCore::ServiceStatus::Running) {
            QTextStream out(stdout);
            out << "Proxy service is now running:" << Qt::endl;
            out << "  SOCKS5: " << m_service->getSocksAddress() << ":" << m_service->getSocksPort() << Qt::endl;
            out << "  HTTP: " << m_service->getHttpAddress() << ":" << m_service->getHttpPort() << Qt::endl;
            if (m_service->isTunModeRunning()) {
                out << "  TUN: Enabled" << Qt::endl;
            }
        }
    }

    void autoStartProxy(int profileId) {
        QTextStream out(stdout);
        out << "Auto-starting proxy with profile " << profileId << "..." << Qt::endl;

        if (!m_service->loadProfile(profileId)) {
            qCritical() << "Failed to load profile for auto-start:" << profileId;
            return;
        }

        if (!m_service->startProxy()) {
            qCritical() << "Failed to auto-start proxy";
            return;
        }

        out << "Proxy auto-started successfully" << Qt::endl;
    }

    void autoStartTun() {
        QTextStream out(stdout);
        out << "Auto-starting TUN mode..." << Qt::endl;

        if (!m_service->startTunMode()) {
            qWarning() << "Failed to auto-start TUN mode";
            return;
        }

        out << "TUN mode auto-started successfully" << Qt::endl;
    }

    void cleanup() {
        QTextStream out(stdout);
        out << "Shutting down daemon..." << Qt::endl;

        if (m_service) {
            m_service->shutdown();
        }

        if (m_webServer) {
            m_webServer->stop();
        }

        out << "Daemon shutdown complete" << Qt::endl;
    }

private:
    NekoCore::NekoService *m_service;
    NekoWeb::WebApiServer *m_webServer;
    bool m_verbose = false;
};

int main(int argc, char *argv[]) {
    // Disable GUI components
    qputenv("QT_QPA_PLATFORM", "offscreen");
    
    DaemonApplication app(argc, argv);
    return app.run();
}

#include "main_daemon.moc"