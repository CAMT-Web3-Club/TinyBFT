#include "EspNowTransport.h"

#ifndef ESP_PLATFORM

// Stub implementation for non-ESP platforms
// Full implementation is in the ESP-IDF component

namespace libbyzea {
// Empty - stub only
}

#else

// ESP32-C3 implementation
#include "Message.h"
#include "Node.h"
#include <esp_log.h>
#include <cstring>

static const char* TAG = "EspNowTransport";

namespace libbyzea {

bool EspNowTransport::send_success_ = true;

EspNowTransport::EspNowTransport() : initialized_(false) {}

EspNowTransport::~EspNowTransport() {
    if (initialized_) {
        esp_now_deinit();
    }
}

void EspNowTransport::init() {
    // Initialize WiFi in station mode
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(ret));
        return;
    }
    
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi set mode failed: %s", esp_err_to_name(ret));
        return;
    }
    
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi start failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Initialize ESP-NOW
    ret = esp_now_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ESP-NOW init failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Register callbacks
    esp_now_register_send_cb(send_cb);
    esp_now_register_recv_cb(recv_cb);
    
    initialized_ = true;
    ESP_LOGI(TAG, "ESP-NOW initialized successfully");
}

void EspNowTransport::send(Message* m, int dest_id) {
    if (!initialized_ || dest_id < 0 || dest_id >= (int)peers_.size()) {
        return;
    }
    
    const uint8_t* peer_mac = peers_[dest_id].peer_addr;
    int size = m->size();
    
    esp_err_t ret = esp_now_send(peer_mac, (const uint8_t*)m->contents(), size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Send failed: %s", esp_err_to_name(ret));
    }
}

Message* EspNowTransport::recv() {
    if (recv_queue_.empty()) {
        return nullptr;
    }
    
    Message* m = recv_queue_.front();
    recv_queue_.pop();
    return m;
}

void EspNowTransport::add_peer(int id, const uint8_t* mac) {
    if (id < 0 || id >= kMaxPeers) {
        return;
    }
    
    esp_now_peer_info_t peer;
    memset(&peer, 0, sizeof(peer));
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = 0;  // Use current channel
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    
    esp_err_t ret = esp_now_add_peer(&peer);
    if (ret != ESP_OK && ret != ESP_ERR_ESPNOW_EXIST) {
        ESP_LOGE(TAG, "Add peer failed: %s", esp_err_to_name(ret));
        return;
    }
    
    if (id >= (int)peers_.size()) {
        peers_.resize(id + 1);
    }
    peers_[id] = peer;
    
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "Added peer %d: %s", id, mac_str);
}

std::string EspNowTransport::peer_addr_string(int id) const {
    if (id < 0 || id >= (int)peers_.size()) {
        return "unknown";
    }
    
    char buf[32];
    const uint8_t* mac = peers_[id].peer_addr;
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return buf;
}

void EspNowTransport::send_cb(const uint8_t* mac, esp_now_send_status_t status) {
    send_success_ = (status == ESP_NOW_SEND_SUCCESS);
    if (!send_success_) {
        ESP_LOGW(TAG, "Send to %02X:%02X:%02X:%02X:%02X:%02X failed",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
}

void EspNowTransport::recv_cb(const uint8_t* mac, const uint8_t* data, int len) {
    // This is a simplified callback - in practice, you'd need to 
    // figure out which node this came from based on the MAC address
    // and push to the appropriate receive queue
    
    // For now, just log that we received something
    ESP_LOGD(TAG, "Received %d bytes from %02X:%02X:%02X:%02X:%02X:%02X",
             len, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

} // namespace libbyzea

#endif // ESP_PLATFORM
