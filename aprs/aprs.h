/*
 * aprs.h
 *
 *  Created on: 24.12.2016
 *      Author: korgeaux
 */

#ifndef APRS_APRS_H_
#define APRS_APRS_H_

#include "misc/def.h"

#include "gps/minmea.h"
#include "protocol/ax25.h"

#define MICE_A_MASK 0x04
#define MICE_B_MASK 0x02
#define MICE_C_MASK 0x01

enum MicEMessage {
    MICE_OFF_DUTY = 0b111,
    MICE_EN_ROUTE = 0b110,
    MICE_IN_SERVICE = 0b101,
    MICE_RETURNING = 0b100,
    MICE_COMMITTED = 0b011,
    MICE_SPECIAL = 0b010,
    MICE_PRIORITY = 0b001,
    MICE_EMERGENCY = 0b000,
};

void AprsInit(void);
void AprsSendMicEPosition(struct minmea_sentence_rmc *rmc, struct minmea_sentence_gga *gga, AX25Call *path, 
    size_t pathLength, enum MicEMessage message, char symbol, char symbolTable, char *info, size_t infoLength);
void AprsSendStatus(AX25Call *path, size_t pathLength, char *info, size_t infoLength);

#endif /* APRS_APRS_H_ */
