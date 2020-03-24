#ifndef __CMDDEF_H__
#define __CMDDEF_H__

#define AGENT_PACKET_HEAD_MAGIC 0xFCFC
struct agent_packet{
	uint16_t magic;
	uint8_t devip[4];
};
#define AGENT_PACKET_HEAD_LEN sizeof(struct agnet_packet)

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
