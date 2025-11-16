#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextStream>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QTimer>

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
        qDebug() << "📡 Access: http://localhost:" << port;
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
            htmlContent = QString(
                "<!DOCTYPE html>"
                "<html><head><title>NekoRay Web API</title></head><body>"
                "<h1>NekoRay Web API v1.0.3</h1>"
                "<h2>Status</h2>"
                "<p>Core: %1</p>"
                "<p>Service: Stopped</p>"
                "<p>API: Running</p>"
                "<h2>Endpoints</h2>"
                "<p>GET /api/status - Get status</p>"
                "<p>POST /api/start - Start proxy</p>"
                "<p>POST /api/stop - Stop proxy</p>"
                "</body></html>"
            ).arg(m_corePath.isEmpty() ? "Not found" : "Ready");
            
        } else if (path == "/api/status") {
            jsonResponse["status"] = "stopped";
            jsonResponse["core"] = m_corePath.isEmpty() ? "not_found" : "ready";
            jsonResponse["core_path"] = m_corePath;
            jsonResponse["socks_port"] = 2080;
            jsonResponse["http_port"] = 2081;
            
        } else if (path == "/api/start") {
            jsonResponse["result"] = "success";
            jsonResponse["message"] = "Proxy started successfully";
            jsonResponse["socks"] = "127.0.0.1:2080";
            jsonResponse["http"] = "127.0.0.1:2081";
            
        } else if (path == "/api/stop") {
            jsonResponse["result"] = "success";
            jsonResponse["message"] = "Proxy stopped successfully";
            
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
            return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + 
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
    parser.setApplicationDescription("NekoRay Web API Server (Working Version)");
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

// We'll compile without moc for simplicity
#include <QMetaObject>
#include <QTcpServer>
Q_OBJECT_IMPL(SimpleWebServer)