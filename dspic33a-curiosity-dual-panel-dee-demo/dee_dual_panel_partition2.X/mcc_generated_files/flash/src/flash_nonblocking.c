/**
 * FLASH Generated Driver Source File
 *
 * @file      flash.c
 *
 * @ingroup   flashdriver
 *
 * @brief     This is the generated driver source file for FLASH driver
 *
 * @skipline @version   Firmware Driver Version 1.0.0
 *
 * @skipline @version   PLIB Version 1.0.4
 *
 * @skipline  Device : dsPIC33AK512MPS512
*/

/*disclaimer*/

#include <stddef.h>
#include "../flash_nonblocking.h"

enum FLASH_PROGRAM_ERASE_ERROR_CODE {
    ERROR_INVALID_OP = 1,
    ERROR_SECURITY_ACCESS_VIOLATION,
    ERROR_FLASH_PANEL_CONTROL_LOGIC,
    ERROR_ROW_OP_SYSTEM_BUS,
    ERROR_ROW_OP_WARM_RESET
};

// User callback and context handles.
static void (*userCallbackHandler)(void *) = NULL;
static void *userContext = NULL;
static bool lock = false;


// Section: Driver Interface
const struct FLASH_INTERFACE flash = {
    .PageAddressGet = FLASH_ErasePageAddressGet,
    .PageOffsetGet = FLASH_ErasePageOffsetGet,
    .NonBlockingPageErase = FLASH_NonBlockingPageErase,
    .NonBlockingBulkErase = NULL,
    .NonBlockingRead = FLASH_NonBlockingRead,
    .NonBlockingWordWrite = FLASH_NonBlockingWordWrite,
    .NonBlockingRowWrite = FLASH_NonBlockingRowWrite,
    .NonBlockingPolledPageErase = NULL,
    .NonBlockingPolledBulkErase = NULL,
    .NonBlockingPolledRead = NULL,
    .NonBlockingPolledWordWrite = NULL,
    .NonBlockingPolledRowWrite = NULL,
    .OperationStatusGet = FLASH_OperationStatusGet

};

void FLASH_Initialize(void)
{
    // Flash Interrupt
    IFS0bits.NVMIF = 0U;
    IEC0bits.NVMIE = 1U;
}

void __attribute__ ( ( interrupt ) ) _NVMInterrupt(void)
{
    IFS0bits.NVMIF = 0U;

    //Interrupt means operation completion, release the lock
    lock = false;

    if (NULL != userCallbackHandler) {
        (*userCallbackHandler)(userContext);
    }
}


enum FLASH_RETURN_STATUS FLASH_OperationStatusGet(void)
{
    enum FLASH_RETURN_STATUS flashReturnStatus = FLASH_NO_ERROR;
    if(1U == NVMCONbits.WR)
    {
        flashReturnStatus = FLASH_OP_BUSY;
    }
    else
    {
        if(1U == NVMCONbits.WRERR)
        {
            flashReturnStatus =  FLASH_WRITE_ERROR;
        }
        else if(ERROR_INVALID_OP == NVMCONbits.WREC)
        {
            flashReturnStatus = FLASH_INVALID_OEPRATION;
        }
        else if(ERROR_SECURITY_ACCESS_VIOLATION == NVMCONbits.WREC)
        {
            flashReturnStatus = FLASH_SECURITY_ACCESS_CONTROL_ERROR;
        }
        else if(ERROR_FLASH_PANEL_CONTROL_LOGIC == NVMCONbits.WREC)
        {
            flashReturnStatus = FLASH_PANEL_CONTROL_LOGIC_ERROR;
        }
        else if(ERROR_ROW_OP_SYSTEM_BUS == NVMCONbits.WREC)
        {
            flashReturnStatus = FLASH_ROW_OP_SYSTEM_BUS_ERROR;
        }
        else if(ERROR_ROW_OP_WARM_RESET == NVMCONbits.WREC)
        {
            flashReturnStatus = FLASH_ROW_OP_WARM_RESET_ERROR;
        }
        else
        {
            flashReturnStatus = FLASH_NO_ERROR;
        }
        lock = false;
    }
    return flashReturnStatus;
}


enum FLASH_RETURN_STATUS FLASH_NonBlockingPageErase(flash_adr_t flashAddress, flash_key_t unlockKey, FLASH_CALLBACK callbackHandler, void* context)
{
    enum FLASH_RETURN_STATUS flashReturnStatus = FLASH_OP_IN_PROGRESS;
    if(false == lock)
    {
        if(unlockKey != (flash_key_t)FLASH_UNLOCK_KEY)
        {
            flashReturnStatus = FLASH_INVALID_KEY;
        }
        if (0U != (flashAddress & ~FLASH_ERASE_PAGE_MASK))
        {
            flashReturnStatus = FLASH_INVALID_ADDRESS;
        }
        else if(NULL == callbackHandler)
        {
            flashReturnStatus  = FLASH_INVALID_CALLBACK_HANDLER;
        }
        else if(0U != NVMCONbits.WR)
        {
            flashReturnStatus = FLASH_OP_BUSY;
        }
        else
        {
            userCallbackHandler = callbackHandler;
            userContext = context;
            lock = true;
            NVMADR = flashAddress;
            NVMCON = FLASH_PAGE_ERASE_OPCODE;
            NVMCONbits.WR = 1U;
            lock = false;
        }
    }
    return flashReturnStatus;
}


enum FLASH_RETURN_STATUS FLASH_NonBlockingWordWrite(flash_adr_t flashAddress, flash_data_t *data, flash_key_t unlockKey, FLASH_CALLBACK callbackHandler, void* context)
{
    enum FLASH_RETURN_STATUS flashReturnStatus = FLASH_OP_IN_PROGRESS;
    if(false == lock)
    {
        if (NULL == data)
        {
            flashReturnStatus = FLASH_INVALID_DATA;
        }
        else if(unlockKey != (flash_key_t)FLASH_UNLOCK_KEY)
        {
            flashReturnStatus = FLASH_INVALID_KEY;
        }
        else if (0U != (flashAddress % FLASH_ADDRESS_MASK))
        {
            flashReturnStatus = FLASH_INVALID_ADDRESS;
        }
        else if(NULL == callbackHandler)
        {
            flashReturnStatus = FLASH_INVALID_CALLBACK_HANDLER;
        }
        else if(0U != NVMCONbits.WR)
        {
            flashReturnStatus = FLASH_OP_BUSY;
        }
        else
        {
            // Save the handler and context to call for completion of this operation.
            userCallbackHandler = callbackHandler;
            userContext = context;
            lock = true;
            NVMDATA0 = *data;
            NVMDATA1 = *(data + 1U);
            NVMDATA2 = *(data + 2U);
            NVMDATA3 = *(data + 3U);

            NVMADR = flashAddress;
            NVMCON = FLASH_WORD_WRITE_OPCODE;

            NVMCONbits.WR = 1U;
            lock = false;

        }
    }
    return flashReturnStatus;
}

enum FLASH_RETURN_STATUS FLASH_NonBlockingRowWrite(flash_adr_t flashAddress, flash_data_t *data, flash_key_t unlockKey, FLASH_CALLBACK callbackHandler, void* context)
{
    enum FLASH_RETURN_STATUS flashReturnStatus = FLASH_OP_IN_PROGRESS;
    if(false == lock)
    {
        if (NULL == data)
        {
            flashReturnStatus = FLASH_INVALID_DATA;
        }
        else if(unlockKey != (flash_key_t)FLASH_UNLOCK_KEY)
        {
            flashReturnStatus = FLASH_INVALID_KEY;
        }
        else if (0U != (flashAddress % FLASH_ADDRESS_MASK))
        {
            flashReturnStatus = FLASH_INVALID_ADDRESS;
        }
        else if(NULL == callbackHandler)
        {
            flashReturnStatus = FLASH_INVALID_CALLBACK_HANDLER;
        }
        else if(0U != NVMCONbits.WR)
        {
            flashReturnStatus = FLASH_OP_BUSY;
        }
        else
        {
            // Save the handler and context to call for completion of this operation.
            userCallbackHandler = callbackHandler;
            userContext = context;
            lock = true;
            NVMSRCADR = (uint32_t)data;
            NVMADR = flashAddress;
            NVMCON = FLASH_ROW_WRITE_OPCODE;

            NVMCONbits.WR = 1U;
            lock = false;
        }
    }
    return flashReturnStatus;
}


enum FLASH_RETURN_STATUS FLASH_NonBlockingRead(flash_adr_t flashAddress, size_t count, flash_data_t *data)
{
    flash_adr_t *flashReadAddress = (flash_adr_t *)flashAddress;
    enum FLASH_RETURN_STATUS flashReturnStatus = FLASH_OP_BUSY;

    if(false == lock)
    {
        if ((NULL == data) || (0U == count))
        {
            flashReturnStatus = FLASH_INVALID_DATA;
        }
        else if (0U != (flashAddress % FLASH_ADDRESS_MASK))
        {
            flashReturnStatus = FLASH_INVALID_ADDRESS;
        }
        else if(0U != NVMCONbits.WR)
        {
            flashReturnStatus = FLASH_OP_BUSY;
        }
        else
        {
            uint32_t index;
            lock = true;
            for(index = 0; index < count ; index++)
            {
                *data = *(flashReadAddress+index);
                data++;
            }

            flashReturnStatus = FLASH_NO_ERROR;

            // Now that the read is done, the lock can be cleared.
            lock = false;
        }
    }
    return flashReturnStatus;
}

uint16_t FLASH_ErasePageOffsetGet(flash_adr_t address)
{
    return ((~FLASH_ERASE_PAGE_MASK) & address);
}

uint32_t FLASH_ErasePageAddressGet(flash_adr_t address)
{
    return (FLASH_ERASE_PAGE_MASK & address);
}
