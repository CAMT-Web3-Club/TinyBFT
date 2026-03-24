#ifndef _EspNowTransport_h
#define _EspNowTransport_h

#include "Transport.h"

#ifdef ESP_PLATFORM

#include <esp_now.h>
#include <esp_wifi.h>
#include <string>
#include <unordered_map>
#include <mutex>
#include <queue>
#include <vector>

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
    
    void set_wifi_channel(uint8_t channel) { wifi_channel_ = channel; }

private:
    static void send_cb(const esp_now_send_info_t* tx_info, esp_now_send_status_t status);
    static void recv_cb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, int len);
    
    int find_node_by_mac(const uint8_t* mac) const;
    std::string mac_to_string(const uint8_t* mac) const;
    
    bool initialized_ = false;
    uint8_t wifi_channel_ = 6;
    std::vector<esp_now_peer_info_t> peers_;
    std::queue<Message*> recv_queue_;
    std::mutex recv_mutex_;
    std::mutex peers_mutex_;
    std::unordered_map<std::string, int> mac_to_id_;
    static bool send_success_;
    static EspNowTransport* instance_;
};

} 

#else

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
