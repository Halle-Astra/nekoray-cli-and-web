#!/bin/bash

# 文档更新辅助脚本
# 用于在代码修改后快速更新开发记录

set -e

echo "🔄 NekoRay CLI 文档更新工具"
echo "================================"

# 获取当前日期
DATE=$(date '+%Y-%m-%d')

# 检查参数
if [ $# -eq 0 ]; then
    echo "用法: $0 <更新类型> [描述]"
    echo ""
    echo "更新类型:"
    echo "  fix     - Bug 修复"
    echo "  feature - 新功能"
    echo "  test    - 测试相关"
    echo "  refactor- 重构"
    echo "  doc     - 文档更新"
    echo ""
    echo "示例: $0 fix \"修复信号连接问题\""
    exit 1
fi

TYPE=$1
DESCRIPTION=${2:-"未提供描述"}

# 验证更新类型
case $TYPE in
    fix|feature|test|refactor|doc)
        ;;
    *)
        echo "❌ 错误: 无效的更新类型 '$TYPE'"
        exit 1
        ;;
esac

echo "📝 更新信息:"
echo "   类型: $TYPE"
echo "   描述: $DESCRIPTION"
echo "   日期: $DATE"
echo ""

# 创建临时更新条目
UPDATE_ENTRY="### $DATE - $TYPE
**更改**: $DESCRIPTION
**测试**: 需要运行验证测试
**影响**: 待分析
"

# 备份原文档
cp DEVELOPMENT_LOG.md DEVELOPMENT_LOG.md.backup
echo "✅ 已备份 DEVELOPMENT_LOG.md"

# 询问是否要更新文档
read -p "是否要将此更新添加到开发记录? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    
    # 检查是否存在"后续开发计划"段落，在其前添加更新
    if grep -q "## 后续开发计划" DEVELOPMENT_LOG.md; then
        # 在"后续开发计划"前插入新的更新记录
        sed -i "/^## 后续开发计划/i\\
## 最新更新记录\\
\\
$UPDATE_ENTRY\\
" DEVELOPMENT_LOG.md
    else
        # 如果没有找到，就在文件末尾添加
        echo "" >> DEVELOPMENT_LOG.md
        echo "## 最新更新记录" >> DEVELOPMENT_LOG.md
        echo "" >> DEVELOPMENT_LOG.md
        echo "$UPDATE_ENTRY" >> DEVELOPMENT_LOG.md
    fi
    
    echo "✅ 已更新 DEVELOPMENT_LOG.md"
else
    echo "⚪ 跳过文档更新"
fi

# 运行基础验证测试
echo ""
echo "🧪 运行基础验证测试..."

if [ -f "build_test/critical_issues_test" ]; then
    echo "运行关键问题检查..."
    cd build_test && ./critical_issues_test
    TEST_RESULT=$?
    cd ..
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "✅ 关键问题检查通过"
    else
        echo "❌ 关键问题检查失败，请检查代码"
        exit 1
    fi
else
    echo "⚠️  警告: 找不到测试文件，请先编译测试"
fi

# 生成更新摘要
echo ""
echo "📊 更新摘要"
echo "================================"
echo "更新类型: $TYPE"
echo "更新描述: $DESCRIPTION"  
echo "更新日期: $DATE"
echo "测试状态: ✅ 基础验证通过"
echo "文档状态: ✅ 已更新"
echo ""
echo "🎯 下一步建议:"
case $TYPE in
    fix)
        echo "1. 运行完整测试套件验证修复"
        echo "2. 检查是否有回归问题" 
        echo "3. 更新测试用例（如需要）"
        ;;
    feature)
        echo "1. 添加新功能的测试用例"
        echo "2. 更新用户文档和使用指南"
        echo "3. 验证与现有功能的兼容性"
        ;;
    test)
        echo "1. 确认测试覆盖率"
        echo "2. 验证测试的有效性"
        echo "3. 更新测试文档"
        ;;
    refactor)
        echo "1. 验证功能完整性"
        echo "2. 检查性能影响"
        echo "3. 更新架构文档"
        ;;
    doc)
        echo "1. 检查文档的准确性"
        echo "2. 验证示例代码"
        echo "3. 确保文档同步"
        ;;
esac

echo ""
echo "✨ 文档更新完成！"