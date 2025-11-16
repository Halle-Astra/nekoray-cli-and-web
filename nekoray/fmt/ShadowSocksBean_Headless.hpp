#pragma once

#include "AbstractBean_Headless.hpp"

namespace NekoCore_fmt {
    
    class ShadowSocksBean : public AbstractBean {
    public:
        QString method = "chacha20-ietf-poly1305";
        QString password = "";
        QString plugin = "";
        QString plugin_opts = "";
        
        explicit ShadowSocksBean();
        
        QString DisplayType() override;
        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;
        QString ToShareLink() override;
        
        QJsonObject ToJson() const override;
        bool FromJson(const QJsonObject &obj) override;
        
        static std::shared_ptr<ShadowSocksBean> FromShareLink(const QString &link);
    };

} // namespace NekoCore_fmt