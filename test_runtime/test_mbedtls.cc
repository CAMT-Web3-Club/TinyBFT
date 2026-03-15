#include <iostream>
#include <string>
#include <mbedtls/pk.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

int main() {
    std::cout << "Initializing MbedTLS..." << std::endl;
    
    mbedtls_pk_context pk;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    
    mbedtls_pk_init(&pk);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    
    std::cout << "Seeding DRBG..." << std::endl;
    int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);
    if (ret != 0) {
        std::cerr << "DRBG seed failed: " << ret << std::endl;
        return 1;
    }
    std::cout << "DRBG seeded successfully" << std::endl;
    
    std::cout << "Parsing key file..." << std::endl;
    ret = mbedtls_pk_parse_keyfile(&pk, "priv/r0.pem", NULL, mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) {
        std::cerr << "Key parse failed: " << ret << std::endl;
        return 1;
    }
    std::cout << "Key parsed successfully" << std::endl;
    
    mbedtls_pk_free(&pk);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    
    return 0;
}
