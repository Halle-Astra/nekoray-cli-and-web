#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QSharedPointer>

#include "../core/NekoService.hpp"

namespace NekoWeb {

    class WebApiServer : public QObject {
        Q_OBJECT

    public:
        explicit WebApiServer(QObject *parent = nullptr);
        ~WebApiServer();

        bool start(int port, NekoCore::NekoService *service);
        void stop();
        
        bool isRunning() const;
        int getPort() const { return m_port; }

    signals:
        void clientConnected(const QString &clientAddress);
        void requestReceived(const QString &method, const QString &path);
        void errorOccurred(const QString &error);

    private slots:
        void onServiceStatusChanged(NekoCore::ServiceStatus status);
        void onServiceError(const QString &error);
        void onServiceLogMessage(const QString &level, const QString &message);

    private:
        // API Endpoints
        QHttpServerResponse handleGetStatus(const QHttpServerRequest &request);
        QHttpServerResponse handlePostStart(const QHttpServerRequest &request);
        QHttpServerResponse handlePostStop(const QHttpServerRequest &request);
        QHttpServerResponse handlePostRestart(const QHttpServerRequest &request);
        QHttpServerResponse handleGetProfiles(const QHttpServerRequest &request);
        QHttpServerResponse handleGetConfig(const QHttpServerRequest &request);
        QHttpServerResponse handlePostConfig(const QHttpServerRequest &request);
        QHttpServerResponse handleGetTraffic(const QHttpServerRequest &request);
        QHttpServerResponse handlePostTunStart(const QHttpServerRequest &request);
        QHttpServerResponse handlePostTunStop(const QHttpServerRequest &request);
        QHttpServerResponse handleGetLogs(const QHttpServerRequest &request);
        QHttpServerResponse handlePostImport(const QHttpServerRequest &request);
        QHttpServerResponse handleGetWebUI(const QHttpServerRequest &request);

        // Helper methods
        QHttpServerResponse jsonResponse(const QJsonObject &data, int statusCode = 200);
        QHttpServerResponse errorResponse(const QString &error, int statusCode = 400);
        QJsonObject parseRequestBody(const QHttpServerRequest &request);
        bool validateJsonRequest(const QHttpServerRequest &request);
        
        // CORS headers
        QHttpServerResponse addCorsHeaders(QHttpServerResponse response);
        QHttpServerResponse handleOptionsRequest(const QHttpServerRequest &request);

        QHttpServer *m_httpServer;
        NekoCore::NekoService *m_service;
        int m_port;
        
        // Logs storage for web interface
        QList<QJsonObject> m_logs;
        int m_maxLogs;

        // Statistics
        QJsonObject m_lastTrafficStats;
    };

} // namespace NekoWeb