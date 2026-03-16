/**
 * TinyBFT Test Client - Full consensus test with SET/GET
 */

#include "libbyz.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>

static bool timed_out = false;

long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int main(int argc, char** argv) {
    const char* config_file = "/home/phukrit7171/Development/TinyBFT/test_runtime/test.conf";
    const char* private_key_file = "/home/phukrit7171/Development/TinyBFT/test_runtime/priv/client0.pem";
    (void)argc; (void)argv;

    fprintf(stderr, "Starting test client...\n");

    int ret = Byz_init_client(config_file, private_key_file, 0);
    if (ret != 0) {
        std::cerr << "Failed to initialize client: " << ret << "\n";
        return 1;
    }

    std::cout << "Client initialized successfully\n";

    // Test 1: SET key1=value1
    std::cout << "\n=== Test 1: SET key1=value1 ===\n";
    long start = get_time_ms();
    {
        Byz_req req;
        const char* command = "SET key1=value1";
        int cmd_len = strlen(command);
        
        ret = Byz_alloc_request(&req, cmd_len);
        if (ret != 0) {
            std::cerr << "Failed to allocate request\n";
            return 1;
        }
        
        memcpy(req.contents, command, cmd_len);
        req.size = cmd_len;
        
        Byz_rep rep;
        
        ret = Byz_invoke(&req, &rep, false);
        long elapsed = get_time_ms() - start;
        
        if (ret != 0) {
            std::cerr << "SET request failed (elapsed: " << elapsed << "ms)\n";
            Byz_free_request(&req);
        } else {
            std::cout << "Reply (" << rep.size << " bytes, " << elapsed << "ms): ";
            std::cout.write(rep.contents, rep.size);
            std::cout << "\n";
            Byz_free_reply(&rep);
        }
        Byz_free_request(&req);
    }

    // Test 2: GET key1
    std::cout << "\n=== Test 2: GET key1 ===\n";
    start = get_time_ms();
    {
        Byz_req req;
        const char* command = "GET key1";
        int cmd_len = strlen(command);
        
        ret = Byz_alloc_request(&req, cmd_len);
        if (ret != 0) {
            std::cerr << "Failed to allocate request\n";
            return 1;
        }
        
        memcpy(req.contents, command, cmd_len);
        req.size = cmd_len;
        
        Byz_rep rep;
        
        ret = Byz_invoke(&req, &rep, true);
        long elapsed = get_time_ms() - start;
        
        if (ret != 0) {
            std::cerr << "GET request failed (elapsed: " << elapsed << "ms)\n";
            Byz_free_request(&req);
        } else {
            std::cout << "Reply (" << rep.size << " bytes, " << elapsed << "ms): ";
            std::cout.write(rep.contents, rep.size);
            std::cout << "\n";
            Byz_free_reply(&rep);
        }
        Byz_free_request(&req);
    }

    // Test 3: SET key2=test123
    std::cout << "\n=== Test 3: SET key2=test123 ===\n";
    start = get_time_ms();
    {
        Byz_req req;
        const char* command = "SET key2=test123";
        int cmd_len = strlen(command);
        
        ret = Byz_alloc_request(&req, cmd_len);
        if (ret != 0) {
            std::cerr << "Failed to allocate request\n";
            return 1;
        }
        
        memcpy(req.contents, command, cmd_len);
        req.size = cmd_len;
        
        Byz_rep rep;
        
        ret = Byz_invoke(&req, &rep, false);
        long elapsed = get_time_ms() - start;
        
        if (ret != 0) {
            std::cerr << "SET request failed (elapsed: " << elapsed << "ms)\n";
            Byz_free_request(&req);
        } else {
            std::cout << "Reply (" << rep.size << " bytes, " << elapsed << "ms): ";
            std::cout.write(rep.contents, rep.size);
            std::cout << "\n";
            Byz_free_reply(&rep);
        }
        Byz_free_request(&req);
    }

    // Test 4: GET key2
    std::cout << "\n=== Test 4: GET key2 ===\n";
    start = get_time_ms();
    {
        Byz_req req;
        const char* command = "GET key2";
        int cmd_len = strlen(command);
        
        ret = Byz_alloc_request(&req, cmd_len);
        if (ret != 0) {
            std::cerr << "Failed to allocate request\n";
            return 1;
        }
        
        memcpy(req.contents, command, cmd_len);
        req.size = cmd_len;
        
        Byz_rep rep;
        
        ret = Byz_invoke(&req, &rep, true);
        long elapsed = get_time_ms() - start;
        
        if (ret != 0) {
            std::cerr << "GET request failed (elapsed: " << elapsed << "ms)\n";
            Byz_free_request(&req);
        } else {
            std::cout << "Reply (" << rep.size << " bytes, " << elapsed << "ms): ";
            std::cout.write(rep.contents, rep.size);
            std::cout << "\n";
            Byz_free_reply(&rep);
        }
        Byz_free_request(&req);
    }

    std::cout << "\n=== All tests completed ===\n";
    return 0;
}
