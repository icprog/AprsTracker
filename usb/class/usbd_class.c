/*
 * usbd_class.c
 *
 *  Created on: 17.04.2017
 *      Author: korgeaux
 */

#include "usbd_class.h"
#include "usbd_core.h"

uint8_t cdcDataBuffer[CDC_DATA_PACKET_SIZE];
uint8_t configDataBuffer[CONFIG_PACKET_SIZE];

static uint8_t USBD_Class_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_Class_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_Class_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t USBD_Class_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_Class_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t *USBD_Class_GetConfigDescriptor(uint16_t *length);
uint8_t *USBD_Class_GetUsrStrDescriptor(struct _USBD_HandleTypeDef *pdev, uint8_t index, uint16_t *length);

USBD_ClassTypeDef USBD_Class = {
        USBD_Class_Init, //Init
        USBD_Class_DeInit, //DeInit
        USBD_Class_Setup, //Setup
        NULL, //EP0_TxSent
        USBD_Class_EP0_RxReady, //EP0_RxReady
        NULL, //DataIn
        USBD_Class_DataOut, //DataOut
        NULL, //SOF
        NULL, //IsoINIncomplete
        NULL, //IsoOUTIncomplete
        USBD_Class_GetConfigDescriptor, //GetHSConfigDescriptor
        USBD_Class_GetConfigDescriptor, //GetFSConfigDescriptor
        USBD_Class_GetConfigDescriptor, //GetOtherSpeedConfigDescriptor
        NULL, //GetDeviceQualifierDescriptor
        USBD_Class_GetUsrStrDescriptor, //GetUsrStrDescriptor
};

static uint8_t USBD_Class_ConfigDescriptor[0x6D] = {

        //Configuration Descriptor:
        //------------------------------
        0x09, //bLength
        USB_DESC_TYPE_CONFIGURATION, //bDescriptorType
        0x6D, //wTotalLength[0]
        0x00, //wTotalLength[1]
        0x03, //bNumInterfaces
        0x01, //bConfigurationValue
        0x04, //iConfiguration ("Configuration")
        0xE0, //bmAttributes (Self-powered Device, Remote-Wakeup)
        0x32, //bMaxPower (100 mA)

        //Interface Association Descriptor:
        //------------------------------
        0x08, //bLength
        0x0B, //bDescriptorType
        0x00, //bFirstInterface
        0x02, //bInterfaceCount
        0x02, //bFunctionClass (Communication Device Class)
        0x02, //bFunctionSubClass (Abstract Control Model)
        0x00, //bFunctionProtocol
        0x06, //iFunction ("APRS UART")

        //Interface Descriptor:
        //------------------------------
        0x09, //bLength
        USB_DESC_TYPE_INTERFACE, //bDescriptorType
        0x00, //bInterfaceNumber
        0x00, //bAlternateSetting
        0x01, //bNumEndPoints
        0x02, //bInterfaceClass (Communication Device Class)
        0x02, //bInterfaceSubClass (Abstract Control Model)
        0x01, //bInterfaceProtocol (ITU-T V.250)
        0x07, //iInterface ("CDC Control")

        //CDC Header Functional Descriptor:
        //------------------------------
        0x05, //bFunctionalLength
        0x24, //bDescriptorType
        0x00, //bDescriptorSubtype
        0x10, //bcdCDC[0]
        0x01, //bcdCDC[1]

        //CDC Call Management Functional Descriptor:
        //------------------------------
        0x05, //bFunctionalLength
        0x24, //bDescriptorType
        0x01, //bDescriptorSubtype
        0x00, //bmCapabilities
        0x01, //bDataInterface

        //CDC Abstract Control Management Functional Descriptor:
        //------------------------------
        0x04, //bFunctionalLength
        0x24, //bDescriptorType
        0x02, //bDescriptorSubtype
        0x02, //bmCapabilities

        //CDC Union Functional Descriptor:
        //------------------------------
        0x05, //bFunctionalLength
        0x24, //bDescriptorType
        0x06, //bDescriptorSubtype
        0x00, //bControlInterface
        0x01, //bSubordinateInterface(0)

        //Endpoint Descriptor:
        //------------------------------
        0x07, //bLength
        USB_DESC_TYPE_ENDPOINT, //bDescriptorType
        CDC_CMD_EP, //bEndpointAddress (IN Endpoint)
        0x03, //bmAttributes (Transfer: Interrupt / Synch: None / Usage: Data)
        LOBYTE(CDC_CMD_PACKET_SIZE), //wMaxPacketSize[0]
        HIBYTE(CDC_CMD_PACKET_SIZE), //wMaxPacketSize[1]
        0x10, //bInterval

        //Interface Descriptor:
        //------------------------------
        0x09, //bLength
        USB_DESC_TYPE_INTERFACE, //bDescriptorType
        0x01, //bInterfaceNumber
        0x00, //bAlternateSetting
        0x02, //bNumEndPoints
        0x0A, //bInterfaceClass (CDC Data)
        0x00, //bInterfaceSubClass
        0x00, //bInterfaceProtocol
        0x08, //iInterface ("APRS Port")

        //Endpoint Descriptor:
        //------------------------------
        0x07, //bLength
        USB_DESC_TYPE_ENDPOINT, //bDescriptorType
        CDC_OUT_EP, //bEndpointAddress (OUT Endpoint)
        0x02, //bmAttributes (Transfer: Bulk / Synch: None / Usage: Data)
        LOBYTE(CDC_DATA_PACKET_SIZE), //wMaxPacketSize[0]
        HIBYTE(CDC_DATA_PACKET_SIZE), //wMaxPacketSize[1]
        0x00, //bInterval

        //Endpoint Descriptor:
        //------------------------------
        0x07, //bLength
        USB_DESC_TYPE_ENDPOINT, //bDescriptorType
        CDC_IN_EP, //bEndpointAddress (IN Endpoint)
        0x02, //bmAttributes (Transfer: Bulk / Synch: None / Usage: Data)
        LOBYTE(CDC_DATA_PACKET_SIZE), //wMaxPacketSize[0]
        HIBYTE(CDC_DATA_PACKET_SIZE), //wMaxPacketSize[1]
        0x00, //bInterval

        //Interface Association Descriptor:
        //------------------------------
        0x08, //bLength
        0x0B, //bDescriptorType
        0x02, //bFirstInterface
        0x01, //bInterfaceCount
        0xFF, //bFunctionClass (Communication Device Class)
        0xFF, //bFunctionSubClass (Abstract Control Model)
        0xFF, //bFunctionProtocol
        0x09, //iFunction ("AprsTracker")

        //Interface Descriptor:
        //------------------------------
        0x09, //bLength
        USB_DESC_TYPE_INTERFACE, //bDescriptorType
        0x02, //bInterfaceNumber
        0x00, //bAlternateSetting
        0x02, //bNumEndPoints
        0xFF, //bInterfaceClass (Vendor specific)
        0xFF, //bInterfaceSubClass (Vendor specific)
        0xFF, //bInterfaceProtocol (Vendor specific)
        0x09, //iInterface ("AprsTracker")

        //Endpoint Descriptor:
        //------------------------------
        0x07, //bLength
        USB_DESC_TYPE_ENDPOINT, //bDescriptorType
        CONFIG_OUT_EP, //bEndpointAddress (OUT Endpoint)
        0x02, //bmAttributes (Transfer: Bulk / Synch: None / Usage: Data)
        LOBYTE(CONFIG_PACKET_SIZE), //wMaxPacketSize[0]
        HIBYTE(CONFIG_PACKET_SIZE), //wMaxPacketSize[1]
        0x00, //bInterval

        //Endpoint Descriptor:
        //------------------------------
        0x07, //bLength
        USB_DESC_TYPE_ENDPOINT, //bDescriptorType
        CONFIG_IN_EP, //bEndpointAddress (IN Endpoint)
        0x02, //bmAttributes (Transfer: Bulk / Synch: None / Usage: Data)
        LOBYTE(CONFIG_PACKET_SIZE), //wMaxPacketSize[0]
        HIBYTE(CONFIG_PACKET_SIZE), //wMaxPacketSize[1]
        0x00, //bInterval

};

static uint8_t USBD_Class_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
    USBD_LL_OpenEP(pdev, CDC_OUT_EP, USBD_EP_TYPE_BULK, CDC_DATA_PACKET_SIZE);
    USBD_LL_OpenEP(pdev, CDC_IN_EP, USBD_EP_TYPE_BULK, CDC_DATA_PACKET_SIZE);
    USBD_LL_OpenEP(pdev, CDC_CMD_EP, USBD_EP_TYPE_INTR, CDC_CMD_PACKET_SIZE);
    USBD_LL_OpenEP(pdev, CONFIG_OUT_EP, USBD_EP_TYPE_BULK, CONFIG_PACKET_SIZE);
    USBD_LL_OpenEP(pdev, CONFIG_IN_EP, USBD_EP_TYPE_BULK, CONFIG_PACKET_SIZE);

    USBD_LL_PrepareReceive(pdev, CDC_OUT_EP, cdcDataBuffer, CDC_DATA_PACKET_SIZE);
    USBD_LL_PrepareReceive(pdev, CONFIG_OUT_EP, configDataBuffer, CONFIG_PACKET_SIZE);
    return USBD_OK;
}

static uint8_t USBD_Class_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
    USBD_LL_CloseEP(pdev, CDC_OUT_EP);
    USBD_LL_CloseEP(pdev, CDC_IN_EP);
    USBD_LL_CloseEP(pdev, CDC_CMD_EP);
    USBD_LL_CloseEP(pdev, CONFIG_OUT_EP);
    USBD_LL_CloseEP(pdev, CONFIG_IN_EP);
    return USBD_OK;
}

static uint8_t USBD_Class_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req) {
    uint8_t interface = 0;
    static CDCLineCoding lineCoding = { 9600, 0, 0, 8 };

    switch (req->bmRequest & USB_REQ_TYPE_MASK) {
    case USB_REQ_TYPE_CLASS:
        switch (req->bRequest) {
        case CDC_REQ_SET_LINE_CODING:
            USBD_CtlPrepareRx(pdev, (uint8_t *) &lineCoding, req->wLength);
            break;
        case CDC_REQ_GET_LINE_CODING:
            USBD_CtlSendData(pdev, (uint8_t *) &lineCoding, req->wLength);
            break;
        case CDC_REQ_SET_CONTROL_LINE_STATE:
            break;
        default:
            USBD_CtlError(pdev, req);
            return USBD_FAIL;
        }
        break;

    case USB_REQ_TYPE_STANDARD:
        switch (req->bRequest) {
        case USB_REQ_GET_INTERFACE:
            USBD_CtlSendData(pdev, &interface, 1);
            break;
        default:
            USBD_CtlError(pdev, req);
            return USBD_FAIL;
        }
    }
    return USBD_OK;
}

static uint8_t USBD_Class_EP0_RxReady(USBD_HandleTypeDef *pdev) {
    return USBD_OK;
}

static uint8_t USBD_Class_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum) {
    //int length = USBD_LL_GetRxDataSize(pdev, epnum);
    switch (epnum) {
    case CDC_OUT_EP:
        USBD_LL_PrepareReceive(pdev, CDC_OUT_EP, cdcDataBuffer, CDC_DATA_PACKET_SIZE);
        break;
    case CONFIG_OUT_EP:
        USBD_LL_PrepareReceive(pdev, CONFIG_OUT_EP, configDataBuffer, CONFIG_PACKET_SIZE);
        break;
    }

    return USBD_OK;
}

static uint8_t *USBD_Class_GetConfigDescriptor(uint16_t *length) {
    *length = sizeof(USBD_Class_ConfigDescriptor);
    return USBD_Class_ConfigDescriptor;
}

uint8_t *USBD_Class_GetUsrStrDescriptor(struct _USBD_HandleTypeDef *pdev, uint8_t index, uint16_t *length) {
    static uint8_t stringBuffer[USBD_MAX_STR_DESC_SIZ];
    const char *string;
    switch (index) {
    case 6:
        string = "APRS UART";
        break;
    case 7:
        string = "CDC Control";
        break;
    case 8:
        string = "APRS Port";
        break;
    case 9:
        string = "AprsTracker";
        break;
    default:
        string = "AprsTracker";
        break;
    }

    USBD_GetString((uint8_t *) string, stringBuffer, length);
    return stringBuffer;
}
