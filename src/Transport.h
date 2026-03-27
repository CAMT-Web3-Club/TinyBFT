#ifndef _Transport_h
#define _Transport_h

#include <cstdint>
#include <cstddef>
#include <string>
#include <memory>

namespace libbyzea {

class Message;

// Transport type based on compile-time selection
enum class TransportType {
    UDP = 1,
    LOOPBACK = 2,
    ESP_NOW = 3
};

// Get default transport based on compile definitions
#if defined(CONFIG_TINYBFT_TRANSPORT_ESPNOW) || defined(TINYBFT_TRANSPORT_ESPNOW)
    #define TINYBFT_DEFAULT_TRANSPORT TransportType::ESP_NOW
#elif defined(CONFIG_TINYBFT_TRANSPORT_LOOPBACK) || defined(TINYBFT_TRANSPORT_LOOPBACK)
    #define TINYBFT_DEFAULT_TRANSPORT TransportType::LOOPBACK
#else
    #define TINYBFT_DEFAULT_TRANSPORT TransportType::UDP
#endif

class Transport {
public:
    virtual ~Transport() = default;
    
    virtual void init() = 0;
    virtual void send(Message* m, int dest_id) = 0;
    virtual Message* recv() = 0;
    virtual void add_peer(int id, const uint8_t* addr) = 0;
    virtual bool is_ready() = 0;
    virtual size_t max_payload_size() const = 0;
    virtual std::string peer_addr_string(int id) const = 0;
    virtual TransportType type() const = 0;
    
    static std::unique_ptr<Transport> create(TransportType type);
};

} 

#endif
