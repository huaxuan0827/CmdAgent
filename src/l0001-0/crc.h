#ifndef __L0001_0_CRC_H__
#define __L0001_0_CRC_H__
uint32_t crc32(uint32_t crc, const unsigned char *buf, uint32_t size);
uint16_t crc16(uint16_t sum, const unsigned char *buf, uint32_t size);
#endif
