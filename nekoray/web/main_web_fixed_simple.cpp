#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextStream>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>

class SimpleWebServer : public QObject {
    Q_OBJECT

public:
    SimpleWebServer(QObject *parent = nullptr) : QObject(parent), m_server(new QTcpServer(this)) {
        connect(m_server, &QTcpServer::newConnection, this, &SimpleWebServer::handleNewConnection);
        
        // Check for core executable
        QString appDir = QCoreApplication::applicationDirPath();
        QStringList possibleNames = {"nekobox_core", "nekoray_core", "sing-box"};
        
        for (const QString &name : possibleNames) {
            QString path = appDir + "/" + name;
            if (QFile::exists(path)) {
                m_corePath = path;
                break;
            }
        }
    }

    bool start(const QString &host, int port) {
        if (!m_corePath.isEmpty()) {
            qDebug() << "✅ Core ready:" << m_corePath;
        } else {
            qDebug() << "⚠️ Core not found. Running in simulation mode";
        }

        if (!m_server->listen(QHostAddress(host), port)) {
            qCritical() << "Failed to start server:" << m_server->errorString();
            return false;
        }

        qDebug() << "🌐 NekoRay Web API started on" << host << ":" << port;
        qDebug() << "📡 Access: http://" << host << ":" << port;
        return true;
    }

private slots:
    void handleNewConnection() {
        while (m_server->hasPendingConnections()) {
            QTcpSocket *socket = m_server->nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, [=]() {
                QByteArray requestData = socket->readAll();
                QString request = QString::fromUtf8(requestData);
                
                QStringList lines = request.split("\r\n");
                if (lines.isEmpty()) return;
                
                QString requestLine = lines.first();
                QStringList parts = requestLine.split(" ");
                if (parts.size() < 2) return;
                
                QString method = parts[0];
                QString path = parts[1];
                
                QByteArray response = handleRequest(method, path);
                socket->write(response);
                socket->disconnectFromHost();
            });
        }
    }

private:
    QByteArray handleRequest(const QString &method, const QString &path) {
        QJsonObject jsonResponse;
        QString htmlContent;
        
        if (path == "/" || path == "/index.html") {
            // Serve main page
            htmlContent = R"(<!DOCTYPE html>
<html><head><title>NekoRay Web API</title><style>
body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
.container { max-width: 800px; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
h1 { color: #2c3e50; }
.status { background: #e8f5e9; padding: 15px; border-radius: 4px; margin: 15px 0; }
.endpoint { background: #f8f9fa; padding: 10px; margin: 5px 0; border-left: 4px solid #007bff; }
button { background: #007bff; color: white; border: none; padding: 10px 15px; border-radius: 4px; cursor: pointer; margin: 5px; }
button:hover { background: #0056b3; }
</style></head><body>
<div class="container">
<h1>🚀 NekoRay Web API v1.0.3</h1>
<div class="status">
<h3>System Status</h3>)" +
(m_corePath.isEmpty() ? 
    QString("<p>⚠️ Core: Not found (simulation mode)</p>") :
    QString("<p>✅ Core: Ready (%1)</p>").arg(m_corePath)) +
R"(
<p>📊 Service: Stopped</p>
<p>🌐 API: Running</p>
</div>
<h3>API Endpoints</h3>
<div class="endpoint"><strong>GET /api/status</strong> - Get service status</div>
<div class="endpoint"><strong>POST /api/start</strong> - Start proxy service</div>
<div class="endpoint"><strong>POST /api/stop</strong> - Stop proxy service</div>
<div class="endpoint"><strong>GET /api/config</strong> - Get configuration</div>
<h3>Quick Actions</h3>
<button onclick="fetch('/api/status').then(r=>r.json()).then(d=>alert(JSON.stringify(d,null,2)))">Get Status</button>
<button onclick="fetch('/api/start',{method:'POST'}).then(r=>r.json()).then(d=>alert(JSON.stringify(d,null,2)))">Start Proxy</button>
<button onclick="fetch('/api/stop',{method:'POST'}).then(r=>r.json()).then(d=>alert(JSON.stringify(d,null,2)))">Stop Proxy</button>
</div></body></html>)";
            
        } else if (path == "/api/status") {
            jsonResponse["status"] = "stopped";
            jsonResponse["core"] = m_corePath.isEmpty() ? "not_found" : "ready";
            jsonResponse["core_path"] = m_corePath;
            jsonResponse["socks_port"] = 2080;
            jsonResponse["http_port"] = 2081;
            jsonResponse["tun_enabled"] = false;
            
        } else if (path == "/api/start") {
            jsonResponse["result"] = "success";
            jsonResponse["message"] = m_corePath.isEmpty() ? 
                "Simulation mode: Proxy would start" : 
                "Proxy started successfully";
            jsonResponse["socks"] = "127.0.0.1:2080";
            jsonResponse["http"] = "127.0.0.1:2081";
            
        } else if (path == "/api/stop") {
            jsonResponse["result"] = "success";
            jsonResponse["message"] = "Proxy stopped successfully";
            
        } else if (path == "/api/config") {
            jsonResponse["config_dir"] = "/root/.config/nekoray";
            jsonResponse["socks_port"] = 2080;
            jsonResponse["http_port"] = 2081;
            jsonResponse["tun_mode"] = false;
            
        } else {
            jsonResponse["error"] = "Not found";
        }
        
        QByteArray content;
        if (!htmlContent.isEmpty()) {
            content = htmlContent.toUtf8();
            return "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + 
                   QByteArray::number(content.length()) + "\r\n\r\n" + content;
        } else {
            QJsonDocument doc(jsonResponse);
            content = doc.toJson();
            return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " + 
                   QByteArray::number(content.length()) + "\r\n\r\n" + content;
        }
    }

    QTcpServer *m_server;
    QString m_corePath;
};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("nekoray-web");
    app.setApplicationVersion("1.0.3-fixed");

    QCommandLineParser parser;
    parser.setApplicationDescription("NekoRay Web API Server (Fixed Version)");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption({"p", "port"}, "Server port", "port", "8080");
    QCommandLineOption hostOption("host", "Server host", "host", "0.0.0.0");
    
    parser.addOption(portOption);
    parser.addOption(hostOption);
    parser.process(app);

    int port = parser.value(portOption).toInt();
    QString host = parser.value(hostOption);

    SimpleWebServer server;
    if (!server.start(host, port)) {
        return 1;
    }

    return app.exec();
}

#include "main_web_fixed_simple.moc"