#include "EspNowTransport.h"
#include "Fragmentation.h"

#ifndef ESP_PLATFORM

namespace libbyzea {
}

#else

#include "Message.h"
#include "Node.h"
#include <esp_log.h>
#include <cstring>

static const char* TAG = "EspNowTransport";

namespace libbyzea {

bool EspNowTransport::send_success_ = true;
EspNowTransport* EspNowTransport::instance_ = nullptr;

EspNowTransport::EspNowTransport() : initialized_(false), wifi_channel_(6) {
    instance_ = this;
}

EspNowTransport::~EspNowTransport() {
    if (initialized_) {
        esp_now_deinit();
    }
    if (instance_ == this) {
        instance_ = nullptr;
    }
}

void EspNowTransport::init() {
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
    
    ret = esp_wifi_set_channel(wifi_channel_, WIFI_SECOND_CHAN_NONE);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "WiFi set channel failed: %s (may already be set)", esp_err_to_name(ret));
    }
    
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi start failed: %s", esp_err_to_name(ret));
        return;
    }
    
    ret = esp_now_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ESP-NOW init failed: %s", esp_err_to_name(ret));
        return;
    }
    
    esp_now_register_send_cb(send_cb);
    esp_now_register_recv_cb(recv_cb);
    
    initialized_ = true;
    ESP_LOGI(TAG, "ESP-NOW initialized on channel %d", wifi_channel_);
}

void EspNowTransport::send(Message* m, int dest_id) {
    if (!initialized_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(peers_mutex_);
    if (dest_id < 0 || dest_id >= (int)peers_.size()) {
        ESP_LOGW(TAG, "Invalid dest_id: %d", dest_id);
        return;
    }
    
    const uint8_t* peer_mac = peers_[dest_id].peer_addr;
    
    std::vector<Message*> frags = Fragmentation::fragment(m, dest_id);
    
    for (Message* frag : frags) {
        esp_err_t ret = esp_now_send(peer_mac, 
            (const uint8_t*)frag->contents(), frag->size());
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Send failed: %s", esp_err_to_name(ret));
        }
        
        if (frag != m) {
            delete frag;
        }
    }
}

Message* EspNowTransport::recv() {
    std::lock_guard<std::mutex> lock(recv_mutex_);
    if (recv_queue_.empty()) {
        return nullptr;
    }
    
    Message* m = recv_queue_.front();
    recv_queue_.pop();
    return m;
}

void EspNowTransport::add_peer(int id, const uint8_t* mac) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    
    if (id < 0 || id >= kMaxPeers) {
        ESP_LOGW(TAG, "Invalid peer id: %d (max: %d)", id, kMaxPeers);
        return;
    }
    
    esp_now_peer_info_t peer;
    memset(&peer, 0, sizeof(peer));
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = wifi_channel_;
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
    
    std::string mac_str = mac_to_string(mac);
    mac_to_id_[mac_str] = id;
    
    ESP_LOGI(TAG, "Added peer %d: %s", id, mac_str.c_str());
}

std::string EspNowTransport::peer_addr_string(int id) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(peers_mutex_));
    if (id < 0 || id >= (int)peers_.size()) {
        return "unknown";
    }
    
    const uint8_t* mac = peers_[id].peer_addr;
    return mac_to_string(mac);
}

std::string EspNowTransport::mac_to_string(const uint8_t* mac) const {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(buf);
}

int EspNowTransport::find_node_by_mac(const uint8_t* mac) const {
    std::string mac_str = mac_to_string(mac);
    auto it = mac_to_id_.find(mac_str);
    if (it != mac_to_id_.end()) {
        return it->second;
    }
    return -1;
}

void EspNowTransport::send_cb(const esp_now_send_info_t* tx_info, esp_now_send_status_t status) {
    (void)tx_info;
    send_success_ = (status == ESP_NOW_SEND_SUCCESS);
    if (!send_success_ && instance_) {
        ESP_LOGW(TAG, "ESP-NOW send failed");
    }
}

void EspNowTransport::recv_cb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, int len) {
    if (!instance_) {
        return;
    }
    
    if (esp_now_info == nullptr || data == nullptr) {
        return;
    }
    
    const uint8_t* sender_mac = esp_now_info->src_addr;
    
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             sender_mac[0], sender_mac[1], sender_mac[2], 
             sender_mac[3], sender_mac[4], sender_mac[5]);
    
    int sender_id = instance_->find_node_by_mac(sender_mac);
    if (sender_id < 0) {
        ESP_LOGW(TAG, "Unknown sender MAC: %s", mac_str);
        return;
    }
    
    if (len < (int)sizeof(Message_rep)) {
        ESP_LOGW(TAG, "Message too short: %d bytes", len);
        return;
    }
    
    Message* msg = new Message(len);
    memcpy(msg->contents(), data, len);
    msg->set_size(len);
    
    if (Fragmentation::is_fragment(msg)) {
        Message* reassembled = Fragmentation::reassemble(msg);
        delete msg;
        
        if (reassembled) {
            std::lock_guard<std::mutex> lock(instance_->recv_mutex_);
            instance_->recv_queue_.push(reassembled);
            ESP_LOGD(TAG, "Reassembled %d bytes from node %d", 
                     reassembled->size(), sender_id);
        }
    } else {
        std::lock_guard<std::mutex> lock(instance_->recv_mutex_);
        instance_->recv_queue_.push(msg);
        ESP_LOGD(TAG, "Queued %d bytes from node %d", msg->size(), sender_id);
    }
}

} 

#endif
