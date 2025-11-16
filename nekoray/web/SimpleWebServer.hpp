#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSharedPointer>

#include "../core/NekoService_Fixed.hpp"

namespace NekoWeb {

    class SimpleWebServer : public QObject {
        Q_OBJECT

    public:
        explicit SimpleWebServer(NekoCore::NekoService *service, QObject *parent = nullptr);
        ~SimpleWebServer();

        bool start(const QString &host, int port);
        void stop();
        
        bool isRunning() const;
        int getPort() const { return m_port; }
        QString getHost() const { return m_host; }

    signals:
        void clientConnected(const QString &clientAddress);
        void requestReceived(const QString &method, const QString &path);
        void errorOccurred(const QString &error);

    private slots:
        void onNewConnection();
        void onClientDisconnected();
        void onReadyRead();

    private:
        // HTTP处理
        void handleHttpRequest(QTcpSocket *socket, const QByteArray &request);
        void sendResponse(QTcpSocket *socket, int statusCode, const QString &contentType, const QByteArray &body);
        void sendJsonResponse(QTcpSocket *socket, const QJsonObject &data, int statusCode = 200);
        void sendErrorResponse(QTcpSocket *socket, const QString &error, int statusCode = 400);
        void sendCorsHeaders(QTcpSocket *socket);
        
        // API端点
        QJsonObject handleApiStatus();
        QJsonObject handleApiStart(const QJsonObject &params);
        QJsonObject handleApiStop();
        QJsonObject handleApiRestart(const QJsonObject &params);
        QJsonObject handleApiProfiles();
        QJsonObject handleApiConfig();
        QJsonObject handleApiTraffic();
        
        // Web界面
        QByteArray getWebInterface();
        
        QTcpServer *m_server;
        NekoCore::NekoService *m_service;
        QString m_host;
        int m_port;
        
        // 连接的客户端
        QList<QTcpSocket*> m_clients;
    };

} // namespace NekoWeb