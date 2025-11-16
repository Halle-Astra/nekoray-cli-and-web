#include "ShadowSocksBean_Headless.hpp"
#include <QUrl>
#include <QUrlQuery>
#include <QByteArray>

namespace NekoCore_fmt {

    ShadowSocksBean::ShadowSocksBean() : AbstractBean(0) {
        software_core_name = "sing-box";
        serverPort = 8388;
    }

    QString ShadowSocksBean::DisplayType() {
        return "Shadowsocks";
    }

    CoreObjOutboundBuildResult ShadowSocksBean::BuildCoreObjSingBox() {
        QJsonObject outbound = BuildOutboundObj("shadowsocks", GenerateRandomTag());
        
        outbound["server"] = serverAddress;
        outbound["server_port"] = serverPort;
        outbound["method"] = method;
        outbound["password"] = password;
        
        // Plugin 支持
        if (!plugin.isEmpty()) {
            QJsonObject pluginObj;
            pluginObj["type"] = plugin;
            
            if (!plugin_opts.isEmpty()) {
                pluginObj["options"] = plugin_opts;
            }
            
            outbound["plugin"] = pluginObj;
        }
        
        return CoreObjOutboundBuildResult(outbound);
    }

    QString ShadowSocksBean::ToShareLink() {
        // ss://method:password@server:port#name
        QString auth = QString("%1:%2").arg(method, password);
        QString authEncoded = auth.toUtf8().toBase64();
        
        QString link = QString("ss://%1@%2:%3")
                          .arg(authEncoded)
                          .arg(serverAddress)
                          .arg(serverPort);
        
        if (!name.isEmpty()) {
            link += "#" + QUrl::toPercentEncoding(name);
        }
        
        return link;
    }

    QJsonObject ShadowSocksBean::ToJson() const {
        QJsonObject obj = AbstractBean::ToJson();
        obj["method"] = method;
        obj["password"] = password;
        obj["plugin"] = plugin;
        obj["plugin_opts"] = plugin_opts;
        return obj;
    }

    bool ShadowSocksBean::FromJson(const QJsonObject &obj) {
        if (!AbstractBean::FromJson(obj)) {
            return false;
        }
        
        method = obj["method"].toString();
        password = obj["password"].toString();
        plugin = obj["plugin"].toString();
        plugin_opts = obj["plugin_opts"].toString();
        
        return IsValid() && !password.isEmpty();
    }

    std::shared_ptr<ShadowSocksBean> ShadowSocksBean::FromShareLink(const QString &link) {
        if (!link.startsWith("ss://")) {
            return nullptr;
        }
        
        auto bean = std::make_shared<ShadowSocksBean>();
        
        QString content = link.mid(5); // 移除 "ss://"
        
        // 解析 # 后面的名称
        int nameIndex = content.indexOf('#');
        if (nameIndex != -1) {
            bean->name = QUrl::fromPercentEncoding(content.mid(nameIndex + 1).toUtf8());
            content = content.left(nameIndex);
        }
        
        // 解析 @ 分隔符
        int atIndex = content.indexOf('@');
        if (atIndex == -1) return nullptr;
        
        QString authPart = content.left(atIndex);
        QString serverPart = content.mid(atIndex + 1);
        
        // 解析服务器和端口
        int colonIndex = serverPart.lastIndexOf(':');
        if (colonIndex == -1) return nullptr;
        
        bean->serverAddress = serverPart.left(colonIndex);
        bean->serverPort = serverPart.mid(colonIndex + 1).toInt();
        
        // 解析认证信息
        QByteArray authDecoded = QByteArray::fromBase64(authPart.toUtf8());
        QString authStr = QString::fromUtf8(authDecoded);
        
        int methodColonIndex = authStr.indexOf(':');
        if (methodColonIndex == -1) return nullptr;
        
        bean->method = authStr.left(methodColonIndex);
        bean->password = authStr.mid(methodColonIndex + 1);
        
        return bean->IsValid() ? bean : nullptr;
    }

} // namespace NekoCore_fmt