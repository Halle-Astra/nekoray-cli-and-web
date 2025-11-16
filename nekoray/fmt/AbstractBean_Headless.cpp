#include "AbstractBean_Headless.hpp"

#include <QJsonDocument>
#include <QFile>
#include <QDebug>
#include <QRandomGenerator>
#include <QDir>
#include <QStandardPaths>

// JsonStore 实现
bool JsonStore::Save() {
    if (fileName.isEmpty()) return false;
    
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/nekoray";
    QDir dir(configDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QString fullPath = configDir + "/" + fileName;
    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to save to" << fullPath;
        return false;
    }
    
    QJsonDocument doc(ToJson());
    file.write(doc.toJson());
    return true;
}

bool JsonStore::Load() {
    if (fileName.isEmpty()) return false;
    
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/nekoray";
    QString fullPath = configDir + "/" + fileName;
    
    QFile file(fullPath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << error.errorString();
        return false;
    }
    
    return FromJson(doc.object());
}

namespace NekoCore_fmt {

    AbstractBean::AbstractBean(int version) : version(version) {
        // 默认构造
    }

    QString AbstractBean::ToNekorayShareLink(const QString &type) {
        // 简化的分享链接生成
        QJsonObject obj = ToJson();
        QJsonDocument doc(obj);
        QString encoded = QString::fromUtf8(doc.toJson().toBase64());
        return QString("nekoray://%1?config=%2").arg(type, encoded);
    }

    void AbstractBean::ResolveDomainToIP(const std::function<void()> &onFinished) {
        // 简化实现，直接回调
        if (onFinished) {
            onFinished();
        }
    }

    QString AbstractBean::DisplayAddress() {
        return QString("%1:%2").arg(serverAddress).arg(serverPort);
    }

    QString AbstractBean::DisplayName() {
        if (!name.isEmpty()) {
            return name;
        }
        return DisplayAddress();
    }

    QString AbstractBean::DisplayTypeAndName() {
        return QString("[%1] %2").arg(DisplayType(), DisplayName());
    }

    bool AbstractBean::IsValid() const {
        return !serverAddress.isEmpty() && serverPort > 0 && serverPort <= 65535;
    }

    QJsonObject AbstractBean::ToJson() const {
        QJsonObject obj;
        obj["version"] = version;
        obj["name"] = name;
        obj["serverAddress"] = serverAddress;
        obj["serverPort"] = serverPort;
        obj["custom_config"] = custom_config;
        obj["custom_outbound"] = custom_outbound;
        obj["software_core_name"] = software_core_name;
        return obj;
    }

    bool AbstractBean::FromJson(const QJsonObject &obj) {
        version = obj["version"].toInt();
        name = obj["name"].toString();
        serverAddress = obj["serverAddress"].toString();
        serverPort = obj["serverPort"].toInt();
        custom_config = obj["custom_config"].toString();
        custom_outbound = obj["custom_outbound"].toString();
        software_core_name = obj["software_core_name"].toString();
        
        return IsValid();
    }

    QJsonObject AbstractBean::BuildInboundObj(const QString &type, int port, const QString &listen) {
        QJsonObject inbound;
        inbound["type"] = type;
        inbound["tag"] = QString("inbound-%1").arg(type);
        inbound["listen"] = listen;
        inbound["listen_port"] = port;
        
        return inbound;
    }

    QJsonObject AbstractBean::BuildOutboundObj(const QString &type, const QString &tag) {
        QJsonObject outbound;
        outbound["type"] = type;
        outbound["tag"] = tag;
        
        return outbound;
    }

    QString AbstractBean::GenerateRandomTag() const {
        const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        QString tag;
        for (int i = 0; i < 8; i++) {
            tag.append(chars.at(QRandomGenerator::global()->bounded(chars.length())));
        }
        return tag;
    }

} // namespace NekoCore_fmt