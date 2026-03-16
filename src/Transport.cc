#include "Transport.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <errno.h>
#include <queue>
#include <mutex>

#include "Message.h"
#include "Node.h"
#include "Principal.h"
#include "EspNowTransport.h"

#ifndef NDEBUG
#define NDEBUG
#endif

namespace libbyzea {

class UdpTransport : public Transport {
public:
    UdpTransport() : sock_(-1), node_(nullptr) {}
    ~UdpTransport() override { if (sock_ >= 0) close(sock_); }
    
    void init() override;
    void send(Message* m, int dest_id) override;
    Message* recv() override;
    void add_peer(int id, const uint8_t* addr) override;
    bool is_ready() override { return sock_ >= 0; }
    size_t max_payload_size() const override { return 65507; }
    std::string peer_addr_string(int id) const override;
    TransportType type() const override { return TransportType::UDP; }
    
    void set_node(Node* node) { node_ = node; }
    int sock() const { return sock_; }

private:
    int sock_;
    Node* node_;
    Addr peer_addrs_[32];
    int num_peers_ = 0;
};

void UdpTransport::init() {
    sock_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ < 0) {
        perror("UdpTransport: socket creation failed");
        return;
    }
    
    int reuse_addr = 1;
    setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
    
    int flags = fcntl(sock_, F_GETFL, 0);
    fcntl(sock_, F_SETFL, flags | O_NONBLOCK);
}

void UdpTransport::send(Message* m, int dest_id) {
    if (sock_ < 0) return;
    
    const Addr* to = nullptr;
    
    if (dest_id == Node::All_replicas) {
        to = node_->i_to_p(0)->address();
    } else if (dest_id >= 0 && dest_id < num_peers_) {
        to = &peer_addrs_[dest_id];
    } else {
        return;
    }
    
    int size = m->size();
    int error = 0;
    while (error < size) {
        error = sendto(sock_, m->contents(), size, 0, 
                       (struct sockaddr*)to, sizeof(Addr));
        if (error < 0 && errno != EAGAIN) {
            perror("UdpTransport::send: sendto");
            break;
        }
    }
}

Message* UdpTransport::recv() {
    if (sock_ < 0) return nullptr;
    
    Message* m = new Message(Max_message_size);
    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);
    
    int ret = recvfrom(sock_, m->contents(), m->msize(), 0,
                       (struct sockaddr*)&from, &from_len);
    
    if (ret >= (int)sizeof(Message_rep) && ret >= m->size()) {
        return m;
    }
    
    delete m;
    return nullptr;
}

void UdpTransport::add_peer(int id, const uint8_t* addr) {
    if (id >= 0 && id < 32) {
        memset(&peer_addrs_[id], 0, sizeof(Addr));
        peer_addrs_[id].sin_family = AF_INET;
        memcpy(&peer_addrs_[id].sin_addr.s_addr, addr, 4);
        peer_addrs_[id].sin_port = htons(9001);
        
        if (id >= num_peers_) num_peers_ = id + 1;
    }
}

std::string UdpTransport::peer_addr_string(int id) const {
    if (id >= 0 && id < num_peers_) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%s:%d", 
                 inet_ntoa(peer_addrs_[id].sin_addr),
                 ntohs(peer_addrs_[id].sin_port));
        return buf;
    }
    return "unknown";
}

// Loopback transport - sends to self (for testing on single machine)
class LoopbackTransport : public Transport {
public:
    LoopbackTransport() : node_(nullptr) {}
    ~LoopbackTransport() override = default;
    
    void init() override {
        // No socket needed for loopback
        // Messages are routed internally
    }
    
    void send(Message* m, int dest_id) override {
        // For loopback, just queue the message for self-receive
        // This simulates network behavior
        if (dest_id == Node::All_replicas || dest_id == node_id_) {
            // Queue for self
            std::lock_guard<std::mutex> lock(queue_mutex_);
            Message* copy = new Message(m->size());
            memcpy(copy->contents(), m->contents(), m->size());
            copy->set_size(m->size());
            recv_queue_.push(copy);
        }
        // For other destinations, we'd need actual network
    }
    
    Message* recv() override {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (!recv_queue_.empty()) {
            Message* m = recv_queue_.front();
            recv_queue_.pop();
            return m;
        }
        return nullptr;
    }
    
    void add_peer(int id, const uint8_t* addr) override {
        (void)addr;  // Not used in loopback
        peer_ids_.push_back(id);
    }
    
    bool is_ready() override { return true; }
    
    size_t max_payload_size() const override { return 65507; }
    
    std::string peer_addr_string(int id) const override {
        (void)id;  // Not used in loopback
        return "loopback";
    }
    
    TransportType type() const override { return TransportType::LOOPBACK; }
    
    void set_node(Node* node) { node_ = node; }
    void set_node_id(int id) { node_id_ = id; }

private:
    Node* node_;
    int node_id_ = 0;
    std::vector<int> peer_ids_;
    std::queue<Message*> recv_queue_;
    std::mutex queue_mutex_;
};

std::unique_ptr<Transport> Transport::create(TransportType type) {
    switch (type) {
        case TransportType::UDP:
            return std::make_unique<UdpTransport>();
        case TransportType::LOOPBACK:
            return std::make_unique<LoopbackTransport>();
        case TransportType::ESP_NOW:
#ifdef ESP_PLATFORM
            return std::make_unique<EspNowTransport>();
#else
            // On non-ESP platforms, return nullptr for ESP_NOW
            fprintf(stderr, "WARNING: ESP_NOW transport not available on this platform\n");
            return nullptr;
#endif
    }
    return nullptr;
}

}
