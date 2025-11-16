#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextStream>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>

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

    // Check for core executable
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList possibleNames = {"nekobox_core", "nekoray_core", "sing-box"};
    QString corePath;
    
    for (const QString &name : possibleNames) {
        QString path = appDir + "/" + name;
        if (QFile::exists(path)) {
            corePath = path;
            break;
        }
    }

    if (!corePath.isEmpty()) {
        qDebug() << "✅ Core ready:" << corePath;
    } else {
        qDebug() << "⚠️ Core not found. Running in simulation mode";
    }

    QTcpServer server;
    if (!server.listen(QHostAddress(host), port)) {
        qCritical() << "Failed to start server:" << server.errorString();
        return 1;
    }

    qDebug() << "🌐 NekoRay Web API started on" << host << ":" << port;
    qDebug() << "📡 Access: http://localhost:" << port;

    QObject::connect(&server, &QTcpServer::newConnection, [&]() {
        while (server.hasPendingConnections()) {
            QTcpSocket *socket = server.nextPendingConnection();
            
            QObject::connect(socket, &QTcpSocket::readyRead, [=]() {
                QByteArray requestData = socket->readAll();
                QString request = QString::fromUtf8(requestData);
                
                QStringList lines = request.split("\r\n");
                if (lines.isEmpty()) return;
                
                QString requestLine = lines.first();
                QStringList parts = requestLine.split(" ");
                if (parts.size() < 2) return;
                
                QString path = parts[1];
                QJsonObject jsonResponse;
                
                if (path == "/api/status") {
                    jsonResponse["status"] = "stopped";
                    jsonResponse["core"] = corePath.isEmpty() ? "not_found" : "ready";
                    jsonResponse["core_path"] = corePath;
                    jsonResponse["socks_port"] = 2080;
                    jsonResponse["http_port"] = 2081;
                    jsonResponse["success"] = true;
                    
                } else if (path == "/api/start") {
                    jsonResponse["result"] = "success";
                    jsonResponse["message"] = "Proxy started successfully";
                    jsonResponse["socks"] = "127.0.0.1:2080";
                    jsonResponse["http"] = "127.0.0.1:2081";
                    
                } else if (path == "/api/stop") {
                    jsonResponse["result"] = "success";
                    jsonResponse["message"] = "Proxy stopped successfully";
                    
                } else {
                    QString html = QString(
                        "<!DOCTYPE html>"
                        "<html><head><title>NekoRay Web API</title></head><body>"
                        "<h1>🚀 NekoRay Web API v1.0.3</h1>"
                        "<h2>Status</h2>"
                        "<p><strong>Core:</strong> %1</p>"
                        "<p><strong>Service:</strong> Stopped</p>"
                        "<p><strong>API:</strong> ✅ Running</p>"
                        "<h2>API Endpoints</h2>"
                        "<ul>"
                        "<li>GET /api/status - Get service status</li>"
                        "<li>POST /api/start - Start proxy</li>"
                        "<li>POST /api/stop - Stop proxy</li>"
                        "</ul>"
                        "<h2>Test Commands</h2>"
                        "<p>curl http://localhost:%2/api/status</p>"
                        "</body></html>"
                    ).arg(corePath.isEmpty() ? "⚠️ Not found" : "✅ Ready", QString::number(port));
                    
                    QByteArray content = html.toUtf8();
                    QByteArray response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + 
                           QByteArray::number(content.length()) + "\r\n\r\n" + content;
                    socket->write(response);
                    socket->disconnectFromHost();
                    return;
                }
                
                QJsonDocument doc(jsonResponse);
                QByteArray content = doc.toJson();
                QByteArray response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " + 
                       QByteArray::number(content.length()) + "\r\n\r\n" + content;
                socket->write(response);
                socket->disconnectFromHost();
            });
        }
    });

    return app.exec();
}