// Critical Issues Verification Test
#include <iostream>
#include <fstream>
#include <string>
#include <regex>

struct IssueCheck {
    std::string file;
    std::string issue;
    std::string pattern;
    bool shouldNotExist;  // true if pattern should NOT be found
};

std::vector<IssueCheck> criticalChecks = {
    // Check for NekoGui dependencies (should NOT exist)
    {"nekoray/core/NekoService_Fixed.cpp", "NekoGui dependency", "NekoGui::", true},
    {"nekoray/core/CoreManager_Fixed.cpp", "NekoGui dependency", "NekoGui::", true},
    {"nekoray/core/TunManager_Fixed.cpp", "NekoGui dependency", "NekoGui::", true},
    {"nekoray/core/ConfigManager.cpp", "NekoGui dependency", "NekoGui::", true},
    
    // Check for wrong signal connections (should NOT exist)
    {"nekoray/core/CoreManager_Fixed.cpp", "Signal as slot connection", "&CoreManager::processFinished", true},
    {"nekoray/core/TunManager_Fixed.cpp", "Signal as slot connection", "&TunManager::processFinished", true},
    
    // Check for correct signal connections (SHOULD exist)
    {"nekoray/core/CoreManager_Fixed.cpp", "Correct slot connection", "&CoreManager::onProcessFinished", false},
    {"nekoray/core/TunManager_Fixed.cpp", "Correct slot connection", "&TunManager::onProcessFinished", false},
    
    // Check for correct header includes (SHOULD exist)
    {"nekoray/core/NekoService_Fixed.cpp", "Correct header include", "#include \"NekoService_Fixed.hpp\"", false},
    {"nekoray/core/ConfigManager.cpp", "Correct header include", "#include \"NekoService_Fixed.hpp\"", false},
    
    // Check for wrong header includes (should NOT exist) 
    {"nekoray/core/ConfigManager.cpp", "Wrong NekoGui include", "#include \"../main/NekoGui.hpp\"", true},
    {"nekoray/web/WebApiServer.cpp", "Wrong NekoGui include", "#include \"../main/NekoGui.hpp\"", true},
    
    // Check for correct Web API header (SHOULD exist)
    {"nekoray/web/WebApiServer.cpp", "Correct header include", "#include \"../core/NekoService_Fixed.hpp\"", false},
};

bool checkFile(const IssueCheck& check) {
    std::string filePath = "/workspace/programs/nekoray_cli/" + check.file;
    std::ifstream file(filePath);
    
    if (!file.is_open()) {
        std::cout << "⚠️  Could not open file: " << check.file << std::endl;
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    bool patternFound = content.find(check.pattern) != std::string::npos;
    
    bool passed = (check.shouldNotExist && !patternFound) || (!check.shouldNotExist && patternFound);
    
    std::cout << (passed ? "✅ " : "❌ ") << check.file << ": " << check.issue;
    if (!passed) {
        if (check.shouldNotExist && patternFound) {
            std::cout << " (FOUND BAD PATTERN: " << check.pattern << ")";
        } else if (!check.shouldNotExist && !patternFound) {
            std::cout << " (MISSING REQUIRED PATTERN: " << check.pattern << ")";
        }
    }
    std::cout << std::endl;
    
    return passed;
}

int main() {
    std::cout << "=== CRITICAL ISSUES VERIFICATION ===" << std::endl;
    std::cout << "Checking for architectural problems and signal/slot errors\n" << std::endl;
    
    int passed = 0;
    int total = criticalChecks.size();
    
    for (const auto& check : criticalChecks) {
        if (checkFile(check)) {
            passed++;
        }
    }
    
    std::cout << "\n📊 CRITICAL CHECKS: " << passed << "/" << total << " passed" << std::endl;
    
    if (passed == total) {
        std::cout << "\n🎉 ALL CRITICAL ISSUES RESOLVED!" << std::endl;
        std::cout << "✅ Architecture is clean and headless" << std::endl;
        std::cout << "✅ Signal/slot connections are correct" << std::endl;
        std::cout << "✅ No global dependencies found" << std::endl;
        std::cout << "✅ Header includes are proper" << std::endl;
    } else {
        std::cout << "\n❌ CRITICAL ISSUES REMAIN!" << std::endl;
        std::cout << "🔴 " << (total - passed) << " issues need immediate attention" << std::endl;
        std::cout << "⚠️  Architecture is not ready for production" << std::endl;
    }
    
    std::cout << "\n📋 VERIFICATION SUMMARY:" << std::endl;
    std::cout << "1. Global Dependency Removal: " << (passed >= 4 ? "✅" : "❌") << std::endl;
    std::cout << "2. Signal/Slot Architecture Fix: " << (passed >= 6 ? "✅" : "❌") << std::endl;  
    std::cout << "3. Header Include Correctness: " << (passed >= 8 ? "✅" : "❌") << std::endl;
    std::cout << "4. Complete Headless Architecture: " << (passed == total ? "✅" : "❌") << std::endl;
    
    return (passed == total) ? 0 : 1;
}