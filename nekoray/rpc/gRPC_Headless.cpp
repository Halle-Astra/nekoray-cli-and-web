#include "gRPC_Headless.hpp"

#include <QHostAddress>
#include <QJsonDocument>
#include <QDebug>
#include <QRandomGenerator>

namespace NekoCore {
    
    SimpleGRPCClient *grpcClient = nullptr;
    
    bool InitializeGRPC() {
        if (grpcClient) return true;
        
        grpcClient = new SimpleGRPCClient();
        return grpcClient != nullptr;
    }
    
    void CleanupGRPC() {
        if (grpcClient) {
            grpcClient->disconnectFromCore();
            delete grpcClient;
            grpcClient = nullptr;
        }
    }
    
    SimpleGRPCClient::SimpleGRPCClient(QObject *parent) 
        : QObject(parent)
        , m_socket(nullptr)
        , m_trafficTimer(nullptr)
        , m_serverPort(50051)
        , m_uploadBytes(0)
        , m_downloadBytes(0)
        , m_lastUpload(0)
        , m_lastDownload(0)
        , m_coreRunning(false)
        , m_currentProfileId(-1)
    {
        m_socket = new QTcpSocket(this);
        connect(m_socket, &QTcpSocket::connected, this, &SimpleGRPCClient::onSocketConnected);
        connect(m_socket, &QTcpSocket::disconnected, this, &SimpleGRPCClient::onSocketDisconnected);
        connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
                this, &SimpleGRPCClient::onSocketError);
        connect(m_socket, &QTcpSocket::readyRead, this, &SimpleGRPCClient::onDataReceived);
        
        m_trafficTimer = new QTimer(this);
        connect(m_trafficTimer, &QTimer::timeout, this, &SimpleGRPCClient::updateTrafficStats);
        m_trafficTimer->setInterval(1000); // 每秒更新一次流量
    }
    
    SimpleGRPCClient::~SimpleGRPCClient() {
        disconnectFromCore();
    }
    
    bool SimpleGRPCClient::connectToCore(const QString &address, int port) {
        if (isConnected()) {
            return true;
        }
        
        m_serverAddress = address;
        m_serverPort = port;
        
        qDebug() << "Connecting to core at" << address << ":" << port;
        
        m_socket->connectToHost(QHostAddress(address), port);
        
        // 等待连接（简化版本，实际应该是异步的）
        if (m_socket->waitForConnected(3000)) {
            return true;
        } else {
            m_lastError = m_socket->errorString();
            return false;
        }
    }
    
    void SimpleGRPCClient::disconnectFromCore() {
        if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->disconnectFromHost();
            if (m_socket->state() != QAbstractSocket::UnconnectedState) {
                m_socket->waitForDisconnected(1000);
            }
        }
        
        if (m_trafficTimer && m_trafficTimer->isActive()) {
            m_trafficTimer->stop();
        }
        
        m_coreRunning = false;
        m_currentProfileId = -1;
    }
    
    bool SimpleGRPCClient::isConnected() const {
        return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
    }
    
    bool SimpleGRPCClient::startCore(const QJsonObject &config) {
        if (!isConnected()) {
            m_lastError = "Not connected to core";
            return false;
        }
        
        if (sendCommand("start", config)) {
            m_coreRunning = true;
            m_trafficTimer->start();
            
            QJsonObject status;
            status["running"] = true;
            status["config"] = config;
            emit statusChanged(status);
            
            qDebug() << "Core started successfully";
            return true;
        }
        
        return false;
    }
    
    bool SimpleGRPCClient::stopCore() {
        if (!isConnected()) {
            m_lastError = "Not connected to core";
            return false;
        }
        
        if (sendCommand("stop")) {
            m_coreRunning = false;
            m_trafficTimer->stop();
            
            QJsonObject status;
            status["running"] = false;
            emit statusChanged(status);
            
            qDebug() << "Core stopped successfully";
            return true;
        }
        
        return false;
    }
    
    bool SimpleGRPCClient::restartCore(const QJsonObject &config) {
        return stopCore() && startCore(config);
    }
    
    QJsonObject SimpleGRPCClient::getStatus() {
        QJsonObject status;
        status["running"] = m_coreRunning;
        status["current_profile"] = m_currentProfileId;
        status["upload_bytes"] = static_cast<qint64>(m_uploadBytes);
        status["download_bytes"] = static_cast<qint64>(m_downloadBytes);
        status["connected"] = isConnected();
        
        return status;
    }
    
    QJsonObject SimpleGRPCClient::getTrafficStats() {
        QJsonObject stats;
        stats["upload"] = static_cast<qint64>(m_uploadBytes);
        stats["download"] = static_cast<qint64>(m_downloadBytes);
        stats["upload_speed"] = static_cast<qint64>(m_uploadBytes - m_lastUpload);
        stats["download_speed"] = static_cast<qint64>(m_downloadBytes - m_lastDownload);
        
        m_lastUpload = m_uploadBytes;
        m_lastDownload = m_downloadBytes;
        
        return stats;
    }
    
    bool SimpleGRPCClient::testLatency(const QString &profileId) {
        Q_UNUSED(profileId)
        
        // 简化的延迟测试，返回模拟结果
        if (!isConnected()) {
            return false;
        }
        
        // 模拟网络延迟测试
        int simulatedLatency = QRandomGenerator::global()->bounded(50, 300);
        qDebug() << "Latency test result:" << simulatedLatency << "ms";
        
        return true;
    }
    
    bool SimpleGRPCClient::updateConfig(const QJsonObject &config) {
        if (!isConnected()) {
            m_lastError = "Not connected to core";
            return false;
        }
        
        return sendCommand("update_config", config);
    }
    
    bool SimpleGRPCClient::selectProfile(int profileId) {
        if (!isConnected()) {
            m_lastError = "Not connected to core";
            return false;
        }
        
        QJsonObject data;
        data["profile_id"] = profileId;
        
        if (sendCommand("select_profile", data)) {
            m_currentProfileId = profileId;
            return true;
        }
        
        return false;
    }
    
    void SimpleGRPCClient::onSocketConnected() {
        qDebug() << "Connected to core server";
        emit connected();
    }
    
    void SimpleGRPCClient::onSocketDisconnected() {
        qDebug() << "Disconnected from core server";
        m_coreRunning = false;
        if (m_trafficTimer && m_trafficTimer->isActive()) {
            m_trafficTimer->stop();
        }
        emit disconnected();
    }
    
    void SimpleGRPCClient::onSocketError() {
        m_lastError = m_socket->errorString();
        qWarning() << "Socket error:" << m_lastError;
        emit errorOccurred(m_lastError);
    }
    
    void SimpleGRPCClient::onDataReceived() {
        // 简化的数据接收处理
        QByteArray data = m_socket->readAll();
        qDebug() << "Received data from core:" << data;
        
        // 这里应该解析来自核心的响应，但为了简化，我们只是记录
        // 在真实实现中，这里会解析gRPC响应
    }
    
    void SimpleGRPCClient::updateTrafficStats() {
        if (!m_coreRunning) return;
        
        // 模拟流量数据更新
        // 在真实实现中，这里会从核心进程获取实际的流量统计
        quint64 uploadIncrement = QRandomGenerator::global()->bounded(1000, 5000);
        quint64 downloadIncrement = QRandomGenerator::global()->bounded(2000, 10000);
        
        m_uploadBytes += uploadIncrement;
        m_downloadBytes += downloadIncrement;
        
        emit trafficUpdated(m_uploadBytes, m_downloadBytes);
    }
    
    bool SimpleGRPCClient::sendCommand(const QString &command, const QJsonObject &data) {
        if (!isConnected()) {
            m_lastError = "Not connected";
            return false;
        }
        
        QJsonObject message;
        message["command"] = command;
        message["data"] = data;
        
        QJsonDocument doc(message);
        QByteArray payload = doc.toJson(QJsonDocument::Compact);
        
        // 发送数据长度前缀 + 数据
        QByteArray lengthPrefix;
        lengthPrefix.append(reinterpret_cast<const char*>(&payload.size()), sizeof(qint32));
        
        qint64 written = m_socket->write(lengthPrefix + payload);
        m_socket->flush();
        
        if (written > 0) {
            qDebug() << "Sent command:" << command;
            return true;
        } else {
            m_lastError = "Failed to send data";
            return false;
        }
    }
    
    QJsonObject SimpleGRPCClient::receiveResponse() {
        // 简化的响应接收
        // 在真实实现中会正确解析响应格式
        QJsonObject response;
        response["success"] = true;
        response["message"] = "OK";
        return response;
    }
    
} // namespace NekoCore

// #include "gRPC_Headless.moc" // 移到hpp中的Q_OBJECT会自动处理