#include <stdint.h>
#include <random>
// #include "gem5/m5ops.h"

int main() {

    uint32_t lut_0[4][256]; // 4 LUTS with 256 words of 4 bytes
    uint32_t lut_10[4][256]; // 4 LUTS with 256 words of 4 bytes

    uint8_t key[16]; // 16 byte key
    uint32_t round_keys[10][4]; // 10 round keys, 4 words of 4 bytes

    uint8_t plaintext[16];  // 16 byte plaintext

    std::random_device entropy_source;
	std::mt19937 generator(entropy_source()); 
	std::uniform_int_distribution<uint32_t> dist32(0, 0xFFFFFFFF);  // 32-bit
    std::uniform_int_distribution<uint16_t> dist8(0, 0xFF);     // 8-bit

    for (int i = 0; i < 16; i++){
        key[i] = dist8(generator);
        plaintext[i] = dist8(generator);
    }

    for (int i = 0; i < 4; i++){
        for (int j = 0; j < 256; j++){
            lut_0[i][j] = dist32(generator);
            lut_10[i][j] = dist32(generator);
        }
        for (int j = 0; j < 4; j++){
            round_keys[i][j] = dist32(generator);
        }
    }

    int num_rounds = 10;

    // first 9 rounds 
    uint8_t intermed[16]; // 16 byte intermediate state

    for (int i = 0; i < 16; i++){
        intermed[i] = plaintext[i] ^ key[i];
    }


    uint32_t x_0_3, x_4_7, x_8_11, x_12_15;

    for (int i = 0; i < num_rounds - 1; i++){
        x_0_3 = lut_0[0][intermed[0]] ^ lut_0[1][intermed[5]] ^ lut_0[2][intermed[10]] ^ lut_0[3][intermed[15]] ^ round_keys[i+1][0]; 
        x_4_7 = lut_0[0][intermed[4]] ^ lut_0[1][intermed[9]] ^ lut_0[2][intermed[14]] ^ lut_0[3][intermed[3]] ^ round_keys[i+1][1];
        x_8_11 = lut_0[0][intermed[8]] ^ lut_0[1][intermed[13]] ^ lut_0[2][intermed[2]] ^ lut_0[3][intermed[7]] ^ round_keys[i+1][2];
        x_12_15 = lut_0[0][intermed[12]] ^ lut_0[1][intermed[1]] ^ lut_0[2][intermed[6]] ^ lut_0[3][intermed[11]] ^ round_keys[i+1][3];

        intermed[0] = (x_0_3 >> 24) & 0xF;
        intermed[1] = (x_0_3 >> 16) & 0xF;
        intermed[2] = (x_0_3 >> 8) & 0xF;
        intermed[3] = x_0_3 & 0xF;
        intermed[4] = (x_4_7 >> 24) & 0xF;
        intermed[5] = (x_4_7 >> 16) & 0xF;
        intermed[6] = (x_4_7 >> 8) & 0xF;
        intermed[7] = x_4_7 & 0xF;
        intermed[8] = (x_8_11 >> 24) & 0xF;
        intermed[9] = (x_8_11 >> 16) & 0xF;
        intermed[10] = (x_8_11 >> 8) & 0xF;
        intermed[11] = x_8_11 & 0xF;
        intermed[12] = (x_12_15 >> 24) & 0xF;
        intermed[13] = (x_12_15 >> 16) & 0xF;
        intermed[14] = (x_12_15 >> 8) & 0xF;
        intermed[15] = x_12_15 & 0xF;
    }

    // last (10th) round
    uint8_t ciphertext[16];
    x_0_3 = lut_10[0][intermed[0]] ^ lut_10[1][intermed[5]] ^ lut_10[2][intermed[10]] ^ lut_10[3][intermed[15]] ^ round_keys[10][0]; 
    x_4_7 = lut_10[0][intermed[4]] ^ lut_10[1][intermed[9]] ^ lut_10[2][intermed[14]] ^ lut_10[3][intermed[3]] ^ round_keys[10][1];
    x_8_11 = lut_10[0][intermed[8]] ^ lut_10[1][intermed[13]] ^ lut_10[2][intermed[2]] ^ lut_10[3][intermed[7]] ^ round_keys[10][2];
    x_12_15 = lut_10[0][intermed[12]] ^ lut_10[1][intermed[1]] ^ lut_10[2][intermed[6]] ^ lut_10[3][intermed[11]] ^ round_keys[10][3];

    intermed[0] = (x_0_3 >> 24) & 0xF;
    intermed[1] = (x_0_3 >> 16) & 0xF;
    intermed[2] = (x_0_3 >> 8) & 0xF;
    intermed[3] = x_0_3 & 0xF;
    intermed[4] = (x_4_7 >> 24) & 0xF;
    intermed[5] = (x_4_7 >> 16) & 0xF;
    intermed[6] = (x_4_7 >> 8) & 0xF;
    intermed[7] = x_4_7 & 0xF;
    intermed[8] = (x_8_11 >> 24) & 0xF;
    intermed[9] = (x_8_11 >> 16) & 0xF;
    intermed[10] = (x_8_11 >> 8) & 0xF;
    intermed[11] = x_8_11 & 0xF;
    intermed[12] = (x_12_15 >> 24) & 0xF;
    intermed[13] = (x_12_15 >> 16) & 0xF;
    intermed[14] = (x_12_15 >> 8) & 0xF;
    intermed[15] = x_12_15 & 0xF;

    return 0;
}



