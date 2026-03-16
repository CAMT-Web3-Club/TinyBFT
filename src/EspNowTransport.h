#ifndef _EspNowTransport_h
#define _EspNowTransport_h

#include "Transport.h"

// ESP-NOW transport stub for Linux
// Full implementation in ESP-IDF component

#ifdef ESP_PLATFORM

#include <esp_now.h>
#include <esp_wifi.h>

namespace libbyzea {

class EspNowTransport : public Transport {
public:
    static constexpr size_t kMaxFragment = 1470;
    static constexpr uint8_t kMaxPeers = 20;
    
    EspNowTransport();
    ~EspNowTransport() override;
    
    void init() override;
    void send(Message* m, int dest_id) override;
    Message* recv() override;
    void add_peer(int id, const uint8_t* mac) override;
    bool is_ready() override { return initialized_; }
    size_t max_payload_size() const override { return kMaxFragment; }
    std::string peer_addr_string(int id) const override;
    TransportType type() const override { return TransportType::ESP_NOW; }
    
private:
    static void send_cb(const uint8_t* mac, esp_now_send_status_t status);
    static void recv_cb(const uint8_t* mac, const uint8_t* data, int len);
    
    bool initialized_ = false;
    std::vector<esp_now_peer_info_t> peers_;
    std::queue<Message*> recv_queue_;
    static bool send_success_;
};

} 

#else

// Stub for non-ESP platforms - returns nullptr from factory
namespace libbyzea {
class EspNowTransport : public Transport {
public:
    void init() override {}
    void send(Message* m, int dest_id) override { (void)m; (void)dest_id; }
    Message* recv() override { return nullptr; }
    void add_peer(int id, const uint8_t* mac) override { (void)id; (void)mac; }
    bool is_ready() override { return false; }
    size_t max_payload_size() const override { return 1470; }
    std::string peer_addr_string(int id) const override { (void)id; return "ESP-NOW (not available)"; }
    TransportType type() const override { return TransportType::ESP_NOW; }
};
}

#endif

#endif
