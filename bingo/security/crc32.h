/*
 * crc32.h
 *
 *  Created on: 2016-2-20
 *      Author: root
 */

#ifndef SECURITY_CRC32_H_
#define SECURITY_CRC32_H_

#include "../string.h"
using bingo::u32;

namespace bingo { namespace security {
	extern u32 make_crc32(const char* data, size_t data_size);
} }


#endif /* SECURITY_CRC32_H_ */
