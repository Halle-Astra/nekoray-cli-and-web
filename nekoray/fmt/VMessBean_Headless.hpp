#pragma once

#include "AbstractBean_Headless.hpp"

namespace NekoCore_fmt {
    
    class VMessBean : public AbstractBean {
    public:
        QString uuid = "";
        QString security = "auto";
        int aid = 0;
        
        // 传输配置
        QString network = "tcp";
        QString path = "/";
        QString host = "";
        QString tls = "";
        
        explicit VMessBean();
        
        QString DisplayType() override;
        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;
        QString ToShareLink() override;
        
        QJsonObject ToJson() const override;
        bool FromJson(const QJsonObject &obj) override;
        
        static std::shared_ptr<VMessBean> FromShareLink(const QString &link);
    };

} // namespace NekoCore_fmt