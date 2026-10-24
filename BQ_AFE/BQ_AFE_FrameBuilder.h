/*
 * BQ_FrameBuilder.h — Pure frame build / decapsulate helpers for the BQ79718
 * wire protocol.
 *
 * Build* helpers write a fully-formed TX frame (init byte + optional device
 * address + register address + payload/len + CRC16) into a caller-provided
 * buffer. BQ_DecapsulateFrame validates a received RX frame and extracts its
 * fields. None of these touch driver state, start TX, or arm RX.
 *
 *  Created on: Apr 21, 2026
 *      Author: Servet
 */

#ifndef BQ_AFE_FRAMEBUILDER_H_
#define BQ_AFE_FRAMEBUILDER_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>



#define BQ_COM_WAKE_BAUD                UINT32_C(2900)
#define BQ_COM_NORMAL_BAUD              UINT32_C(1000000)

#define BQ_AFE_INIT_POS                 0U
#define BQ_AFE_DEV_ADDR_POS             1U
#define BQ_AFE_REG_ADDR_POS             2U
#define BQ_AFE_PAYLOAD_POS              4U

#define BQ_AFE_INIT_BYTE_LEN            1U
#define BQ_AFE_DEV_ADDR_LEN             1U
#define BQ_AFE_REG_ADDR_LEN             2U
#define BQ_AFE_REQ_LEN_FIELD_LEN        1U
#define BQ_AFE_CRC_LEN                  2U

#define BQ_AFE_HEADER_LEN               (BQ_AFE_INIT_BYTE_LEN + BQ_AFE_DEV_ADDR_LEN + BQ_AFE_REG_ADDR_LEN)

#define BQ_AFE_RESPONSE_FRAME_SIZE      (BQ_AFE_HEADER_LEN + BQ_AFE_CRC_LEN)


#define BQ_AFE_MAX_RESPONSE_DATA_SIZE   128U
#define BQ_AFE_MAX_WRITE_DATA_SIZE      8U


typedef enum {
	BQ_AFE_TARGET_BROADCAST 	= -2,
	BQ_AFE_TARGET_STACK	 		= -1,
	BQ_AFE_TARGET_BRIDGE 		= 0,
	BQ_AFE_TARGET_DEVICE_BASE	= BQ_AFE_TARGET_BRIDGE,
}BQ_AFE_Target_t;


#define BQ_AFE_MAX_TX_SIZE   14U
#define BQ_AFE_MAX_TX_FRAMES   8U

typedef struct {
	uint8_t  data[BQ_AFE_MAX_TX_SIZE];
	uint16_t size;
}BQ_AFE_TxFrame_t;


/* Transaction type encoded in the init byte. Single transactions address one
 * device by id; stack/broadcast span the daisy-chain. Internal to the frame
 * builder — derived from the request target by _targetToRequestType. */
typedef enum {
	BQ_AFE_SINGLE_READ             = 0b000,
	BQ_AFE_SINGLE_WRITE            = 0b001,
	BQ_AFE_STACK_READ              = 0b010,
	BQ_AFE_STACK_WRITE             = 0b011,
	BQ_AFE_BROADCAST_READ          = 0b100,
	BQ_AFE_BROADCAST_WRITE         = 0b101,
	BQ_AFE_BROADCAST_WRITE_REVERSE = 0b110,
} BQ_AFE_RequestType_t;


/* Map an addressing target + direction to the on-wire request type;
 * (uint8_t)target doubles as the device address for single transactions. */
static inline BQ_AFE_RequestType_t _targetToRequestType(BQ_AFE_Target_t target, bool isRead) {

	if (target == BQ_AFE_TARGET_BROADCAST) {
		return isRead ? BQ_AFE_BROADCAST_READ : BQ_AFE_BROADCAST_WRITE;
	}

	if (target == BQ_AFE_TARGET_STACK) {
		return isRead ? BQ_AFE_STACK_READ : BQ_AFE_STACK_WRITE;
	}

	return isRead ? BQ_AFE_SINGLE_READ : BQ_AFE_SINGLE_WRITE;
}

/* ---- Frame layout ----------------------------------------------------- */

static const uint16_t _crcTable[256] = {
		0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
		0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
		0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
		0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
		0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
		0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
		0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
		0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
		0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
		0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
		0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
		0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
		0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
		0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
		0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
		0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
		0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
		0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
		0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
		0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
		0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
		0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
		0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
		0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
		0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
		0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
		0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
		0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
		0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
		0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
		0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
		0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040,
};

static inline uint16_t _Crc16(const uint8_t *data, uint16_t len) {
	uint16_t crc = 0xFFFF;
	for (uint16_t i = 0; i < len; i++) {
		crc ^= data[i] & 0xFF;
		crc  = _crcTable[crc & 0xFF] ^ (crc >> 8);
	}
	return crc;
}


/* BQ_AFE_MAX_RX_FRAME_SIZE is driver-specific; each device driver defines it
 * based on its own stack depth. */

/* ---- Frame types ------------------------------------------------------ */

typedef enum {
	BQ_AFE_FRAME_TYPE_RESPONSE = 0,
	BQ_AFE_FRAME_TYPE_COMMAND  = 1,
} BQ_AFE_FrameType_t;

typedef union {
	struct {
		uint8_t dataSize    : 3;
		uint8_t reserved    : 1;
		uint8_t requestType : 3;
		uint8_t frameType   : 1;
	} bit;
	uint8_t u8;
} BQ_AFE_InitByteCmd_t;

typedef union {
	struct {
		uint8_t dataSize  : 7; // read byte not exceed 128 byte
		uint8_t frameType : 1;
	}bit;
	uint8_t u8;
}BQ_AFE_InitByteResponse_t;

typedef struct {
	BQ_AFE_InitByteResponse_t initByte;
	uint8_t  devId;
	uint16_t regAddr;
	uint8_t* dataPtr;
	uint16_t crc;
} BQ_AFE_ResponseFrame_t;

/* Build a register WRITE frame for `target` into txFrameOut (data + size). The
 * init-byte request type and device address are derived from `target`
 * internally. Returns true on success, false if the args are invalid or the
 * frame won't fit txFrameOut->data. */
static inline bool BQ_AFE_FRAME_BuildWrite(BQ_AFE_Target_t target, uint16_t regAddr, const uint8_t *data, uint8_t len, BQ_AFE_TxFrame_t *txFrameOut) {

	if (txFrameOut == NULL || len == 0 || len > BQ_AFE_MAX_WRITE_DATA_SIZE)
		return false;

	uint8_t              *out     = txFrameOut->data;
	const uint16_t        outCap  = (uint16_t)sizeof(txFrameOut->data);
	BQ_AFE_RequestType_t  reqType = _targetToRequestType(target, false);

	uint16_t idx          = 0;
	uint16_t txBufferSize = 0;

	bool addDeviceIdInFrame = reqType == BQ_AFE_SINGLE_WRITE ? true : false;

	if (addDeviceIdInFrame)
		txBufferSize = BQ_AFE_INIT_BYTE_LEN + BQ_AFE_DEV_ADDR_LEN + BQ_AFE_REG_ADDR_LEN + len + BQ_AFE_CRC_LEN;
	else
		txBufferSize = BQ_AFE_INIT_BYTE_LEN + BQ_AFE_REG_ADDR_LEN + len + BQ_AFE_CRC_LEN;

	if (txBufferSize > outCap)
		return false;

	memset(out, 0, txBufferSize);

	// Frame initial byte (1)

	BQ_AFE_InitByteCmd_t initByte = {
			.bit = {
					.dataSize    = (uint8_t)(len - 1),
					.reserved    = 0,
					.requestType = reqType,               // Single Write / Broadcast Write vs.
					.frameType   = BQ_AFE_FRAME_TYPE_COMMAND,
			}
	};

	out[idx++] = (uint8_t)initByte.u8;

	// Device address byte (1)

	if (addDeviceIdInFrame)
		out[idx++] = (uint8_t)target;

	// Register bytes (2)

	out[idx++] = (uint8_t)(regAddr >> 8);
	out[idx++] = (uint8_t)(regAddr & 0xFF);

	// Data (len)

	if (data)
		memcpy(&out[idx], data, len);
	idx += len;

	// CRC bytes (2)

	uint16_t crc = _Crc16(out, idx);

	out[idx++] = (uint8_t)(crc & 0xFF);
	out[idx++] = (uint8_t)(crc >> 8);

	txFrameOut->size = idx;
	return true;
}

/* Build a register READ request frame for `target` into txFrameOut. `len` is the
 * number of payload bytes the device should return. See BQ_AFE_FRAME_BuildWrite. */
static inline bool BQ_AFE_FRAME_BuildRequest(BQ_AFE_Target_t target, uint16_t regAddr, uint8_t len, BQ_AFE_TxFrame_t *txFrameOut) {

	if (txFrameOut == NULL || len == 0 || len > BQ_AFE_MAX_RESPONSE_DATA_SIZE)
		return false;

	uint8_t              *out     = txFrameOut->data;
	const uint16_t        outCap  = (uint16_t)sizeof(txFrameOut->data);
	BQ_AFE_RequestType_t  reqType = _targetToRequestType(target, true);

	uint16_t idx          = 0;
	uint16_t txBufferSize = 0;

	bool addDeviceIdInFrame = reqType == BQ_AFE_SINGLE_READ ? true : false;

	if (addDeviceIdInFrame)
		txBufferSize = BQ_AFE_INIT_BYTE_LEN + BQ_AFE_DEV_ADDR_LEN + BQ_AFE_REG_ADDR_LEN + BQ_AFE_REQ_LEN_FIELD_LEN + BQ_AFE_CRC_LEN;
	else
		txBufferSize = BQ_AFE_INIT_BYTE_LEN + BQ_AFE_REG_ADDR_LEN + BQ_AFE_REQ_LEN_FIELD_LEN + BQ_AFE_CRC_LEN;

	if (txBufferSize > outCap)
		return false;

	memset(out, 0, txBufferSize);

	// Frame initial byte (1)

	BQ_AFE_InitByteCmd_t initByte = {
			.bit = {
					.dataSize    = 0,                     // readLen byte gonderimi icin "0" sabit kalir cunku data payload yok
					.reserved    = 0,
					.requestType = reqType,               // Single Read / Broadcast Read vs.
					.frameType   = BQ_AFE_FRAME_TYPE_COMMAND,
			}
	};

	out[idx++] = (uint8_t)initByte.u8;

	// Device address byte (1)

	if (addDeviceIdInFrame)
		out[idx++] = (uint8_t)target;

	// Register bytes (2)

	out[idx++] = (uint8_t)(regAddr >> 8);
	out[idx++] = (uint8_t)(regAddr & 0xFF);

	// Data (1)

	out[idx++] = (uint8_t)(len - 1);

	// CRC bytes (2)

	uint16_t crc = _Crc16(out, idx);

	out[idx++] = (uint8_t)(crc & 0xFF);
	out[idx++] = (uint8_t)(crc >> 8);

	txFrameOut->size = idx;
	return true;
}

/* Total on-wire length of the response frame at `frame`, derived from its init
 * byte's 7-bit dataSize (protocol 0 == 1 payload byte): HEADER + payload + CRC.
 * Returns 0 if `remaining` doesn't hold a whole frame (too few bytes, or a
 * truncated tail). This is the primitive for walking several responses packed
 * back-to-back in one RX buffer: a STACK / BROADCAST read returns one frame per
 * daisy-chained device, concatenated in a single receive-to-idle burst. */
static inline uint16_t BQ_AFE_FRAME_TotalLen(const uint8_t *frame, uint16_t remaining)
{
	if (frame == NULL || remaining < BQ_AFE_RESPONSE_FRAME_SIZE) {
		return 0;
	}

	BQ_AFE_InitByteResponse_t initByte = { .u8 = frame[BQ_AFE_INIT_POS] };

	uint16_t payloadSize = (uint16_t)initByte.bit.dataSize + 1U;   /* 0 == 1 byte */
	uint16_t totalLen    = BQ_AFE_HEADER_LEN + payloadSize + BQ_AFE_CRC_LEN;

	return (totalLen <= remaining) ? totalLen : 0U;
}

static inline bool BQ_AFE_FRAME_Validate(const uint8_t *rawDataPtr, uint16_t dataSize)
{
    uint16_t frameLen = BQ_AFE_HEADER_LEN + dataSize + BQ_AFE_CRC_LEN;

    if (_Crc16(rawDataPtr, frameLen) != 0) {
        return false;
    }

    return true;
}

static inline bool BQ_AFE_FRAME_Decapsulate(const uint8_t *dataPtr, BQ_AFE_ResponseFrame_t *decapsulatedFrame) {

	decapsulatedFrame->initByte.u8 = dataPtr[BQ_AFE_INIT_POS];

	if(decapsulatedFrame->initByte.bit.dataSize > BQ_AFE_MAX_RESPONSE_DATA_SIZE)
		return false;

	decapsulatedFrame->initByte.bit.dataSize += 1;  /* protokolde 0 = 1 byte */

	decapsulatedFrame->devId   = dataPtr[BQ_AFE_DEV_ADDR_POS];
	decapsulatedFrame->regAddr = dataPtr[BQ_AFE_REG_ADDR_POS + 1] | (dataPtr[BQ_AFE_REG_ADDR_POS] << 8);
	decapsulatedFrame->dataPtr = (uint8_t *)&dataPtr[BQ_AFE_PAYLOAD_POS];
	decapsulatedFrame->crc     = dataPtr[decapsulatedFrame->initByte.bit.dataSize + 4] | (dataPtr[decapsulatedFrame->initByte.bit.dataSize + 5] << 8);

	return true;
}



#endif /* BQ_AFE_FRAMEBUILDER_H_ */
