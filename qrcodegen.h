/* 
 * QR Code generator library (C)
 * 
 * Copyright (c) Project Nayuki. (MIT License)
 * https://www.nayuki.io/page/qr-code-generator-library
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * - The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * - The Software is provided "as is", without warranty of any kind, express or
 *   implied, including but not limited to the warranties of merchantability,
 *   fitness for a particular purpose and noninfringement. In no event shall the
 *   authors or copyright holders be liable for any claim, damages or other
 *   liability, whether in an action of contract, tort or otherwise, arising from,
 *   out of or in connection with the Software or the use or other dealings in the
 *   Software.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/* 
 * This library creates QR Code symbols, which is a type of two-dimension barcode.
 * Invented by Denso Wave and described in the ISO/IEC 18004 standard.
 * A QR Code structure is an immutable square grid of dark and light cells.
 * The library provides functions to create a QR Code from text or binary data.
 * The library covers the QR Code Model 2 specification, supporting all versions (sizes)
 * from 1 to 40, all 4 error correction levels, and 4 character encoding modes.
 * 
 * Ways to create a QR Code object:
 * - High level: Take the payload data and call qrcodegen_encodeText() or qrcodegen_encodeBinary().
 * - Low level: Custom-make the list of segments and call
 *   qrcodegen_encodeSegments() or qrcodegen_encodeSegmentsAdvanced().
 * (Note that all ways require supplying the desired error correction level and various byte buffers.)
 */


/*---- Enum and struct types----*/

/* 
 * The error correction level in a QR Code symbol.
 */
enum qrcodegen_Ecc {
	// Must be declared in ascending order of error protection
	// so that an internal qrcodegen function works properly
	qrcodegen_Ecc_LOW = 0 ,  // The QR Code can tolerate about  7% erroneous codewords
	qrcodegen_Ecc_MEDIUM  ,  // The QR Code can tolerate about 15% erroneous codewords
	qrcodegen_Ecc_QUARTILE,  // The QR Code can tolerate about 25% erroneous codewords
	qrcodegen_Ecc_HIGH    ,  // The QR Code can tolerate about 30% erroneous codewords
};


/* 
 * The mask pattern used in a QR Code symbol.
 */
enum qrcodegen_Mask {
	// A special value to tell the QR Code encoder to
	// automatically select an appropriate mask pattern
	qrcodegen_Mask_AUTO = -1,
	// The eight actual mask patterns
	qrcodegen_Mask_0 = 0,
	qrcodegen_Mask_1,
	qrcodegen_Mask_2,
	qrcodegen_Mask_3,
	qrcodegen_Mask_4,
	qrcodegen_Mask_5,
	qrcodegen_Mask_6,
	qrcodegen_Mask_7,
};


/* 
 * Describes how a segment's data bits are interpreted.
 */
enum qrcodegen_Mode {
	qrcodegen_Mode_NUMERIC      = 0x1,
	qrcodegen_Mode_ALPHANUMERIC = 0x2,
	qrcodegen_Mode_BYTE         = 0x4,
	qrcodegen_Mode_KANJI        = 0x8,
	qrcodegen_Mode_ECI          = 0x7,
};


/* 
 * A segment of character/binary/control data in a QR Code symbol.
 * The mid-level way to create a segment is to take the payload data
 * and call a factory function such as qrcodegen_makeNumeric().
 * The low-level way to create a segment is to custom-make the bit buffer
 * and initialize a qrcodegen_Segment struct with appropriate values.
 * Even in the most favorable conditions, a QR Code can only hold 7089 characters of data.
 * Any segment longer than this is meaningless for the purpose of generating QR Codes.
 * Moreover, the maximum allowed bit length is 32767 because
 * the largest QR Code (version 40) has 31329 modules.
 */
struct qrcodegen_Segment {
	// The mode indicator of this segment.
	enum qrcodegen_Mode mode;
	
	// The length of this segment's unencoded data. Measured in characters for
	// numeric/alphanumeric/kanji mode, bytes for byte mode, and 0 for ECI mode.
	// Always zero or positive. Not the same as the data's bit length.
	int numChars;
	
	// The data bits of this segment, packed in bitwise big endian.
	// Can be null if the bit length is zero.
	uint8_t *data;
	
	// The number of valid data bits used in the buffer. Requires
	// 0 <= bitLength <= 32767, and bitLength <= (capacity of data array) * 8.
	// The character count (numChars) must agree with the mode and the bit buffer length.
	int bitLength;
};



/*---- Macro constants and functions ----*/

#define qrcodegen_VERSION_MIN   1  // The minimum version number supported in the QR Code Model 2 standard
#define qrcodegen_VERSION_MAX  40  // The maximum version number supported in the QR Code Model 2 standard

// Calculates the number of bytes needed to store any QR Code up to and including the given version number,
// as a compile-time constant. For example, 'uint8_t buffer[qrcodegen_BUFFER_LEN_FOR_VERSION(25)];'
// can store any single QR Code from version 1 to 25 (inclusive). The result fits in an int (or int16).
// Requires qrcodegen_VERSION_MIN <= n <= qrcodegen_VERSION_MAX.
#define qrcodegen_BUFFER_LEN_FOR_VERSION(n)  ((((n) * 4 + 17) * ((n) * 4 + 17) + 7) / 8 + 1)

template<qrcodegen_Ecc ecl, int version, enum qrcodegen_Mask mask>
bool qrcodegen_encodeSegmentsAdvancedSophia(const char*text, int text_len, struct qrcodegen_Segment seg, uint8_t qrcode[qrcodegen_BUFFER_LEN_FOR_VERSION(1)])


#ifdef __cplusplus
}
#endif