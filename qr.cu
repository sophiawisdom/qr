#define MAX_INPUTS 17
#include <stdio.h>
#include "curand_kernel.h"


__device__ static char tlds[37][3] = {
    "ar", "at", "au", "cc", "ch", "cn", "co", "de", "dk", "es", "eu", "fm", "fr", "gr", "hk", "hu", "io", "is", "it", "kr", "ly", "me", "mx", "nl", "no", "nz", "pt", "pw", "rs", "ru", "se", "sg", "si", "tn", "tw", "uk", "us"
};


// Returns the product of the two given field elements modulo GF(2^8/0x11D).
// All inputs are valid. This could be implemented as a 256*256 lookup table.
__device__ __attribute__((always_inline)) inline unsigned char reedSolomonMultiply(unsigned char x, unsigned char y) {
	// Russian peasant multiplication
	unsigned char z = 0;
	for (int i = 7; i >= 0; i--) {
		z = (unsigned char)((z << 1) ^ ((z >> 7) * 0x11D));
		z ^= ((y >> i) & 1) * x;
	}
	return z;
}

template<int degree>
__device__ __attribute__((noinline)) void fill_out_table(unsigned char *table) {
    unsigned char generator[degree] = {127, 122, 154, 164, 11, 68, 117};
    #pragma unroll 1
    for (int i = 0; i < 256; i++) {
        #pragma unroll 7 // otherwise generator goes in local memory
        for (int j = 0; j < 7; j++) {
            table[i*8 + j] = reedSolomonMultiply(generator[j], i);
        }
        table[i*8+7] = 0;
    }
}

template<int degree, int len>
__device__ __attribute__((always_inline)) inline unsigned int calculate_bitcount(unsigned long long *ll_table, unsigned char input_data[17]) {
    constexpr int dataLen = 19;
    unsigned char new_data[dataLen] = {0};
    new_data[0] = 0b01000000 | ((len&0b11110000)>>4);
    new_data[1] = ((len&0b1111)<<4);

    for (int i = 0; i < len; i++) {
        new_data[i+1] |= ((input_data[i]&0b11110000)>>4);
        new_data[i+2] |= ((input_data[i]&0b1111)<<4);
    }

    // ideally this is precomputed
    bool EC = true; // alternating EC and 11
    for (int i = len; i < MAX_INPUTS; i++) {
        unsigned char val = EC ? 0xEC : 0x11;
        EC = !EC;
        new_data[i+2] = val;
    }

    unsigned char generator[7] = {127, 122, 154, 164, 11, 68, 117};

    int offset = 0;
    unsigned char ecc_data[7] = {0};
    unsigned long long ecc_data_ll = 0;
    #pragma unroll dataLen
	for (int i = 0; i < dataLen; i++) {  // Polynomial division
        int zero_idx = (degree + offset - 1)%degree;
        unsigned char factor = new_data[i] ^ ((ecc_data_ll & (0xFFULL<<(zero_idx*8)))>>(zero_idx*8));
        ecc_data_ll &= (0xFFFFFFFFFFFFFFFFULL^(0xFFULL << (zero_idx*8)));
        // ecc_data[zero_idx] = 0;
        unsigned long long table_val = ll_table[factor];
        // printf("factor is %02X, table_val is %llu, ecc_data_ll is %llu\n", factor, table_val, ecc_data_ll);
        // printf("ecc_data_ll is %llu\n", ecc_data_ll);
        #pragma unroll degree
		for (int j = 0; j < degree; j++) {
            unsigned int v = (j+offset);
            unsigned int idx = (v)%degree;
            unsigned long long table_ll = (table_val & (0xFFULL<<(j*8)))>>(j*8);
            ecc_data_ll ^= (table_ll << (idx*8));
            // ecc_data[idx] ^= reedSolomonMultiply(generator[j], factor);
        }
        offset = (offset+1)%degree;
    }
    /*
    if (threadIdx.x == 0 && blockIdx.x == 0) {
        printf("For input data \"%.17s\", ECC:\n", input_data);
        for (int i = 0; i < 8; i++) {
            printf("ecc: %02X\t", ecc_data[i]);
            unsigned int result = (ecc_data_ll & (0xFFULL<<(i*8)))>>(i*8);
            printf("ll %02X\n", result);
        }
    }
    */

    unsigned char all_codewords[26];
    memcpy(all_codewords, new_data, 19);

    for (int i = 0; i < 7; i++) {
        int idx = (i+4)%7;
        all_codewords[19+i] = (ecc_data_ll & (0xFFULL<<(idx*8)))>>(idx*8);
    }

    unsigned char masks[8][26] = {
        { 153,153,153,102,102,102,153,153,153,102,102,102,153,153,153,150,102,153,150,102,102,102,153,153,102,153 },
        { 204,204,204,51,51,51,204,204,204,51,51,51,204,204,204,195,51,204,195,51,51,51,204,51,204,51 },
        { 0,0,0,170,170,170,85,85,85,0,0,0,170,170,170,170,170,85,85,85,85,85,0,0,170,85 },
        { 97,134,24,146,73,36,134,24,97,36,146,73,24,97,134,17,134,73,34,73,36,146,24,36,134,73 },
        { 195,195,195,150,150,150,60,60,60,195,195,195,150,150,150,154,90,15,12,60,60,60,195,60,105,195 },
        { 12,32,194,171,170,186,76,100,198,131,8,48,174,170,234,170,171,198,65,147,25,49,194,67,201,87 },
        { 124,167,202,191,171,250,206,108,230,163,218,61,175,234,254,175,171,206,99,155,57,179,202,111,205,87 },
        { 11,208,189,98,118,39,185,27,145,126,7,224,216,157,137,210,118,185,30,70,228,110,189,129,118,185 }
    };

    int max_count = 0;
    int min_mask = 20;
    #pragma unroll 8
    for (int mask = 5; mask < 7; mask++) {
        int total_count = 0;
        #pragma unroll 26
        for (int i = 0; i < 26; i++) {
            total_count += __builtin_popcount(all_codewords[i] ^ masks[mask][i]);
        }
        // lots of black or lots of white
        total_count = abs(104-total_count);
        if (total_count > max_count) {
            max_count = total_count;
            min_mask = mask;
        }
    }

    return (max_count << 16) + min_mask;
}

/*
__device__ static inline char random_char(curandState_t &state) {
    char val = (curand(state) % 62)+48;
    if (val <= '9') {
        return val;
    }
    val += 8; // 9 -> A
    if (val <= 'A') {
        return val;
    }
    val += 7;
    return val;
}
*/

#define RANDOM_CHAR(state) ( \
    { \
        char val = (curand(&state) % 62) + 48; \
        (val <= '9') ? val : ((val+8) <= 'Z') ? val+8 : val+13; \
    } \
)

__global__ void generate_qrs(int *lock, unsigned char *output) {
    unsigned int seed = blockIdx.x * gridDim.x + threadIdx.x + lock[64];
    curandState_t state;
    curand_init(seed, 0, 0, &state);

    unsigned char input_data[17];

    input_data[4] = '.';

    input_data[7] = '/';

    constexpr int len = 17;

    constexpr int degree = 7;
    __shared__ unsigned long long ll_table[256];
    fill_out_table<degree>((unsigned char *)ll_table);

    unsigned long long final_result = 0;
    #pragma unroll 9
    for (int iter = 0; iter < (1000*1000*1000); iter++) {
        if ((iter % (10*1000)) == 0) {
            input_data[0] = RANDOM_CHAR(state);
            input_data[1] = RANDOM_CHAR(state);
            input_data[2] = RANDOM_CHAR(state);
            input_data[3] = RANDOM_CHAR(state);

            int tld_idx = curand(&state) % 37;
            input_data[5] = tlds[tld_idx][0];
            input_data[6] = tlds[tld_idx][1];
        }
        if (iter%676 == 0) {
            #pragma unroll 9
            for (int i = 0; i < 9; i++) {
                input_data[i+8] = RANDOM_CHAR(state); 
            }
        }
        input_data[iter%9 + 8] = RANDOM_CHAR(state);

        unsigned int ret = calculate_bitcount<degree, len>(ll_table, input_data);
        // printf("Got value %d for string %.17s\n", count, input_data);
        if (ret >= (46 << 16)) {
            unsigned char mask = (ret & 0xFFFF);
            unsigned char count = ((ret&0xFFFF0000)>>16);
            int output_index = atomicAdd(lock, 1);
            #pragma unroll 17
            for (int input_idx = 0; input_idx < 17; input_idx++) {
                output[(output_index*19)+input_idx] = input_data[input_idx];
            }
            output[(output_index*19)+17] = mask + '0';
            output[(output_index*19)+18] = count;
        }
    }
}

int run_qr(int *lock, unsigned char *output) {
    int blocks = 256;
    int threads = 256;
    generate_qrs<<<blocks, threads>>>(lock, output);
    // generate_qrs<<<1, 1>>>(lock, output);
    return 0;
}