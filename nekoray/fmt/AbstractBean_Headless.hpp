#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QStringList>
#include <functional>

// 移除NekoGui依赖的JsonStore，自己实现简化版本
class JsonStore {
public:
    JsonStore(const QString &fileName = "") : fileName(fileName) {}
    virtual ~JsonStore() = default;
    
    virtual QJsonObject ToJson() const = 0;
    virtual bool FromJson(const QJsonObject &obj) = 0;
    
    // 基础数据保存加载
    bool Save();
    bool Load();
    
protected:
    QString fileName;
    QJsonObject data;
};

namespace NekoCore_fmt {  // 替代 NekoGui_fmt
    
    struct CoreObjOutboundBuildResult {
    public:
        QJsonObject outbound;
        QString error;
        
        CoreObjOutboundBuildResult() = default;
        CoreObjOutboundBuildResult(const QJsonObject &obj, const QString &err = "") 
            : outbound(obj), error(err) {}
    };

    struct ExternalBuildResult {
    public:
        QString program;
        QStringList env;
        QStringList arguments;
        QString tag;
        QString error;
        QString config_export;
        
        ExternalBuildResult() = default;
        bool IsValid() const { return error.isEmpty() && !program.isEmpty(); }
    };

    class AbstractBean : public JsonStore {
    public:
        int version;
        
        QString name = "";
        QString serverAddress = "127.0.0.1";
        int serverPort = 1080;
        
        QString custom_config = "";
        QString custom_outbound = "";
        
        // 协议类型标识
        QString software_core_name = "sing-box";
        
        explicit AbstractBean(int version);
        virtual ~AbstractBean() = default;
        
        // 基础方法
        QString ToNekorayShareLink(const QString &type);
        void ResolveDomainToIP(const std::function<void()> &onFinished);
        
        // 显示方法
        virtual QString DisplayAddress();
        virtual QString DisplayName();
        virtual QString DisplayCoreType() { return software_core_name; }
        virtual QString DisplayType() { return "Unknown"; }
        virtual QString DisplayTypeAndName();
        
        // 验证方法
        virtual bool IsValid() const;
        
        // 构建方法
        virtual int NeedExternal(bool isFirstProfile) { return 0; }
        virtual CoreObjOutboundBuildResult BuildCoreObjSingBox() { return {}; }
        virtual ExternalBuildResult BuildExternal(int mapping_port, int socks_port, int external_stat) { return {}; }
        virtual QString ToShareLink() { return {}; }
        
        // JSON序列化
        QJsonObject ToJson() const override;
        bool FromJson(const QJsonObject &obj) override;
        
    protected:
        // 工具方法
        QJsonObject BuildInboundObj(const QString &type, int port, const QString &listen = "127.0.0.1");
        QJsonObject BuildOutboundObj(const QString &type, const QString &tag = "proxy");
        QString GenerateRandomTag() const;
    };

    // 各种协议Bean的前向声明
    class SocksHttpBean;
    class ShadowSocksBean;
    class ChainBean;
    class VMessBean;
    class TrojanVLESSBean;
    class NaiveBean;
    class QUICBean;
    class CustomBean;

} // namespace NekoCore_fmt