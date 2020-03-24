#ifndef __CMDDEF_H__
#define __CMDDEF_H__

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


#endif
