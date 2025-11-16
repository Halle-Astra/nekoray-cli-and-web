#include "SocksHttpBean_Headless.hpp"
#include <QUrl>
#include <QUrlQuery>

namespace NekoCore_fmt {

    SocksHttpBean::SocksHttpBean(proxy_type type) : AbstractBean(0) {
        socks_http_type = type;
        software_core_name = "sing-box";
    }

    QString SocksHttpBean::DisplayType() {
        return socks_http_type == type_HTTP ? "HTTP" : "SOCKS5";
    }

    CoreObjOutboundBuildResult SocksHttpBean::BuildCoreObjSingBox() {
        QJsonObject outbound = BuildOutboundObj(
            socks_http_type == type_HTTP ? "http" : "socks", 
            GenerateRandomTag()
        );
        
        // 服务器配置
        outbound["server"] = serverAddress;
        outbound["server_port"] = serverPort;
        
        // 认证信息
        if (!username.isEmpty()) {
            outbound["username"] = username;
            if (!password.isEmpty()) {
                outbound["password"] = password;
            }
        }
        
        // SOCKS特定配置
        if (socks_http_type == type_Socks5) {
            outbound["version"] = "5";
        }
        
        return CoreObjOutboundBuildResult(outbound);
    }

    QString SocksHttpBean::ToShareLink() {
        QUrl url;
        
        if (socks_http_type == type_HTTP) {
            url.setScheme("http");
        } else {
            url.setScheme("socks5");
        }
        
        url.setHost(serverAddress);
        url.setPort(serverPort);
        
        if (!username.isEmpty()) {
            url.setUserName(username);
            if (!password.isEmpty()) {
                url.setPassword(password);
            }
        }
        
        QUrlQuery query;
        if (!name.isEmpty()) {
            query.addQueryItem("name", name);
        }
        
        url.setQuery(query);
        return url.toString();
    }

    QJsonObject SocksHttpBean::ToJson() const {
        QJsonObject obj = AbstractBean::ToJson();
        obj["socks_http_type"] = static_cast<int>(socks_http_type);
        obj["username"] = username;
        obj["password"] = password;
        return obj;
    }

    bool SocksHttpBean::FromJson(const QJsonObject &obj) {
        if (!AbstractBean::FromJson(obj)) {
            return false;
        }
        
        socks_http_type = static_cast<proxy_type>(obj["socks_http_type"].toInt());
        username = obj["username"].toString();
        password = obj["password"].toString();
        
        return IsValid();
    }

    std::shared_ptr<SocksHttpBean> SocksHttpBean::FromShareLink(const QString &link) {
        QUrl url(link);
        auto bean = std::make_shared<SocksHttpBean>();
        
        if (url.scheme() == "http") {
            bean->socks_http_type = type_HTTP;
        } else if (url.scheme() == "socks5") {
            bean->socks_http_type = type_Socks5;
        } else {
            return nullptr;
        }
        
        bean->serverAddress = url.host();
        bean->serverPort = url.port();
        bean->username = url.userName();
        bean->password = url.password();
        
        QUrlQuery query(url.query());
        if (query.hasQueryItem("name")) {
            bean->name = query.queryItemValue("name");
        }
        
        return bean->IsValid() ? bean : nullptr;
    }

} // namespace NekoCore_fmt