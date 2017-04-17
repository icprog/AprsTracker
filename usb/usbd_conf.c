/*
 * usb_conf.c
 *
 *  Created on: 17.04.2017
 *      Author: korgeaux
 */

#include "stm32f0xx.h"
#include "stm32f0xx_hal.h"
#include "usbd_def.h"
#include "usbd_core.h"
#include "usbd_class.h"

PCD_HandleTypeDef hpcd_USB_FS;

void USB_IRQHandler(void) {
    HAL_PCD_IRQHandler(&hpcd_USB_FS);
}

void HAL_PCD_MspInit(PCD_HandleTypeDef* pcdHandle) {
    if (pcdHandle->Instance == USB) {
        RCC->APB1ENR |= RCC_APB1ENR_USBEN;
        HAL_NVIC_SetPriority(USB_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USB_IRQn);
    }
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef* pcdHandle) {
    if (pcdHandle->Instance == USB) {
        RCC->APB1ENR &= ~RCC_APB1ENR_USBEN;
        HAL_NVIC_DisableIRQ(USB_IRQn);
    }
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd) {
    USBD_LL_SetupStage((USBD_HandleTypeDef*) hpcd->pData, (uint8_t *) hpcd->Setup);
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
    USBD_LL_DataOutStage((USBD_HandleTypeDef*) hpcd->pData, epnum, hpcd->OUT_ep[epnum].xfer_buff);
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
    USBD_LL_DataInStage((USBD_HandleTypeDef*) hpcd->pData, epnum, hpcd->IN_ep[epnum].xfer_buff);
}

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd) {
    USBD_LL_SOF((USBD_HandleTypeDef*) hpcd->pData);
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd) {
    USBD_SpeedTypeDef speed = USBD_SPEED_FULL;

    switch (hpcd->Init.speed) {
    case PCD_SPEED_FULL:
        speed = USBD_SPEED_FULL;
        break;
    default:
        speed = USBD_SPEED_FULL;
        break;
    }

    USBD_LL_SetSpeed((USBD_HandleTypeDef*) hpcd->pData, speed);
    USBD_LL_Reset((USBD_HandleTypeDef*) hpcd->pData);
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd) {
    USBD_LL_Suspend((USBD_HandleTypeDef*) hpcd->pData);
    if (hpcd->Init.low_power_enable) {
        SCB->SCR |= (uint32_t) ((uint32_t) (SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk));
    }
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd) {
    if (hpcd->Init.low_power_enable) {
        SCB->SCR &= (uint32_t) ~((uint32_t) (SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk));
        SystemInit();
    }
    USBD_LL_Resume((USBD_HandleTypeDef*) hpcd->pData);
}

void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
    USBD_LL_IsoOUTIncomplete((USBD_HandleTypeDef*) hpcd->pData, epnum);
}

void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
    USBD_LL_IsoINIncomplete((USBD_HandleTypeDef*) hpcd->pData, epnum);
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd) {
    USBD_LL_DevConnected((USBD_HandleTypeDef*) hpcd->pData);
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd) {
    USBD_LL_DevDisconnected((USBD_HandleTypeDef*) hpcd->pData);
}

USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *pdev) {
    hpcd_USB_FS.pData = pdev;
    pdev->pData = &hpcd_USB_FS;

    hpcd_USB_FS.Instance = USB;
    hpcd_USB_FS.Init.dev_endpoints = 8;
    hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
    hpcd_USB_FS.Init.ep0_mps = DEP0CTL_MPS_64;
    hpcd_USB_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
    hpcd_USB_FS.Init.low_power_enable = DISABLE;
    hpcd_USB_FS.Init.lpm_enable = DISABLE;
    hpcd_USB_FS.Init.battery_charging_enable = DISABLE;
    HAL_PCD_Init(&hpcd_USB_FS);

    HAL_PCDEx_PMAConfig((PCD_HandleTypeDef*) pdev->pData, 0x00, PCD_SNG_BUF, 0x18);
    HAL_PCDEx_PMAConfig((PCD_HandleTypeDef*) pdev->pData, 0x80, PCD_SNG_BUF, 0x58);

    HAL_PCDEx_PMAConfig((PCD_HandleTypeDef*) pdev->pData, 0x01, PCD_SNG_BUF, 0xC0);
    HAL_PCDEx_PMAConfig((PCD_HandleTypeDef*) pdev->pData, 0x81, PCD_SNG_BUF, 0x100);
    HAL_PCDEx_PMAConfig((PCD_HandleTypeDef*) pdev->pData, 0x82, PCD_SNG_BUF, 0x140);

    HAL_PCDEx_PMAConfig((PCD_HandleTypeDef*) pdev->pData, 0x03, PCD_SNG_BUF, 0x148);
    HAL_PCDEx_PMAConfig((PCD_HandleTypeDef*) pdev->pData, 0x83, PCD_SNG_BUF, 0x188);

    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev) {
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_DeInit(pdev->pData);

    switch (hal_status) {
    case HAL_OK:
        usb_status = USBD_OK;
        break;
    case HAL_ERROR:
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY:
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT:
        usb_status = USBD_FAIL;
        break;
    default:
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev) {
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_Start(pdev->pData);

    switch (hal_status) {
    case HAL_OK:
        usb_status = USBD_OK;
        break;
    case HAL_ERROR:
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY:
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT:
        usb_status = USBD_FAIL;
        break;
    default:
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev) {
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_Stop(pdev->pData);

    switch (hal_status) {
    case HAL_OK:
        usb_status = USBD_OK;
        break;
    case HAL_ERROR:
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY:
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT:
        usb_status = USBD_FAIL;
        break;
    default:
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t ep_type, uint16_t ep_mps) {
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_EP_Open(pdev->pData, ep_addr, ep_mps, ep_type);

    switch (hal_status) {
    case HAL_OK:
        usb_status = USBD_OK;
        break;
    case HAL_ERROR:
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY:
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT:
        usb_status = USBD_FAIL;
        break;
    default:
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_EP_Close(pdev->pData, ep_addr);

    switch (hal_status) {
    case HAL_OK:
        usb_status = USBD_OK;
        break;
    case HAL_ERROR:
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY:
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT:
        usb_status = USBD_FAIL;
        break;
    default:
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_EP_Flush(pdev->pData, ep_addr);

    switch (hal_status) {
    case HAL_OK:
        usb_status = USBD_OK;
        break;
    case HAL_ERROR:
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY:
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT:
        usb_status = USBD_FAIL;
        break;
    default:
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_EP_SetStall(pdev->pData, ep_addr);

    switch (hal_status) {
    case HAL_OK:
        usb_status = USBD_OK;
        break;
    case HAL_ERROR:
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY:
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT:
        usb_status = USBD_FAIL;
        break;
    default:
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_EP_ClrStall(pdev->pData, ep_addr);

    switch (hal_status) {
    case HAL_OK:
        usb_status = USBD_OK;
        break;
    case HAL_ERROR:
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY:
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT:
        usb_status = USBD_FAIL;
        break;
    default:
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
    PCD_HandleTypeDef *hpcd = (PCD_HandleTypeDef*) pdev->pData;

    if ((ep_addr & 0x80) == 0x80) {
        return hpcd->IN_ep[ep_addr & 0x7F].is_stall;
    } else {
        return hpcd->OUT_ep[ep_addr & 0x7F].is_stall;
    }
}

USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, uint8_t dev_addr) {
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_SetAddress(pdev->pData, dev_addr);

    switch (hal_status) {
    case HAL_OK:
        usb_status = USBD_OK;
        break;
    case HAL_ERROR:
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY:
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT:
        usb_status = USBD_FAIL;
        break;
    default:
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t *pbuf, uint16_t size) {
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_EP_Transmit(pdev->pData, ep_addr, pbuf, size);

    switch (hal_status) {
    case HAL_OK:
        usb_status = USBD_OK;
        break;
    case HAL_ERROR:
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY:
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT:
        usb_status = USBD_FAIL;
        break;
    default:
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t *pbuf, uint16_t size) {
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_EP_Receive(pdev->pData, ep_addr, pbuf, size);

    switch (hal_status) {
    case HAL_OK:
        usb_status = USBD_OK;
        break;
    case HAL_ERROR:
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY:
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT:
        usb_status = USBD_FAIL;
        break;
    default:
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
    return HAL_PCD_EP_GetRxCount((PCD_HandleTypeDef*) pdev->pData, ep_addr);
}