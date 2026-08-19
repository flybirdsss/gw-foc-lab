#ifndef __DRV8323_H
#define __DRV8323_H

#include "main.h"

/* ================= SPI Frame ================= */

#define DRV8323_RW_SHIFT        15U
#define DRV8323_ADDR_SHIFT      11U

#define DRV8323_READ            (1U << DRV8323_RW_SHIFT)
#define DRV8323_WRITE           (0U << DRV8323_RW_SHIFT)

#define DRV8323_ADDR_MASK       0x7800U
#define DRV8323_DATA_MASK       0x07FFU


/* ================= Register Address ================= */

#define DRV8323_REG_FAULT1      0x00U
#define DRV8323_REG_FAULT2      0x01U
#define DRV8323_REG_CTRL        0x02U
#define DRV8323_REG_GATE_HS     0x03U
#define DRV8323_REG_GATE_LS     0x04U
#define DRV8323_REG_OCP         0x05U
#define DRV8323_REG_CSA         0x06U


/* ================= Check Result ================= */

typedef enum
{
    DRV8323_CHECK_OK = 0,
    DRV8323_CHECK_SPI_ERROR,
    DRV8323_CHECK_VALUE_ERROR

} DRV8323_CheckStatus_t;


typedef struct
{
    uint8_t  addr;
    uint16_t actual;
    uint16_t expected;

} DRV8323_CheckError_t;


/* ================= Function ================= */

HAL_StatusTypeDef DRV8323_ReadReg(uint8_t addr,
                                  uint16_t *data);

HAL_StatusTypeDef DRV8323_WriteReg(uint8_t addr,
                                   uint16_t data);

DRV8323_CheckStatus_t DRV8323_CheckSPI(
    DRV8323_CheckError_t *error
);

#endif