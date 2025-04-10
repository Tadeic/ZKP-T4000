#ifndef __IATU_H
#define __IATU_H

#include <linux/types.h>
#include <stddef.h>
#include "eon_drv.h"

#define IATU_BASE_OFFSET 0x9000
#define IATU_INDEX_OFFSET 0x200

#define IATU_REGION_CTRL_1_OUTBOUND(base, n)                                   \
	(base + n * IATU_INDEX_OFFSET + 0ul)
#define IATU_REGION_CTRL_2_OUTBOUND(base, n)                                   \
	(base + n * IATU_INDEX_OFFSET + 0x4ul)
#define IATU_LWR_BASE_ADDR_OUTBOUND(base, n)                                   \
	(base + n * IATU_INDEX_OFFSET + 0x8ul)
#define IATU_UPPER_BASE_ADDR_OUTBOUND(base, n)                                 \
	(base + n * IATU_INDEX_OFFSET + 0xcul)
#define IATU_LIMIT_ADDR_OUTBOUND(base, n)                                      \
	(base + n * IATU_INDEX_OFFSET + 0x10ul)
#define IATU_LWR_TARGET_ADDR_OUTBOUND(base, n)                                 \
	(base + n * IATU_INDEX_OFFSET + 0x14ul)
#define IATU_UPPER_TARGET_ADDR_OUTBOUND(base, n)                               \
	(base + n * IATU_INDEX_OFFSET + 0x18ul)
#define IATU_UPPER_LIMIT_ADDR_OUTBOUND(base, n)                                \
	(base + n * IATU_INDEX_OFFSET + 0x20ul)

#define IATU_REGION_CTRL_1_INBOUND(base, n)                                    \
	(base + n * IATU_INDEX_OFFSET + 0x100ul)
#define IATU_REGION_CTRL_2_INBOUND(base, n)                                    \
	(base + n * IATU_INDEX_OFFSET + 0x104ul)
#define IATU_LWR_BASE_ADDR_INBOUND(base, n)                                    \
	(base + n * IATU_INDEX_OFFSET + 0x108ul)
#define IATU_UPPER_BASE_ADDR_INBOUND(base, n)                                  \
	(base + n * IATU_INDEX_OFFSET + 0x10cul)
#define IATU_LIMIT_ADDR_INBOUND(base, n)                                       \
	(base + n * IATU_INDEX_OFFSET + 0x110ul)
#define IATU_LWR_TARGET_ADDR_INBOUND(base, n)                                  \
	(base + n * IATU_INDEX_OFFSET + 0x114ul)
#define IATU_UPPER_TARGET_ADDR_INBOUND(base, n)                                \
	(base + n * IATU_INDEX_OFFSET + 0x118ul)
#define IATU_UPPER_LIMIT_ADDR_INBOUND(base, n)                                 \
	(base + n * IATU_INDEX_OFFSET + 0x120ul)

int iatu_map_outbound_bar(struct eon_device *eon_device, uint8_t bar_index,
			  uint8_t atu_index, uint64_t addr, uint64_t size);
int iatu_map_inbound_bar(struct eon_device *eon_device, uint8_t bar_index,
			 uint8_t atu_index, uint64_t addr, uint64_t size);
#endif