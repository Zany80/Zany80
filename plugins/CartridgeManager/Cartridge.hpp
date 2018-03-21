#pragma once

typedef struct __attribute__((__packed__)) {
	union {
		uint8_t raw_data[16 * 0x4000];
		uint8_t banks[16][0x4000];
		struct {
			uint32_t zany;
			uint16_t title;
			uint16_t start;
		} metadata;
	};
} Cartridge;
