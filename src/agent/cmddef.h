#ifndef __CMDDEF_H__
#define __CMDDEF_H__

#include <stdint.h>

#define AGENT_INIT_PACKET_HEAD_MAGIC 0xFCFA
struct agent_packet{
	uint16_t magic;
	uint8_t devip[4];
	uint16_t port;
}__attribute__((packed));

#define AGENT_PACKET_HEAD_LEN sizeof(struct agent_packet)

#define AGENT_CMD_PACKET_HEAD_MAGIC 0xBEEF
struct cmd_packet{
	uint16_t magic;
	uint8_t cmd;
	uint8_t seqno;
	union {
		uint8_t func[2];
		uint16_t length;
	}f;
	uint16_t crc16;
}__attribute__((packed));


#define CMD_PACKET_MAX_LENGTH		2048
#define CMD_PACKET_HEAD_LEN 		sizeof(struct cmd_packet)

#define CMD_MAX_RESP_TIME_SEC		5

#endif
