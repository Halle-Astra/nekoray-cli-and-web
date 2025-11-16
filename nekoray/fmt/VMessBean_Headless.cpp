#include "VMessBean_Headless.hpp"
#include <QJsonDocument>
#include <QUrl>

namespace NekoCore_fmt {

    VMessBean::VMessBean() : AbstractBean(0) {
        software_core_name = "sing-box";
        serverPort = 443;
    }

    QString VMessBean::DisplayType() {
        return "VMess";
    }

    CoreObjOutboundBuildResult VMessBean::BuildCoreObjSingBox() {
        QJsonObject outbound = BuildOutboundObj("vmess", GenerateRandomTag());
        
        outbound["server"] = serverAddress;
        outbound["server_port"] = serverPort;
        outbound["uuid"] = uuid;
        outbound["security"] = security;
        
        if (aid > 0) {
            outbound["alter_id"] = aid;
        }
        
        // 传输配置
        QJsonObject transport;
        transport["type"] = network;
        
        if (network == "ws") {
            transport["path"] = path;
            if (!host.isEmpty()) {
                QJsonObject headers;
                headers["Host"] = host;
                transport["headers"] = headers;
            }
        } else if (network == "grpc") {
            transport["service_name"] = path;
        }
        
        outbound["transport"] = transport;
        
        // TLS配置
        if (tls == "tls") {
            QJsonObject tlsConfig;
            tlsConfig["enabled"] = true;
            if (!host.isEmpty()) {
                tlsConfig["server_name"] = host;
            }
            outbound["tls"] = tlsConfig;
        }
        
        return CoreObjOutboundBuildResult(outbound);
    }

    QString VMessBean::ToShareLink() {
        QJsonObject config;
        config["v"] = "2";
        config["ps"] = name;
        config["add"] = serverAddress;
        config["port"] = QString::number(serverPort);
        config["id"] = uuid;
        config["aid"] = QString::number(aid);
        config["net"] = network;
        config["type"] = "none";
        config["host"] = host;
        config["path"] = path;
        config["tls"] = tls;
        
        QJsonDocument doc(config);
        QString encoded = doc.toJson(QJsonDocument::Compact).toBase64();
        return "vmess://" + encoded;
    }

    QJsonObject VMessBean::ToJson() const {
        QJsonObject obj = AbstractBean::ToJson();
        obj["uuid"] = uuid;
        obj["security"] = security;
        obj["aid"] = aid;
        obj["network"] = network;
        obj["path"] = path;
        obj["host"] = host;
        obj["tls"] = tls;
        return obj;
    }

    bool VMessBean::FromJson(const QJsonObject &obj) {
        if (!AbstractBean::FromJson(obj)) {
            return false;
        }
        
        uuid = obj["uuid"].toString();
        security = obj["security"].toString();
        aid = obj["aid"].toInt();
        network = obj["network"].toString();
        path = obj["path"].toString();
        host = obj["host"].toString();
        tls = obj["tls"].toString();
        
        return IsValid() && !uuid.isEmpty();
    }

    std::shared_ptr<VMessBean> VMessBean::FromShareLink(const QString &link) {
        if (!link.startsWith("vmess://")) {
            return nullptr;
        }
        
        auto bean = std::make_shared<VMessBean>();
        
        QString encoded = link.mid(8);
        QByteArray decoded = QByteArray::fromBase64(encoded.toUtf8());
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(decoded, &error);
        if (error.error != QJsonParseError::NoError) {
            return nullptr;
        }
        
        QJsonObject config = doc.object();
        
        bean->name = config["ps"].toString();
        bean->serverAddress = config["add"].toString();
        bean->serverPort = config["port"].toString().toInt();
        bean->uuid = config["id"].toString();
        bean->aid = config["aid"].toString().toInt();
        bean->network = config["net"].toString();
        bean->host = config["host"].toString();
        bean->path = config["path"].toString();
        bean->tls = config["tls"].toString();
        
        return bean->IsValid() ? bean : nullptr;
    }

} // namespace NekoCore_fmt