#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QList>
#include <QMap>
#include <QDateTime>
#include <QSharedPointer>
#include <QMutex>
#include <memory>

// 前向声明
namespace NekoCore_fmt {
    class AbstractBean;
    class SocksHttpBean;
    class ShadowSocksBean;
    class ChainBean;
    class VMessBean;
    class TrojanVLESSBean;
    class NaiveBean;
    class QUICBean;
    class CustomBean;
}

// 基础配置项类型
enum itemType {
    stringType,
    integerType,
    booleanType,
    integerList
};

// JSON存储基类
class JsonStore {
public:
    JsonStore(const QString &fileName) : fileName(fileName) {}
    virtual ~JsonStore() = default;
    
    virtual void Save();
    virtual void Load();
    
protected:
    QString fileName;
    QJsonObject data;
    
    struct configItem {
        QString name;
        void *ptr;
        itemType type;
        
        configItem(const QString &n, void *p, itemType t) : name(n), ptr(p), type(t) {}
    };
    
    QList<configItem*> items;
    
    void _add(configItem *item) { items.append(item); }
};

// Group类定义
class Group {
public:
    int id = -1;
    QString name;
    qint64 created_at = 0;
    qint64 updated_at = 0;
    QString archive_id;
    QString url;
    
    // 简化的属性
    bool manually_column_width = false;
    int sub_last_update = 0;
    
    Group() = default;
    ~Group() = default;
    
    QJsonObject ToJson() const;
    bool FromJson(const QJsonObject &obj);
};

// ProxyEntity前向声明
class ProxyEntity {
public:
    int id = -1;
    int gid = 0;
    QString name;
    QString type;
    int latency = 0;
    qint64 created_at = 0;
    qint64 updated_at = 0;
    
    std::shared_ptr<NekoCore_fmt::AbstractBean> bean;
    
    ProxyEntity() = default;
    ProxyEntity(NekoCore_fmt::AbstractBean *bean, const QString &type_);
    ~ProxyEntity() = default;
    
    // 显示方法
    QString DisplayLatencyColor() const;
    QString DisplayAddress() const;
    QString DisplayName() const;
    QString DisplayType() const;
    bool IsValid() const;
    
    // 数据操作
    void UpdateBean(NekoCore_fmt::AbstractBean *newBean);
    QJsonObject ToJson() const;
    bool FromJson(const QJsonObject &obj);
};

namespace NekoCore {
    class ProfileManager : private JsonStore {
    public:
        // JsonStore继承
        QList<int> groupsTabOrder;
        
        // Manager数据
        QMap<int, std::shared_ptr<ProxyEntity>> profiles;
        QList<Group*> groups;  // 简化为普通指针列表
        
        ProfileManager();
        ~ProfileManager();
        
        // 初始化
        void InitializeDefaultGroup();
        
        // 基础管理方法
        NekoCore_fmt::AbstractBean *GetBean(int profileId);
        QString GetProfileType(int profileId);
        
        // Group管理
        Group *NewGroup();
        void AddGroup(Group *group);
        void DeleteGroup(int id);
        QList<Group*> GetGroups();
        
        // Profile管理
        std::shared_ptr<ProxyEntity> GetProfile(int id);
        bool AddProfile(const std::shared_ptr<ProxyEntity> &ent, int gid = -1);
        void DeleteProfile(int id);
        
        // 工具方法
        int GetNextId();
        
    private:
        QMutex mutex;
    };

    extern ProfileManager *profileManager;
} // namespace NekoCore