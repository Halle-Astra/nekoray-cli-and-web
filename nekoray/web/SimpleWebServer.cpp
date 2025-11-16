#include "SimpleWebServer.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QUrlQuery>
#include <QUrl>
#include <QDebug>
#include <QHostAddress>
#include <QTextStream>

namespace NekoWeb {

SimpleWebServer::SimpleWebServer(NekoCore::NekoService *service, QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_service(service)
    , m_port(0)
{
    connect(m_server, &QTcpServer::newConnection, this, &SimpleWebServer::onNewConnection);
}

SimpleWebServer::~SimpleWebServer() {
    stop();
}

bool SimpleWebServer::start(const QString &host, int port) {
    if (m_server->isListening()) {
        stop();
    }

    QHostAddress address = QHostAddress::LocalHost;
    if (host != "127.0.0.1" && host != "localhost") {
        address = QHostAddress(host);
    }

    if (!m_server->listen(address, port)) {
        emit errorOccurred(QString("Failed to start server: %1").arg(m_server->errorString()));
        return false;
    }

    m_host = host;
    m_port = m_server->serverPort();
    return true;
}

void SimpleWebServer::stop() {
    if (m_server->isListening()) {
        m_server->close();
    }

    for (QTcpSocket *client : m_clients) {
        client->disconnectFromHost();
        client->deleteLater();
    }
    m_clients.clear();
}

bool SimpleWebServer::isRunning() const {
    return m_server->isListening();
}

void SimpleWebServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket *client = m_server->nextPendingConnection();
        m_clients.append(client);
        
        connect(client, &QTcpSocket::readyRead, this, &SimpleWebServer::onReadyRead);
        connect(client, &QTcpSocket::disconnected, this, &SimpleWebServer::onClientDisconnected);
        
        emit clientConnected(client->peerAddress().toString());
    }
}

void SimpleWebServer::onClientDisconnected() {
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (client) {
        m_clients.removeAll(client);
        client->deleteLater();
    }
}

void SimpleWebServer::onReadyRead() {
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QByteArray request = client->readAll();
    handleHttpRequest(client, request);
}

void SimpleWebServer::handleHttpRequest(QTcpSocket *socket, const QByteArray &request) {
    QString requestStr = QString::fromUtf8(request);
    QStringList lines = requestStr.split("\r\n");
    
    if (lines.isEmpty()) {
        sendErrorResponse(socket, "Invalid request", 400);
        return;
    }

    QStringList requestLine = lines[0].split(" ");
    if (requestLine.size() < 3) {
        sendErrorResponse(socket, "Invalid request line", 400);
        return;
    }

    QString method = requestLine[0];
    QString path = requestLine[1];
    
    emit requestReceived(method, path);

    // Handle CORS preflight
    if (method == "OPTIONS") {
        sendResponse(socket, 200, "text/plain", "OK");
        sendCorsHeaders(socket);
        return;
    }

    // Parse JSON body if present
    QJsonObject params;
    if (method == "POST" || method == "PUT") {
        int bodyStart = requestStr.indexOf("\r\n\r\n");
        if (bodyStart > 0) {
            QString body = requestStr.mid(bodyStart + 4);
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(body.toUtf8(), &error);
            if (error.error == QJsonParseError::NoError) {
                params = doc.object();
            }
        }
    }

    // Route to appropriate handler
    if (path == "/" || path.startsWith("/index")) {
        sendResponse(socket, 200, "text/html", getWebInterface());
    }
    else if (path == "/api/status") {
        if (method == "GET") {
            sendJsonResponse(socket, handleApiStatus());
        } else {
            sendErrorResponse(socket, "Method not allowed", 405);
        }
    }
    else if (path == "/api/start") {
        if (method == "POST") {
            sendJsonResponse(socket, handleApiStart(params));
        } else {
            sendErrorResponse(socket, "Method not allowed", 405);
        }
    }
    else if (path == "/api/stop") {
        if (method == "POST") {
            sendJsonResponse(socket, handleApiStop());
        } else {
            sendErrorResponse(socket, "Method not allowed", 405);
        }
    }
    else if (path == "/api/restart") {
        if (method == "POST") {
            sendJsonResponse(socket, handleApiRestart(params));
        } else {
            sendErrorResponse(socket, "Method not allowed", 405);
        }
    }
    else if (path == "/api/profiles") {
        if (method == "GET") {
            sendJsonResponse(socket, handleApiProfiles());
        } else {
            sendErrorResponse(socket, "Method not allowed", 405);
        }
    }
    else if (path == "/api/config") {
        if (method == "GET") {
            sendJsonResponse(socket, handleApiConfig());
        } else {
            sendErrorResponse(socket, "Method not allowed", 405);
        }
    }
    else if (path == "/api/traffic") {
        if (method == "GET") {
            sendJsonResponse(socket, handleApiTraffic());
        } else {
            sendErrorResponse(socket, "Method not allowed", 405);
        }
    }
    else {
        sendErrorResponse(socket, "Not found", 404);
    }
}

void SimpleWebServer::sendResponse(QTcpSocket *socket, int statusCode, const QString &contentType, const QByteArray &body) {
    QString response = QString("HTTP/1.1 %1 OK\r\n"
                              "Content-Type: %2\r\n"
                              "Content-Length: %3\r\n"
                              "Access-Control-Allow-Origin: *\r\n"
                              "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
                              "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
                              "\r\n")
                      .arg(statusCode)
                      .arg(contentType)
                      .arg(body.size());
    
    socket->write(response.toUtf8());
    socket->write(body);
    socket->flush();
    socket->disconnectFromHost();
}

void SimpleWebServer::sendJsonResponse(QTcpSocket *socket, const QJsonObject &data, int statusCode) {
    QJsonDocument doc(data);
    sendResponse(socket, statusCode, "application/json", doc.toJson());
}

void SimpleWebServer::sendErrorResponse(QTcpSocket *socket, const QString &error, int statusCode) {
    QJsonObject errorObj;
    errorObj["error"] = error;
    errorObj["success"] = false;
    sendJsonResponse(socket, errorObj, statusCode);
}

void SimpleWebServer::sendCorsHeaders(QTcpSocket *socket) {
    QString headers = "Access-Control-Allow-Origin: *\r\n"
                     "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
                     "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    socket->write(headers.toUtf8());
}

QJsonObject SimpleWebServer::handleApiStatus() {
    QJsonObject response;
    response["success"] = true;
    response["status"] = m_service->getStatusString();
    response["current_profile"] = m_service->getCurrentProfileId();
    response["socks_address"] = m_service->getSocksAddress();
    response["socks_port"] = m_service->getSocksPort();
    response["http_address"] = m_service->getHttpAddress();
    response["http_port"] = m_service->getHttpPort();
    response["tun_running"] = m_service->isTunModeRunning();
    response["upload_bytes"] = static_cast<qint64>(m_service->getUploadBytes());
    response["download_bytes"] = static_cast<qint64>(m_service->getDownloadBytes());
    response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    return response;
}

QJsonObject SimpleWebServer::handleApiStart(const QJsonObject &params) {
    QJsonObject response;
    
    int profileId = params.value("profile_id").toInt(1);
    bool tunMode = params.value("tun_mode").toBool(false);
    
    if (!m_service->loadProfile(profileId)) {
        response["success"] = false;
        response["error"] = "Failed to load profile";
        return response;
    }
    
    if (!m_service->startProxy()) {
        response["success"] = false;
        response["error"] = "Failed to start proxy";
        return response;
    }
    
    if (tunMode && !m_service->startTunMode()) {
        response["success"] = false;
        response["error"] = "Failed to start TUN mode";
        return response;
    }
    
    response["success"] = true;
    response["message"] = "Proxy started successfully";
    response["profile_id"] = profileId;
    response["tun_mode"] = tunMode && m_service->isTunModeRunning();
    return response;
}

QJsonObject SimpleWebServer::handleApiStop() {
    QJsonObject response;
    
    bool success = m_service->stopProxy();
    m_service->stopTunMode();
    
    response["success"] = success;
    response["message"] = success ? "Proxy stopped successfully" : "Failed to stop proxy";
    return response;
}

QJsonObject SimpleWebServer::handleApiRestart(const QJsonObject &params) {
    QJsonObject response;
    
    if (!m_service->stopProxy()) {
        response["success"] = false;
        response["error"] = "Failed to stop proxy";
        return response;
    }
    
    return handleApiStart(params);
}

QJsonObject SimpleWebServer::handleApiProfiles() {
    QJsonObject response;
    response["success"] = true;
    
    QJsonArray profiles;
    QJsonObject sampleProfile;
    sampleProfile["id"] = 1;
    sampleProfile["name"] = "Sample Profile";
    sampleProfile["type"] = "direct";
    sampleProfile["server"] = "";
    profiles.append(sampleProfile);
    
    response["profiles"] = profiles;
    return response;
}

QJsonObject SimpleWebServer::handleApiConfig() {
    QJsonObject response;
    response["success"] = true;
    response["config"] = m_service->getCurrentConfig();
    return response;
}

QJsonObject SimpleWebServer::handleApiTraffic() {
    QJsonObject response;
    response["success"] = true;
    response["upload"] = static_cast<qint64>(m_service->getUploadBytes());
    response["download"] = static_cast<qint64>(m_service->getDownloadBytes());
    response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    return response;
}

QByteArray SimpleWebServer::getWebInterface() {
    static const QByteArray html(
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>NekoRay CLI Web Interface</title>\n"
        "    <meta charset=\"utf-8\">\n"
        "    <style>\n"
        "        body { font-family: Arial, sans-serif; margin: 20px; }\n"
        "        .status { padding: 10px; border-radius: 5px; margin: 10px 0; }\n"
        "        .running { background-color: #d4edda; }\n"
        "        .stopped { background-color: #f8d7da; }\n"
        "        button { padding: 10px 20px; margin: 5px; }\n"
        "        .container { max-width: 800px; margin: 0 auto; }\n"
        "        .section { border: 1px solid #ddd; padding: 15px; margin: 10px 0; border-radius: 5px; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"container\">\n"
        "        <h1>NekoRay CLI Web Interface</h1>\n"
        "        \n"
        "        <div class=\"section\">\n"
        "            <h2>Service Status</h2>\n"
        "            <div id=\"status-display\" class=\"status\">Loading...</div>\n"
        "            <button onclick=\"updateStatus()\">Refresh Status</button>\n"
        "        </div>\n"
        "\n"
        "        <div class=\"section\">\n"
        "            <h2>Proxy Control</h2>\n"
        "            <input type=\"number\" id=\"profile-id\" value=\"1\" min=\"1\" placeholder=\"Profile ID\">\n"
        "            <label><input type=\"checkbox\" id=\"tun-mode\"> Enable TUN Mode</label><br>\n"
        "            <button onclick=\"startProxy()\">Start Proxy</button>\n"
        "            <button onclick=\"stopProxy()\">Stop Proxy</button>\n"
        "            <button onclick=\"restartProxy()\">Restart Proxy</button>\n"
        "        </div>\n"
        "\n"
        "        <div class=\"section\">\n"
        "            <h2>Traffic Statistics</h2>\n"
        "            <div id=\"traffic-display\">Loading...</div>\n"
        "            <button onclick=\"updateTraffic()\">Update Traffic</button>\n"
        "        </div>\n"
        "    </div>\n"
        "\n"
        "    <script>\n"
        "        function updateStatus() {\n"
        "            fetch('/api/status')\n"
        "                .then(response => response.json())\n"
        "                .then(data => {\n"
        "                    const status = document.getElementById('status-display');\n"
        "                    if (data.success) {\n"
        "                        status.innerHTML = \n"
        "                            '<strong>Status:</strong> ' + data.status + '<br>' +\n"
        "                            '<strong>Profile ID:</strong> ' + data.current_profile + '<br>' +\n"
        "                            '<strong>SOCKS:</strong> ' + data.socks_address + ':' + data.socks_port + '<br>' +\n"
        "                            '<strong>HTTP:</strong> ' + data.http_address + ':' + data.http_port + '<br>' +\n"
        "                            '<strong>TUN Mode:</strong> ' + (data.tun_running ? 'Running' : 'Stopped');\n"
        "                        status.className = 'status ' + (data.status === 'Running' ? 'running' : 'stopped');\n"
        "                    } else {\n"
        "                        status.innerHTML = 'Error loading status';\n"
        "                        status.className = 'status stopped';\n"
        "                    }\n"
        "                });\n"
        "        }\n"
        "\n"
        "        function startProxy() {\n"
        "            const profileId = document.getElementById('profile-id').value;\n"
        "            const tunMode = document.getElementById('tun-mode').checked;\n"
        "            \n"
        "            fetch('/api/start', {\n"
        "                method: 'POST',\n"
        "                headers: {'Content-Type': 'application/json'},\n"
        "                body: JSON.stringify({profile_id: parseInt(profileId), tun_mode: tunMode})\n"
        "            })\n"
        "            .then(response => response.json())\n"
        "            .then(data => {\n"
        "                alert(data.success ? data.message : data.error);\n"
        "                updateStatus();\n"
        "            });\n"
        "        }\n"
        "\n"
        "        function stopProxy() {\n"
        "            fetch('/api/stop', {method: 'POST'})\n"
        "                .then(response => response.json())\n"
        "                .then(data => {\n"
        "                    alert(data.success ? data.message : data.error);\n"
        "                    updateStatus();\n"
        "                });\n"
        "        }\n"
        "\n"
        "        function restartProxy() {\n"
        "            const profileId = document.getElementById('profile-id').value;\n"
        "            const tunMode = document.getElementById('tun-mode').checked;\n"
        "            \n"
        "            fetch('/api/restart', {\n"
        "                method: 'POST',\n"
        "                headers: {'Content-Type': 'application/json'},\n"
        "                body: JSON.stringify({profile_id: parseInt(profileId), tun_mode: tunMode})\n"
        "            })\n"
        "            .then(response => response.json())\n"
        "            .then(data => {\n"
        "                alert(data.success ? data.message : data.error);\n"
        "                updateStatus();\n"
        "            });\n"
        "        }\n"
        "\n"
        "        function updateTraffic() {\n"
        "            fetch('/api/traffic')\n"
        "                .then(response => response.json())\n"
        "                .then(data => {\n"
        "                    if (data.success) {\n"
        "                        document.getElementById('traffic-display').innerHTML = \n"
        "                            '<strong>Upload:</strong> ' + (data.upload / 1024 / 1024).toFixed(2) + ' MB<br>' +\n"
        "                            '<strong>Download:</strong> ' + (data.download / 1024 / 1024).toFixed(2) + ' MB<br>' +\n"
        "                            '<strong>Updated:</strong> ' + new Date(data.timestamp).toLocaleString();\n"
        "                    }\n"
        "                });\n"
        "        }\n"
        "\n"
        "        setInterval(() => {\n"
        "            updateStatus();\n"
        "            updateTraffic();\n"
        "        }, 5000);\n"
        "\n"
        "        updateStatus();\n"
        "        updateTraffic();\n"
        "    </script>\n"
        "</body>\n"
        "</html>\n"
    );
    return html;
}

} // namespace NekoWeb