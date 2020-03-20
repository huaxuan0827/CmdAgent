#ifndef __L0006_NETCMD_H__
#define __L0006_NETCMD_H__


	
#ifdef PLATFORM_RASPI3B
#define L0005_PTRTYPE		uint32_t
#else
#define L0005_PTRTYPE		uint64_t
#endif


#define L0006_INBUF_MAX								10240
#define L0006_NETCMD_PORT_DEFAULT						7777
#define L0006_NETCMD_COMMAND_TIMEOUT					10
#define L0006_NETCMD_WINDOWSIZE_DEFAULT				4

#define L0006_NETCOM_ACK_SUCCESS			0
#define L0006_NETCOM_ACK_FAIL				1
#define L0006_NETCOM_ACK_TIMEOUT			2

#define L0006_NETCMD_RECOVER_CBOARD_ADDR			0xFE

#define L0006_NETCMD_PACKET_MAGIC						0xBEEF
#define L0006_NETCMD_PACKET_MAGIC0					0xEF
#define L0006_NETCMD_PACKET_MAGIC1					0xBE

#define L0006_NETCMD_BUZZ							0x0
#define L0006_NETCMD_BLINK						0x1
#define L0006_NETCMD_WINSIZE						0x2
#define L0006_NETCMD_RESET						0x3
#define L0006_NETCMD_ADDATA						0x4		//STM32 SEND ADDATA ACTIVELY
#define L0006_NETCMD_VERSION						0x5
#define L0006_NETCMD_DEBUGMODE					0x6
#define L0006_NETCMD_DEVSTAT						0x7
#define L0006_NETCMD_RECOVER						0x8			//recover cboard address to 0xFE
#define L0006_NETCMD_SAVECONF						0x9

#define L0006_NETCMD_BUZZACK						0x80
#define L0006_NETCMD_BLINKACK						0x81
#define L0006_NETCMD_WINSIZEACK					0x82
#define L0006_NETCMD_RESETACK						0x83
#define L0006_NETCMD_VERSIONACK					0x84
#define L0006_NETCMD_DEBUGMODEACK					0x85
#define L0006_NETCMD_DEVSTATACK					0x86
#define L0006_NETCMD_RECOVERACK					0x87
#define L0006_NETCMD_SAVECONFACK					0x88

#define L0006_NETCMD_DEVSTAT_FUNC1_QUERY				0
#define L0006_NETCMD_DEVSTAT_FUNC1_SET					1
#define L0006_NETCMD_DEVSTAT_FUNC2_GENMODE				0
#define L0006_NETCMD_DEVSTAT_FUNC2_ADDRMODE				1

#define L0006_NETCMD_SAVECONF_FLAG_READ					0
#define L0006_NETCMD_SAVECONF_FLAG_WRITE				1
#define L0006_NETCMD_SAVECONF_FLAG_SUCCESS				0x9A	
#define L0006_NETCMD_SAVECONF_FLAG_FAILURE				0x9B

#define L0006_NETCMD_VERSION_HBOARD						0
#define L0006_NETCMD_VERSION_CBOARD						1

#define L0006_NETCMD_EX_HEADER_SIZE							sizeof(struct l0006_netcmd_ex_packet)
#define L0006_NETCMD_HEADER_SIZE							sizeof(struct l0006_netcmd_packet)
#define L0006_NETCMD_CHECKSUM_SIZE						(L0006_NETCMD_HEADER_SIZE - sizeof(uint16_t))
#define L0006_STREAM_SEGMAX				10

#define L0006_STREAM_TIMESTAMP_SIZE		(sizeof(struct l0006_netcmd_stream_timestamp))
#define L0006_STREAM_HEADER_SIZE			(sizeof(struct l0006_netcmd_stream_header))
#define L0006_STREAM_SEG_SIZE				(sizeof(struct l0006_netcmd_stream_seg))
#define L0006_STREAM_CRC16_SIZE				4
#define L0006_CHANNEL_MAX						5
#define L0006_NODE_PERCHNL_MAX				16

#define L0006_STREAM_MAX_SIZE					(L0006_NETCMD_HEADER_SIZE + L0006_STREAM_TIMESTAMP_SIZE + (L0006_STREAM_HEADER_SIZE + L0006_STREAM_SEG_SIZE * L0006_STREAM_SEGMAX) * L0006_NODE_PERCHNL_MAX * L0006_CHANNEL_MAX + L0006_STREAM_CRC16_SIZE)

#define L0006_CALIBRATE_FAILURE_BEEP(NC, TASK)			l0006_cmdset_beep(NC, TASK, 250, 3);
#define L0006_CALIBRATE_SUCCESS_BEEP(NC, TASK)			l0006_cmdset_beep(NC, TASK, 250, 1);

#define L0006_NETCMD_FORMAT(PACKET, CMD, SEQNO, FUNC1, FUNC2)	\
	do {\
		(PACKET)->magic = L0006_NETCMD_PACKET_MAGIC;\
		(PACKET)->cmd = CMD;\
		(PACKET)->seqno = SEQNO;\
		(PACKET)->f.func[0] = FUNC1;\
		(PACKET)->f.func[1] = FUNC2;\
		(PACKET)->crc16 = crc16(0, (const unsigned char*)(PACKET), L0006_NETCMD_CHECKSUM_SIZE);\
	}while(0);

struct l0006_netcmd_packet {
	uint16_t magic;
	uint8_t cmd;
	uint8_t seqno;
	union {
		uint8_t func[2];
		uint16_t length;
	}f;
	uint16_t crc16;
}__attribute__((packed));
struct l0006_netcmd_ex_packet {
	uint16_t magic;
	uint8_t cmd;
	uint8_t seqno;
	union {
		uint8_t func[2];
		uint16_t length;
	}f;
	uint16_t crc16;
	uint8_t c[56];
}__attribute__((packed));

struct l0006_netcmd_stream_timestamp {
	uint32_t tv_sec;
	uint32_t tv_usec;
}__attribute__((packed));


struct l0006_netcmd_stream_header {
	uint8_t channel_index;
	uint8_t node_index;
	uint8_t adstatus;
	uint8_t count;
}__attribute__((packed));

struct l0006_netcmd_stream_seg {
	uint32_t data;
	struct l0006_netcmd_stream_timestamp ts;
}__attribute__((packed));

const char* l0006_netcmd_getcmdstring(int cmd);
int l0006_netcmd_getackcmd(int cmd);
int l0006_netcmd_write(struct l0006_netcom_info *nc, void *data, int len, struct l0006_netcom_action *ncaction, int ack);
void l0006_cmdset_beep(struct l0006_netcom_info *nc, struct task_info *wait_task, int msec, int repeat);
int l0006_cmdset_querymode(struct l0006_netcom_info *nc, struct task_info *wait_task, struct l0006_netcmd_packet *pkt);
int l0006_cmdset_setnormmode(struct l0006_netcom_info *nc, struct task_info *wait_task);
int l0006_cmdset_setaddressmode(struct l0006_netcom_info *nc, struct task_info *wait_task);
int l0006_cmdset_recovercboard(struct l0006_netcom_info *nc, struct task_info *wait_task, int channelno);
int l0006_cmdset_setwindowsize(struct l0006_netcom_info *nc, struct task_info *wait_task, uint8_t chnl_bmp);
int l0006_netcmd_general_command(struct l0006_netcom_info *nc, struct task_info *wait_task, struct l0006_netcmd_packet *netcmd, int *outret);
int l0006_netcmd_general_command_ex(struct l0006_netcom_info *nc, struct task_info *wait_task, struct l0006_netcmd_ex_packet *netcmd, int *outret, uint8_t *config, uint8_t sz);
int l0006_netcmd_general_command2(struct l0006_netcom_info *nc, struct task_info *wait_task, struct l0006_netcmd_packet *netcmd, int *outret, uint8_t** pp_packet);
int l0006_netcmd_general_action(void **p_in, void **p_out, void *p, int timedout);
struct l0006_netcom_info* l0006_netcmd_initialize(void* parent);
void l0006_netcmd_release(struct l0006_netcom_info *nc);
int l0006_cmdset_load_sensorconfig(struct l0006_netcom_info *nc, struct task_info *wait_task, int chnl_index, int node_index, void **config, uint8_t *rdsz);
int l0006_cmdset_save_sensorconfig(struct l0006_netcom_info *nc, struct task_info *wait_task, int chnl_index, int node_index, void *config, uint8_t sz);
int l0006_cmdset_hardwareversion(struct l0006_netcom_info *nc, struct task_info *wait_task, int chnl_index, int node_index);

#endif

