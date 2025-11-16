#pragma once

#include "AbstractBean_Headless.hpp"

namespace NekoCore_fmt {
    
    class SocksHttpBean : public AbstractBean {
    public:
        enum proxy_type {
            type_HTTP = 0,
            type_Socks5 = 1,
        };

        proxy_type socks_http_type = type_Socks5;
        QString username = "";
        QString password = "";

        explicit SocksHttpBean(proxy_type type = type_Socks5);
        
        QString DisplayType() override;
        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;
        QString ToShareLink() override;
        
        QJsonObject ToJson() const override;
        bool FromJson(const QJsonObject &obj) override;
        
        static std::shared_ptr<SocksHttpBean> FromShareLink(const QString &link);
    };

} // namespace NekoCore_fmt