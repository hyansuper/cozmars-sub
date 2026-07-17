#pragma once

typedef struct __attribute__((packed)) {
	uint8_t major;
	uint8_t minor;
	uint8_t patch;
	uint8_t reserved;
} sub_msg_version_resp_t;

#define GET_SUB_MSG_VER_RESP() {0, 1, 0, 0}