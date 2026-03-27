#include <stdio.h>
#include "libbyz.h"

#ifdef PIO_UNIT_TESTING
extern "C" void unity_run_menu();

extern "C" void app_main() {
    printf("Running TinyBFT Unity Tests from main...\n");
    unity_run_menu();
}
#else
extern "C" void app_main() {
    printf("TinyBFT App Started\n");
    
    // Example: Initialize as Replica 0 on port 5679
    // In a real application, you would load config/keys from flash/SPIFFS
    // int r = Byz_init_replica("config.txt", "private_key.pem", 0, 5679);
    //
    // Byz_replica_run();
}
#endif
