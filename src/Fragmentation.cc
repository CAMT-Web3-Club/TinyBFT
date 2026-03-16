#include "Fragmentation.h"

#include <cstring>
#include <algorithm>

#include "th_assert.h"

namespace libbyzea {

std::unordered_map<uint32_t, Fragmentation::FragmentInfo> Fragmentation::pending_fragments_;
std::mutex Fragmentation::mutex_;
size_t Fragmentation::max_payload_size_ = 0;
uint32_t Fragmentation::next_msg_id_ = 1;

std::vector<Message*> Fragmentation::fragment(Message* msg, int dest_id) {
    std::vector<Message*> fragments;
    
    size_t msg_size = msg->size();
    size_t payload_per_frag = max_payload_size();
    
    if (msg_size <= payload_per_frag) {
        fragments.push_back(msg);
        return fragments;
    }
    
    uint16_t total_frags = (msg_size + payload_per_frag - 1) / payload_per_frag;
    uint32_t msg_id = next_msg_id_++;
    
    const char* src = msg->contents();
    
    for (uint16_t seq = 0; seq < total_frags; seq++) {
        size_t frag_data_size = std::min(payload_per_frag, msg_size - seq * payload_per_frag);
        
        Message* frag = new Message(kFragHeaderSize + frag_data_size);
        
        FragHeader header;
        header.msg_id = msg_id;
        header.seq = seq;
        header.total = total_frags;
        header.size = frag_data_size;
        header.dest_id = dest_id;
        header.protocol = 1;
        header.flags = (seq == total_frags - 1) ? 0x01 : 0x00;
        
        char* dst = frag->contents();
        memcpy(dst, &header, kFragHeaderSize);
        memcpy(dst + kFragHeaderSize, src + seq * payload_per_frag, frag_data_size);
        
        frag->set_size(kFragHeaderSize + frag_data_size);
        fragments.push_back(frag);
    }
    
    return fragments;
}

Message* Fragmentation::reassemble(Message* frag_msg) {
    FragHeader header = get_header(frag_msg);
    
    if (header.size == 0 || header.total == 0) {
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = pending_fragments_.find(header.msg_id);
    
    if (it == pending_fragments_.end()) {
        if (header.total == 1) {
            char* data = frag_msg->contents() + kFragHeaderSize;
            size_t data_size = header.size;
            
            Message* reassembled = new Message(data_size);
            memcpy(reassembled->contents(), data, data_size);
            reassembled->set_size(data_size);
            return reassembled;
        }
        
        FragmentInfo info;
        info.msg_id = header.msg_id;
        info.total = header.total;
        info.timestamp = 0;
        
        it = pending_fragments_.emplace(header.msg_id, std::move(info)).first;
    }
    
    FragmentInfo& info = it->second;
    
    if (header.seq < info.fragments.size() && info.fragments[header.seq] != nullptr) {
        delete frag_msg;
        return nullptr;
    }
    
    if (header.seq >= info.fragments.size()) {
        info.fragments.resize(header.seq + 1, nullptr);
    }
    info.fragments[header.seq] = frag_msg;
    
    if (!info.complete()) {
        return nullptr;
    }
    
    size_t total_size = 0;
    for (auto& f : info.fragments) {
        if (f) {
            FragHeader h = get_header(f);
            total_size += h.size;
        }
    }
    
    Message* reassembled = new Message(total_size);
    char* dst = reassembled->contents();
    size_t offset = 0;
    
    for (auto& f : info.fragments) {
        if (f) {
            FragHeader h = get_header(f);
            char* src = f->contents() + kFragHeaderSize;
            memcpy(dst + offset, src, h.size);
            offset += h.size;
            delete f;
        }
    }
    
    reassembled->set_size(total_size);
    pending_fragments_.erase(it);
    
    return reassembled;
}

bool Fragmentation::is_fragment(const Message* msg) {
    if ((size_t)msg->size() < kFragHeaderSize) {
        return false;
    }
    
    const char* contents = msg->contents();
    const FragHeader* header = reinterpret_cast<const FragHeader*>(contents);
    
    return header->protocol == 1 && header->total > 0;
}

FragHeader Fragmentation::get_header(const Message* msg) {
    FragHeader header;
    memset(&header, 0, sizeof(header));
    
    if ((size_t)msg->size() >= kFragHeaderSize) {
        memcpy(&header, msg->contents(), kFragHeaderSize);
    }
    
    return header;
}

void Fragmentation::clear_pending() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : pending_fragments_) {
        for (auto& frag : pair.second.fragments) {
            delete frag;
        }
    }
    pending_fragments_.clear();
}

void Fragmentation::clear_pending(uint32_t msg_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = pending_fragments_.find(msg_id);
    if (it != pending_fragments_.end()) {
        for (auto& frag : it->second.fragments) {
            delete frag;
        }
        pending_fragments_.erase(it);
    }
}

}
