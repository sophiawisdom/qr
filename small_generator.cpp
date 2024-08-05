
#define VERSION 1
#define MAX_CODEPOINTS 18
#define COUNT_LEN 8

#include <cstring>
#include <stdio.h>

testable void addEccAndInterleave(uint8_t data[], int version, enum qrcodegen_Ecc ecl, uint8_t result[]) {
	int numBlocks = 1;
	int blockEccLen = 7; // https://github.com/nayuki/QR-Code-generator/blob/master/c/qrcodegen.c#L100 ECC_CODEWORDS_PER_BLOCK[0][1]
	int rawCodewords = getNumRawDataModules(version) / 8;
	int dataLen = getNumDataCodewords(version, ecl);
	int numShortBlocks = numBlocks - rawCodewords % numBlocks;
	int shortBlockDataLen = rawCodewords / numBlocks - blockEccLen;
	
	// Split data into blocks, calculate ECC, and interleave
	// (not concatenate) the bytes into a single sequence
	uint8_t rsdiv[qrcodegen_REED_SOLOMON_DEGREE_MAX];
	reedSolomonComputeDivisor(blockEccLen, rsdiv);
	const uint8_t *dat = data;
	for (int i = 0; i < numBlocks; i++) {
		int datLen = shortBlockDataLen + (i < numShortBlocks ? 0 : 1);
		uint8_t *ecc = &data[dataLen];  // Temporary storage
		reedSolomonComputeRemainder(dat, datLen, rsdiv, blockEccLen, ecc);
		for (int j = 0, k = i; j < datLen; j++, k += numBlocks) {  // Copy data
			if (j == shortBlockDataLen)
				k -= numShortBlocks;
			result[k] = dat[j];
		}
		for (int j = 0, k = dataLen + i; j < blockEccLen; j++, k += numBlocks)  // Copy ECC
			result[k] = ecc[j];
		dat += datLen;
	}
}

void generate_code(char input_data[MAX_CODEPOINTS], int len) {
    printf("mode: 0100\n"); // mode byte
    char count_bits[8];
    for (int i = 0; i < 8; i++) {
        count_bits[i] = (len & (1<<(7-i))) ? '1' : '0';
    }
    printf("count: %.8s\ndata: ", count_bits);
    for (int i = 0; i < len; i++) {
        char bit_string[8];
        for (int j = 0; j < 8; j++) {
            bit_string[j] = (input_data[i] & (1<<(7-j))) ? '1' : '0';
        }
        printf("%.8s", bit_string);
    }
    printf("\n");

    printf("terminator: 0000\n");
}

int main(int argc, char **argv) {
    printf("String is \"%s\"\n", argv[1]);
    int len = strlen(argv[1]);
    generate_code(argv[1], len);
}
