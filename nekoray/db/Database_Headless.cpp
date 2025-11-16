#include "Database_Headless.hpp"
#include "../fmt/includes_headless.h"

#include <QFile>
#include <QDir>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

namespace NekoCore {  // 替代 NekoGui

    ProfileManager *profileManager = new ProfileManager();

    ProfileManager::ProfileManager() : JsonStore("groups/pm.json") {
        _add(new configItem("groups", &groupsTabOrder, itemType::integerList));
    }

    ProfileManager::~ProfileManager() {
        for (const auto &group : groups) {
            delete group;
        }
    }

    void ProfileManager::InitializeDefaultGroup() {
        if (groups.empty()) {
            auto defaultGroup = NekoCore::ProfileManager::NewGroup();
            NekoCore::profileManager->AddGroup(defaultGroup);
        }
    }

    NekoCore_fmt::AbstractBean *ProfileManager::GetBean(int profileId) {
        NekoCore_fmt::AbstractBean *bean;
        
        // 根据配置类型创建对应的Bean
        QString type = GetProfileType(profileId);
        
        if (type == "socks") {
            bean = new NekoCore_fmt::SocksHttpBean(NekoCore_fmt::SocksHttpBean::type_Socks5);
        } else if (type == "http") {
            bean = new NekoCore_fmt::SocksHttpBean(NekoCore_fmt::SocksHttpBean::type_HTTP);
        } else if (type == "shadowsocks") {
            bean = new NekoCore_fmt::ShadowSocksBean();
        } else if (type == "chain") {
            bean = new NekoCore_fmt::ChainBean();
        } else if (type == "vmess") {
            bean = new NekoCore_fmt::VMessBean();
        } else if (type == "trojan") {
            bean = new NekoCore_fmt::TrojanVLESSBean(NekoCore_fmt::TrojanVLESSBean::proxy_Trojan);
        } else if (type == "vless") {
            bean = new NekoCore_fmt::TrojanVLESSBean(NekoCore_fmt::TrojanVLESSBean::proxy_VLESS);
        } else if (type == "naive") {
            bean = new NekoCore_fmt::NaiveBean();
        } else if (type == "hysteria2") {
            bean = new NekoCore_fmt::QUICBean(NekoCore_fmt::QUICBean::proxy_Hysteria2);
        } else if (type == "tuic") {
            bean = new NekoCore_fmt::QUICBean(NekoCore_fmt::QUICBean::proxy_TUIC);
        } else if (type == "custom") {
            bean = new NekoCore_fmt::CustomBean();
        } else {
            bean = new NekoCore_fmt::AbstractBean(-114514);
        }

        return bean;
    }

    QString ProfileManager::GetProfileType(int profileId) {
        // 从配置文件中读取profile类型，默认返回socks
        // 实际实现需要根据数据库查询
        return "socks";
    }

    Group *ProfileManager::NewGroup() {
        auto group = new Group();
        group->id = GetNextId();
        group->name = QString("Group %1").arg(group->id);
        group->created_at = QDateTime::currentMSecsSinceEpoch();
        group->updated_at = group->created_at;
        return group;
    }

    void ProfileManager::AddGroup(Group *group) {
        groups.append(group);
        Save();
    }

    int ProfileManager::GetNextId() {
        int maxId = 0;
        for (const auto &group : groups) {
            if (group->id > maxId) {
                maxId = group->id;
            }
        }
        return maxId + 1;
    }

    void ProfileManager::DeleteGroup(int id) {
        for (int i = 0; i < groups.size(); i++) {
            if (groups[i]->id == id) {
                delete groups[i];
                groups.removeAt(i);
                Save();
                break;
            }
        }
    }

    QList<Group*> ProfileManager::GetGroups() {
        return groups;
    }

} // namespace NekoCore

// ProxyEntity 实现
ProxyEntity::ProxyEntity(NekoCore_fmt::AbstractBean *bean, const QString &type_) {
    if (bean != nullptr) {
        this->bean = std::shared_ptr<NekoCore_fmt::AbstractBean>(bean);
    }
    this->type = type_;
    this->id = -1;
    this->gid = 0;
    this->latency = 0;
    this->created_at = QDateTime::currentMSecsSinceEpoch();
    this->updated_at = this->created_at;
}

QString ProxyEntity::DisplayLatencyColor() const {
    // 替代原来的QColor，返回颜色字符串
    if (latency < 0) return "#ff0000";  // 红色 - 错误
    if (latency == 0) return "#808080"; // 灰色 - 未测试
    if (latency < 200) return "#00ff00"; // 绿色 - 优秀
    if (latency < 500) return "#ffff00"; // 黄色 - 良好
    return "#ffa500"; // 橙色 - 较慢
}

QString ProxyEntity::DisplayAddress() const {
    if (!bean) return "Invalid";
    return bean->DisplayAddress();
}

QString ProxyEntity::DisplayName() const {
    if (name.isEmpty()) {
        return DisplayAddress();
    }
    return name;
}

QString ProxyEntity::DisplayType() const {
    if (!bean) return "Unknown";
    return bean->DisplayType();
}

bool ProxyEntity::IsValid() const {
    return bean != nullptr && bean->IsValid();
}

void ProxyEntity::UpdateBean(NekoCore_fmt::AbstractBean *newBean) {
    if (newBean != nullptr) {
        this->bean = std::shared_ptr<NekoCore_fmt::AbstractBean>(newBean);
        this->updated_at = QDateTime::currentMSecsSinceEpoch();
    }
}

QJsonObject ProxyEntity::ToJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["gid"] = gid;
    obj["type"] = type;
    obj["name"] = name;
    obj["latency"] = latency;
    obj["created_at"] = created_at;
    obj["updated_at"] = updated_at;
    
    if (bean) {
        obj["bean"] = bean->ToJson();
    }
    
    return obj;
}

bool ProxyEntity::FromJson(const QJsonObject &obj) {
    id = obj["id"].toInt();
    gid = obj["gid"].toInt();
    type = obj["type"].toString();
    name = obj["name"].toString();
    latency = obj["latency"].toInt();
    created_at = obj["created_at"].toVariant().toLongLong();
    updated_at = obj["updated_at"].toVariant().toLongLong();
    
    // 根据类型创建相应的Bean
    if (obj.contains("bean")) {
        QJsonObject beanObj = obj["bean"].toObject();
        bean = std::shared_ptr<NekoCore_fmt::AbstractBean>(
            NekoCore::profileManager->GetBean(id)
        );
        if (bean) {
            bean->FromJson(beanObj);
        }
    }
    
    return IsValid();
}