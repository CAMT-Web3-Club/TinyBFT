/**
 * TinyBFT Simple Client Example
 * 
 * This example demonstrates how to use the TinyBFT library as a client.
 * 
 * Build:
 *   g++ -o simple_client simple_client.cc -I../include -L../build -lbyzea -lmbedtls -lmbedcrypto -lmbedx509 -lpthread
 * 
 * Run:
 *   ./simple_client <config_file> <private_key_file>
 * 
 * Example config file (see README for full format):
 *   generic
 *   1
 *   1800000
 *   4
 *   234.5.6.8 3669
 *   esp0 192.168.178.47 3669 pub/esp0.pub
 *   esp1 192.168.178.48 3669 pub/esp1.pub
 *   esp2 192.168.178.49 3669 pub/esp2.pub
 *   esp3 192.168.178.50 3669 pub/esp3.pub
 *   client0 192.168.178.51 3669 pub/client0.pub
 *   181000
 *   150
 *   9999250000
 */

#include "libbyz.h"

#include <cstdio>
#include <cstring>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <config_file> <private_key_file>\n";
        return 1;
    }

    const char* config_file = argv[1];
    const char* private_key_file = argv[2];

    fprintf(stderr, "DEBUG: Starting client\n");
    fflush(stderr);

    // Initialize the client
    // port=0 means first matching entry in config
    fprintf(stderr, "DEBUG: Calling Byz_init_client\n");
    fflush(stderr);
    int ret = Byz_init_client(config_file, private_key_file, 0);
    fprintf(stderr, "DEBUG: Byz_init_client returned %d\n", ret);
    fflush(stderr);
    if (ret != 0) {
        std::cerr << "Failed to initialize client\n";
        return 1;
    }

    std::cout << "Client initialized successfully\n";

    // Allocate a request buffer
    Byz_req req;
    const char* command = "Hello, BFT World!";
    int cmd_len = strlen(command);

    ret = Byz_alloc_request(&req, cmd_len);
    if (ret != 0) {
        std::cerr << "Failed to allocate request\n";
        return 1;
    }

    // Fill in the command
    memcpy(req.contents, command, cmd_len);
    req.size = cmd_len;

    // Allocate a reply buffer
    Byz_rep rep;

    // Send request and wait for reply (synchronous invoke)
    std::cout << "Sending request...\n";
    ret = Byz_invoke(&req, &rep, false);  // false = read-write request

    if (ret != 0) {
        std::cerr << "Request failed\n";
        Byz_free_request(&req);
        return 1;
    }

    // Print the reply
    std::cout << "Received reply (" << rep.size << " bytes): ";
    std::cout.write(rep.contents, rep.size);
    std::cout << "\n";

    // Clean up
    Byz_free_request(&req);
    Byz_free_reply(&rep);

    // Reset client for next request (optional)
    Byz_reset_client();

    std::cout << "Client example completed\n";
    return 0;
}
