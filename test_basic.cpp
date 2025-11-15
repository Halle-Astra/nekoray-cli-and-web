#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>

// Simple test to verify C++17 compilation works
class TestNekoService {
public:
    TestNekoService() = default;
    
    bool initialize(const std::string& configDir) {
        m_configDir = configDir;
        std::cout << "Test service initialized with config: " << configDir << std::endl;
        return true;
    }
    
    bool startProxy(int profileId) {
        m_currentProfile = profileId;
        std::cout << "Test proxy started with profile: " << profileId << std::endl;
        return true;
    }
    
    bool stopProxy() {
        std::cout << "Test proxy stopped" << std::endl;
        m_currentProfile = -1;
        return true;
    }
    
    std::string getStatus() const {
        if (m_currentProfile > 0) {
            return "running";
        }
        return "stopped";
    }
    
    int getCurrentProfile() const {
        return m_currentProfile;
    }

private:
    std::string m_configDir;
    int m_currentProfile = -1;
};

int main() {
    std::cout << "NekoRay Headless - Basic Compilation Test" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    // Test basic C++17 features
    auto service = std::make_unique<TestNekoService>();
    
    // Test initialization
    if (!service->initialize("/tmp/test_config")) {
        std::cerr << "Failed to initialize service" << std::endl;
        return 1;
    }
    
    // Test operations
    std::cout << "Initial status: " << service->getStatus() << std::endl;
    
    if (!service->startProxy(1)) {
        std::cerr << "Failed to start proxy" << std::endl;
        return 1;
    }
    
    std::cout << "Status after start: " << service->getStatus() << std::endl;
    std::cout << "Current profile: " << service->getCurrentProfile() << std::endl;
    
    if (!service->stopProxy()) {
        std::cerr << "Failed to stop proxy" << std::endl;
        return 1;
    }
    
    std::cout << "Status after stop: " << service->getStatus() << std::endl;
    
    // Test STL containers
    std::vector<int> profiles = {1, 2, 3, 4, 5};
    std::map<std::string, std::string> config = {
        {"socks_port", "2080"},
        {"http_port", "2081"},
        {"bind_address", "127.0.0.1"}
    };
    
    std::cout << "\nTest STL containers:" << std::endl;
    std::cout << "Available profiles: ";
    for (const auto& id : profiles) {
        std::cout << id << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Default config:" << std::endl;
    for (const auto& [key, value] : config) {  // C++17 structured bindings
        std::cout << "  " << key << ": " << value << std::endl;
    }
    
    std::cout << "\n✅ Basic compilation test passed!" << std::endl;
    std::cout << "✅ C++17 features working" << std::endl;
    std::cout << "✅ STL containers working" << std::endl;
    std::cout << "✅ Memory management working" << std::endl;
    
    return 0;
}