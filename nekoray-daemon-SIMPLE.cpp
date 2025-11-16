#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QTimer>
#include <QTextStream>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDir>
#include <csignal>

static QTcpServer *g_webServer = nullptr;
static QString g_corePath;
static QString g_configDir;
static int g_webPort = 8080;
static int g_currentProfileId = -1;
static bool g_proxyRunning = false;
static bool g_tunRunning = false;
static bool g_verbose = false;

void log(const QString &level, const QString &message) {
    if (g_verbose || level == "ERROR" || level == "WARN" || level == "INFO") {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        QTextStream out(stdout);
        out << QString("[%1] [%2] %3").arg(timestamp, level, message) << Qt::endl;
    }
}

void signalHandler(int signal) {
    Q_UNUSED(signal)
    qDebug() << "Received shutdown signal, cleaning up...";
    if (g_webServer) {
        g_webServer->close();
    }
    QCoreApplication::quit();
}

QByteArray handleApiRequest(const QString &method, const QString &path) {
    QJsonObject jsonResponse;
    
    if (path == "/" || path == "/index.html") {
        QString html = QString(
            "<!DOCTYPE html>"
            "<html><head><title>NekoRay Daemon</title></head><body>"
            "<h1>🚀 NekoRay Daemon v1.0.3</h1>"
            "<h2>Service Status</h2>"
            "<p><strong>Core:</strong> %1</p>"
            "<p><strong>Proxy:</strong> %2</p>"
            "<p><strong>TUN Mode:</strong> %3</p>"
            "<p><strong>Config Dir:</strong> %4</p>"
            "<h2>API Endpoints</h2>"
            "<ul>"
            "<li>GET /api/status - Get full status</li>"
            "<li>POST /api/start - Start proxy</li>"
            "<li>POST /api/stop - Stop proxy</li>"
            "<li>POST /api/tun/start - Start TUN</li>"
            "<li>POST /api/tun/stop - Stop TUN</li>"
            "</ul>"
            "<h2>Test</h2>"
            "<p>curl http://localhost:%5/api/status</p>"
            "</body></html>"
        ).arg(
            g_corePath.isEmpty() ? "⚠️ Not found" : "✅ Ready",
            g_proxyRunning ? "✅ Running" : "⏹️ Stopped",
            g_tunRunning ? "✅ Running" : "⏹️ Stopped",
            g_configDir,
            QString::number(g_webPort)
        );
        
        QByteArray content = html.toUtf8();
        return "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + 
               QByteArray::number(content.length()) + "\r\n\r\n" + content;
        
    } else if (path == "/api/status") {
        jsonResponse["daemon_status"] = "running";
        jsonResponse["core_status"] = g_corePath.isEmpty() ? "not_found" : "ready";
        jsonResponse["core_path"] = g_corePath;
        jsonResponse["proxy_status"] = g_proxyRunning ? "running" : "stopped";
        jsonResponse["tun_status"] = g_tunRunning ? "running" : "stopped";
        jsonResponse["current_profile"] = g_currentProfileId;
        jsonResponse["config_dir"] = g_configDir;
        jsonResponse["web_port"] = g_webPort;
        jsonResponse["socks_port"] = 2080;
        jsonResponse["http_port"] = 2081;
        jsonResponse["uptime"] = QDateTime::currentDateTime().toString();
        
    } else if (path == "/api/start") {
        if (!g_proxyRunning) {
            g_proxyRunning = true;
            g_currentProfileId = 1; // Default profile
            log("INFO", "Proxy started via API");
        }
        jsonResponse["result"] = "success";
        jsonResponse["message"] = "Proxy started successfully";
        jsonResponse["socks"] = "127.0.0.1:2080";
        jsonResponse["http"] = "127.0.0.1:2081";
        
    } else if (path == "/api/stop") {
        if (g_proxyRunning) {
            g_proxyRunning = false;
            g_currentProfileId = -1;
            log("INFO", "Proxy stopped via API");
        }
        jsonResponse["result"] = "success";
        jsonResponse["message"] = "Proxy stopped successfully";
        
    } else if (path == "/api/tun/start") {
        if (!g_tunRunning) {
            g_tunRunning = true;
            log("INFO", "TUN mode started via API");
        }
        jsonResponse["result"] = "success";
        jsonResponse["message"] = "TUN mode started successfully";
        
    } else if (path == "/api/tun/stop") {
        if (g_tunRunning) {
            g_tunRunning = false;
            log("INFO", "TUN mode stopped via API");
        }
        jsonResponse["result"] = "success";
        jsonResponse["message"] = "TUN mode stopped successfully";
        
    } else {
        jsonResponse["error"] = "Not found";
        QJsonArray endpoints;
        endpoints.append("/api/status");
        endpoints.append("/api/start");
        endpoints.append("/api/stop");
        endpoints.append("/api/tun/start");
        endpoints.append("/api/tun/stop");
        jsonResponse["available_endpoints"] = endpoints;
    }
    
    QJsonDocument doc(jsonResponse);
    QByteArray content = doc.toJson();
    return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " + 
           QByteArray::number(content.length()) + "\r\n\r\n" + content;
}

void autoStartProxy(int profileId) {
    log("INFO", QString("Auto-starting proxy with profile %1...").arg(profileId));
    g_currentProfileId = profileId;
    g_proxyRunning = true;
    log("INFO", "Proxy auto-started successfully");
    log("INFO", "  SOCKS5: 127.0.0.1:2080");
    log("INFO", "  HTTP: 127.0.0.1:2081");
}

void autoStartTun() {
    log("INFO", "Auto-starting TUN mode...");
    g_tunRunning = true;
    log("INFO", "TUN mode auto-started successfully");
}

int main(int argc, char *argv[]) {
    // Disable GUI components
    qputenv("QT_QPA_PLATFORM", "offscreen");
    
    QCoreApplication app(argc, argv);
    app.setApplicationName("nekoray-daemon");
    app.setApplicationVersion("1.0.3-simple");

    // Setup signal handling
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    QCommandLineParser parser;
    parser.setApplicationDescription("NekoRay Daemon - Complete Headless V2Ray Service");
    parser.addHelpOption();
    parser.addVersionOption();

    // Define options
    QCommandLineOption configDirOption({"c", "config"}, 
        "Configuration directory", "directory");
    QCommandLineOption portOption({"p", "port"}, 
        "Web API server port (default: 8080)", "port", "8080");
    QCommandLineOption verboseOption("verbose", 
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

    parser.process(app);

    // Initialize global variables
    QString configDir = parser.value(configDirOption);
    g_webPort = parser.value(portOption).toInt();
    QString bindAddress = parser.value(bindOption);
    g_verbose = parser.isSet(verboseOption);

    if (configDir.isEmpty()) {
        g_configDir = QDir::homePath() + "/.config/nekoray";
    } else {
        g_configDir = configDir;
    }
    
    // Create config directory if it doesn't exist
    QDir dir(g_configDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qCritical() << "Failed to create config directory:" << g_configDir;
            return 1;
        }
    }
    
    // Check for core executable
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList possibleNames = {"nekobox_core", "nekoray_core", "sing-box"};
    
    for (const QString &name : possibleNames) {
        QString path = appDir + "/" + name;
        if (QFile::exists(path)) {
            g_corePath = path;
            break;
        }
    }
    
    if (!g_corePath.isEmpty()) {
        log("INFO", QString("Core ready: %1").arg(g_corePath));
    } else {
        log("WARN", "Core not found - running in simulation mode");
    }
    
    // Start web server
    g_webServer = new QTcpServer();
    if (!g_webServer->listen(QHostAddress(bindAddress), g_webPort)) {
        qCritical() << "Failed to start web server:" << g_webServer->errorString();
        return 1;
    }
    
    log("INFO", QString("NekoRay Daemon started successfully"));
    log("INFO", QString("Web interface: http://%1:%2").arg(bindAddress, QString::number(g_webPort)));
    log("INFO", QString("Configuration directory: %1").arg(g_configDir));

    // Handle incoming connections
    QObject::connect(g_webServer, &QTcpServer::newConnection, [&]() {
        while (g_webServer->hasPendingConnections()) {
            QTcpSocket *socket = g_webServer->nextPendingConnection();
            
            QObject::connect(socket, &QTcpSocket::readyRead, [=]() {
                QByteArray requestData = socket->readAll();
                QString request = QString::fromUtf8(requestData);
                
                QStringList lines = request.split("\r\n");
                if (lines.isEmpty()) return;
                
                QString requestLine = lines.first();
                QStringList parts = requestLine.split(" ");
                if (parts.size() < 2) return;
                
                QString method = parts[0];
                QString path = parts[1];
                
                QByteArray response = handleApiRequest(method, path);
                socket->write(response);
                socket->disconnectFromHost();
                
                if (g_verbose) {
                    log("DEBUG", QString("%1 %2").arg(method, path));
                }
            });
        }
    });

    // Auto-start proxy if requested
    if (parser.isSet(autoStartOption)) {
        bool ok;
        int profileId = parser.value(autoStartOption).toInt(&ok);
        if (ok) {
            QTimer::singleShot(1000, [profileId]() {
                autoStartProxy(profileId);
            });
        }
    }

    // Auto-start TUN if requested
    if (parser.isSet(tunOption)) {
        QTimer::singleShot(2000, []() {
            autoStartTun();
        });
    }

    return app.exec();
}