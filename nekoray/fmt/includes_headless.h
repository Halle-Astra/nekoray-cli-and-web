#pragma once

#include "SocksHttpBean_Headless.hpp"
#include "ShadowSocksBean_Headless.hpp"
#include "VMessBean_Headless.hpp"

// 简化版本的其他Bean类
namespace NekoCore_fmt {
    
    // TrojanVLESSBean 简化实现
    class TrojanVLESSBean : public AbstractBean {
    public:
        enum proxy_type { proxy_Trojan = 0, proxy_VLESS = 1 };
        
        proxy_type proxy_type_value = proxy_Trojan;
        QString password = "";
        QString uuid = "";
        QString flow = "";
        QString network = "tcp";
        QString path = "/";
        QString host = "";
        QString tls = "";
        
        explicit TrojanVLESSBean(proxy_type type = proxy_Trojan) : AbstractBean(0), proxy_type_value(type) {
            software_core_name = "sing-box";
        }
        
        QString DisplayType() override {
            return proxy_type_value == proxy_Trojan ? "Trojan" : "VLESS";
        }
        
        CoreObjOutboundBuildResult BuildCoreObjSingBox() override {
            QJsonObject outbound = BuildOutboundObj(
                proxy_type_value == proxy_Trojan ? "trojan" : "vless", 
                GenerateRandomTag()
            );
            
            outbound["server"] = serverAddress;
            outbound["server_port"] = serverPort;
            
            if (proxy_type_value == proxy_Trojan) {
                outbound["password"] = password;
            } else {
                outbound["uuid"] = uuid;
                if (!flow.isEmpty()) {
                    outbound["flow"] = flow;
                }
            }
            
            return CoreObjOutboundBuildResult(outbound);
        }
        
        QJsonObject ToJson() const override {
            QJsonObject obj = AbstractBean::ToJson();
            obj["proxy_type_value"] = static_cast<int>(proxy_type_value);
            obj["password"] = password;
            obj["uuid"] = uuid;
            obj["flow"] = flow;
            obj["network"] = network;
            obj["path"] = path;
            obj["host"] = host;
            obj["tls"] = tls;
            return obj;
        }
        
        bool FromJson(const QJsonObject &obj) override {
            if (!AbstractBean::FromJson(obj)) return false;
            proxy_type_value = static_cast<proxy_type>(obj["proxy_type_value"].toInt());
            password = obj["password"].toString();
            uuid = obj["uuid"].toString();
            flow = obj["flow"].toString();
            network = obj["network"].toString();
            path = obj["path"].toString();
            host = obj["host"].toString();
            tls = obj["tls"].toString();
            return IsValid();
        }
    };
    
    // NaiveBean 简化实现
    class NaiveBean : public AbstractBean {
    public:
        QString username = "";
        QString password = "";
        
        explicit NaiveBean() : AbstractBean(0) {
            software_core_name = "naive";
        }
        
        QString DisplayType() override { return "Naive"; }
        
        QJsonObject ToJson() const override {
            QJsonObject obj = AbstractBean::ToJson();
            obj["username"] = username;
            obj["password"] = password;
            return obj;
        }
        
        bool FromJson(const QJsonObject &obj) override {
            if (!AbstractBean::FromJson(obj)) return false;
            username = obj["username"].toString();
            password = obj["password"].toString();
            return IsValid();
        }
    };
    
    // QUICBean 简化实现
    class QUICBean : public AbstractBean {
    public:
        enum proxy_type { proxy_Hysteria2 = 0, proxy_TUIC = 1 };
        
        proxy_type proxy_type_value = proxy_Hysteria2;
        QString password = "";
        QString uuid = "";
        
        explicit QUICBean(proxy_type type = proxy_Hysteria2) : AbstractBean(0), proxy_type_value(type) {
            software_core_name = "sing-box";
        }
        
        QString DisplayType() override {
            return proxy_type_value == proxy_Hysteria2 ? "Hysteria2" : "TUIC";
        }
        
        QJsonObject ToJson() const override {
            QJsonObject obj = AbstractBean::ToJson();
            obj["proxy_type_value"] = static_cast<int>(proxy_type_value);
            obj["password"] = password;
            obj["uuid"] = uuid;
            return obj;
        }
        
        bool FromJson(const QJsonObject &obj) override {
            if (!AbstractBean::FromJson(obj)) return false;
            proxy_type_value = static_cast<proxy_type>(obj["proxy_type_value"].toInt());
            password = obj["password"].toString();
            uuid = obj["uuid"].toString();
            return IsValid();
        }
    };
    
    // CustomBean 简化实现
    class CustomBean : public AbstractBean {
    public:
        QString core_type = "sing-box";
        QString config_simple = "";
        
        explicit CustomBean() : AbstractBean(0) {
            software_core_name = "sing-box";
        }
        
        QString DisplayType() override { return "Custom"; }
        
        QJsonObject ToJson() const override {
            QJsonObject obj = AbstractBean::ToJson();
            obj["core_type"] = core_type;
            obj["config_simple"] = config_simple;
            return obj;
        }
        
        bool FromJson(const QJsonObject &obj) override {
            if (!AbstractBean::FromJson(obj)) return false;
            core_type = obj["core_type"].toString();
            config_simple = obj["config_simple"].toString();
            return true; // Custom bean always valid
        }
    };
    
    // ChainBean 简化实现
    class ChainBean : public AbstractBean {
    public:
        QStringList chain_list;
        
        explicit ChainBean() : AbstractBean(0) {
            software_core_name = "sing-box";
        }
        
        QString DisplayType() override { return "Chain"; }
        
        QJsonObject ToJson() const override {
            QJsonObject obj = AbstractBean::ToJson();
            QJsonArray chainArray;
            for (const QString &item : chain_list) {
                chainArray.append(item);
            }
            obj["chain_list"] = chainArray;
            return obj;
        }
        
        bool FromJson(const QJsonObject &obj) override {
            if (!AbstractBean::FromJson(obj)) return false;
            chain_list.clear();
            QJsonArray chainArray = obj["chain_list"].toArray();
            for (const auto &item : chainArray) {
                chain_list.append(item.toString());
            }
            return true;
        }
    };

} // namespace NekoCore_fmt