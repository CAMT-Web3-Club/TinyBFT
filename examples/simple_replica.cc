/**
 * TinyBFT Simple Replica Example
 * 
 * This example demonstrates how to use the TinyBFT library as a replica.
 * It implements a trivial key-value store using NO_STATE_TRANSLATION mode.
 * 
 * Build:
 *   # First build the library, then:
 *   cd examples/build
 *   cmake -DMBEDTLS_INCLUDE_PATH=/path/to/mbedtls/include ..
 *   make
 * 
 * Or manually:
 *   g++ -o simple_replica simple_replica.cc -I../../include -I../../src \
 *       -L../../build -lbyzea -lmbedtls -lmbedcrypto -lmbedx509 -lpthread
 * 
 * Run:
 *   ./simple_replica <config_file> <private_key_file> <port>
 */

#include "libbyz.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <string>

// Simple in-memory key-value store
static std::map<std::string, std::string> kv_store;

/**
 * Execute a committed request.
 * This is the main application-specific logic.
 * 
 * Parameters:
 *   - req: the request from client (contains the command)
 *   - rep: the reply to send back to client  
 *   - ndet: non-deterministic choices (e.g., timestamp)
 *   - client: client ID
 *   - read_only: true if request should not modify state
 * 
 * Returns: 0 on success, -1 if read_only but request is not read-only
 */
int exec_command(Byz_req* req, Byz_rep* rep, Byz_buffer* ndet, int client,
                 bool read_only) {
    // Parse command from request
    // Format: "GET <key>" or "SET <key> <value>"
    std::string cmd(req->contents, req->size);
    
    std::cout << "Executing command from client " << client << ": " << cmd << "\n";
    
    if (cmd.substr(0, 4) == "GET ") {
        // Read-only operation
        std::string key = cmd.substr(4);
        auto it = kv_store.find(key);
        
        if (it != kv_store.end()) {
            std::string value = it->second;
            // Allocate reply buffer - rep->size contains max size
            int max_size = rep->size;
            if ((int)value.size() <= max_size) {
                memcpy(rep->contents, value.data(), value.size());
                rep->size = value.size();
            } else {
                // Value too large
                rep->size = 0;
            }
        } else {
            std::string not_found = "NOT_FOUND";
            memcpy(rep->contents, not_found.data(), not_found.size());
            rep->size = not_found.size();
        }
        return 0;
        
    } else if (cmd.substr(0, 4) == "SET ") {
        // Write operation - check if read-only flag is incorrectly set
        if (read_only) {
            return -1;  // Reject write in read-only mode
        }
        
        // Parse SET command: "SET <key> <value>"
        size_t first_space = cmd.find(' ', 4);
        size_t second_space = cmd.find(' ', first_space + 1);
        
        if (first_space == std::string::npos || second_space == std::string::npos) {
            std::string error = "INVALID_FORMAT";
            memcpy(rep->contents, error.data(), error.size());
            rep->size = error.size();
            return 0;
        }
        
        std::string key = cmd.substr(4, first_space - 4);
        std::string value = cmd.substr(second_space + 1);
        
        kv_store[key] = value;
        
        std::string ok = "OK";
        memcpy(rep->contents, ok.data(), ok.size());
        rep->size = ok.size();
        return 0;
        
    } else {
        // Unknown command
        std::string error = "UNKNOWN_COMMAND";
        memcpy(rep->contents, error.data(), error.size());
        rep->size = error.size();
        return 0;
    }
}

/**
 * Compute non-deterministic choices for a request.
 * This is called for each request to provide randomness (e.g., timestamp).
 */
void comp_ndet(Seqno seqno, Byz_buffer* ndet) {
    // For simplicity, we don't use any non-deterministic choices
    ndet->size = 0;
}

/**
 * Handle reply from replica (for recovery)
 */
int recv_reply(Byz_rep* rep) {
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] 
                  << " <config_file> <private_key_file> [port]\n";
        return 1;
    }

    const char* config_file = argv[1];
    const char* private_key_file = argv[2];
    short port = (argc > 3) ? atoi(argv[3]) : 5679;  // Default port for replica 0

    // Allocate memory for state (NO_STATE_TRANSLATION mode)
    const size_t state_size = 1024 * 1024;  // 1MB
    char* state_mem = new char[state_size];
    
    // Initialize the replica
    int ret = Byz_init_replica(config_file, private_key_file, state_mem,
                                state_size, exec_command, comp_ndet, 64,
                                recv_reply, port);
    
    if (ret < 0) {
        std::cerr << "Failed to initialize replica (error code: " << ret << ")\n";
        delete[] state_mem;
        return 1;
    }
    
    std::cout << "Replica initialized successfully (using " << ret 
              << " bytes of state memory)\n";

    std::cout << "Starting replica event loop...\n";
    Byz_replica_run();

    delete[] state_mem;
    return 0;
}
