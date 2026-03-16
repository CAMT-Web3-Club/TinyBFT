#ifndef _Fragmentation_h
#define _Fragmentation_h

#include <cstdint>
#include <cstddef>
#include <vector>
#include <queue>
#include <unordered_map>
#include <mutex>

#include "Message.h"
#include "types.h"

namespace libbyzea {

#pragma pack(push, 1)
struct FragHeader {
    uint32_t msg_id;      // 4 bytes: Unique message identifier
    uint16_t seq;         // 2 bytes: Fragment sequence (0, 1, 2, ...)
    uint16_t total;       // 2 bytes: Total fragments
    uint16_t size;        // 2 bytes: This fragment's data size
    uint16_t dest_id;     // 2 bytes: Destination replica ID
    uint8_t  protocol;    // 1 byte: Protocol version (1)
    uint8_t  flags;       // 1 byte: Flags: 0x01 = last fragment
    uint16_t padding;     // 2 bytes: Padding to make 16 bytes
}; 
#pragma pack(pop)

static_assert(sizeof(FragHeader) == 16, "FragHeader must be 16 bytes");

constexpr size_t kFragHeaderSize = sizeof(FragHeader);
constexpr size_t kMaxPayload = 1470 - kFragHeaderSize;  // ~1454 bytes for ESP-NOW

class Fragmentation {
public:
    static constexpr uint32_t kFragTimeoutMs = 5000;
    
    struct FragmentInfo {
        std::vector<Message*> fragments;
        uint32_t msg_id;
        uint16_t total;
        uint64_t timestamp;
        bool complete() const {
            return fragments.size() == total;
        }
    };
    
    static std::vector<Message*> fragment(Message* msg, int dest_id);
    
    static Message* reassemble(Message* fragment);
    
    static bool is_fragment(const Message* msg);
    
    static FragHeader get_header(const Message* msg);
    
    static void clear_pending();
    static void clear_pending(uint32_t msg_id);
    
    static void set_max_payload_size(size_t size) {
        max_payload_size_ = size;
    }
    
    static size_t max_payload_size() {
        return max_payload_size_ > 0 ? max_payload_size_ : kMaxPayload;
    }

private:
    static std::unordered_map<uint32_t, FragmentInfo> pending_fragments_;
    static std::mutex mutex_;
    static size_t max_payload_size_;
    static uint32_t next_msg_id_;
};

Message* fragment_message(Message* msg, int dest_id, size_t max_frag_size);

}

#endif
