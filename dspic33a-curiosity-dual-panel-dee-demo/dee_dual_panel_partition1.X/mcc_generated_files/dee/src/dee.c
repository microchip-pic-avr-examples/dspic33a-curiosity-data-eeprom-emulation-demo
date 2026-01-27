/*
© [2026] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip 
    software and any derivatives exclusively with Microchip products. 
    You are responsible for complying with 3rd party license terms  
    applicable to your use of 3rd party software (including open source  
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
    THIS SOFTWARE.
*/

#include <stdlib.h>
#include <xc.h>
#include "../../flash/flash_nonblocking.h"
#include "../dee.h"
#include "../dee_types.h"
#include "../dee_dual_panel.h"

#define DEE_PAGE_ADDRESS(bank, page)      ((DEE_START_ADDRESS_INACTIVE_PARTITION + (DEE_BANK_SIZE * (bank)) + (DEE_PAGE_SIZE * (page))) & 0xFFFFFF)

#define GetAddrNotFound()                 dataEEFlags.addrNotFound
#define SetAddrNotFound(x)                dataEEFlags.addrNotFound = x
#define GetPageCorruptStatus()            dataEEFlags.pageCorrupt
#define SetPageCorruptStatus(x)           dataEEFlags.pageCorrupt = x
#define GetPageExpiredPage()              dataEEFlags.expiredPage
#define SetPageExpiredPage(x)             dataEEFlags.expiredPage = x
#define GetPageIllegalAddress()           dataEEFlags.IllegalAddress
#define SetPageIllegalAddress(x)          dataEEFlags.IllegalAddress = x
#define GetPagePackBeforeInit()           dataEEFlags.packBeforeInit
#define SetPagePackBeforeInit(x)          dataEEFlags.packBeforeInit = x
#define GetPagePackBeforePageFull()       dataEEFlags.packBeforePageFull
#define SetPagePackBeforePageFull(x)      dataEEFlags.packBeforePageFull = x
#define GetPagePackSkipped()              dataEEFlags.packSkipped
#define SetPagePackSkipped(x)             dataEEFlags.packSkipped = x
#define GetPageWriteError()               dataEEFlags.writeError
#define SetPageWriteError(x)              dataEEFlags.writeError = x

DATA_EE_FLAGS dataEEFlags;

static uint8_t                  DEE_GetPageStatus(uint8_t bank, uint8_t page, uint8_t field);
static uint16_t                 DEE_GetNextAvailCount(uint8_t bank);
static DEE_RETURN_STATUS        DEE_Pack(uint8_t bank);
static void                     DEE_FlashPageErase(uint8_t bank, uint8_t page);
static void                     DEE_FlashWrite(flash_adr_t flashAddress, flash_data_t *data);
static void                     DEE_FlashRead(flash_adr_t flashAddress, size_t count, flash_data_t *data);
static DEE_RETURN_STATUS        DEE_Initialize(void);

static void FlashEraseOpHandler(void* context) {
    // Doing nothing
}

static void FlashProgramOpHandler(void *context) {
    // Doing nothing
}

DEE_RETURN_STATUS DEE_Init(void)
{
    DEE_RETURN_STATUS deeReturnStatus =  DEE_NO_ERROR;
    deeReturnStatus = DEE_ActivePartitionInit();
    if(deeReturnStatus == DEE_NO_ERROR)
    {
        bool dataCopied =  DEE_ActiveToInactivePartitionDataCopy();
        if(!dataCopied)
        {
            deeReturnStatus = DEE_Initialize();
        }
    }
    return deeReturnStatus;
}

DEE_RETURN_STATUS DEE_Initialize(void)
{
    uint8_t pageCnt;
    uint16_t erasePage;
    uint16_t currentPage;
    uint32_t pageAddress;
    uint8_t bank;
    uint32_t statusWord[4];

    dataEEFlags.val = 0;
    for (bank = 0; bank < DATA_EE_BANKS; bank++)
    {
        pageCnt = 0;
        erasePage = 0;

        // Find unexpired page
        for (currentPage = 0;
                (currentPage < NUM_DATA_EE_PAGES) &&
                (DEE_GetPageStatus(bank, currentPage, STATUS_EXPIRED) == PAGE_EXPIRED);
                currentPage++)
        {
        }

        if (currentPage == NUM_DATA_EE_PAGES)
        {
            SetPageExpiredPage(1);
            return (DEE_EXPIRED_PAGE); // Error - All pages expired
        }
        // Count active page(s).
        for (currentPage = 0; currentPage < NUM_DATA_EE_PAGES; currentPage++)
        {
            if (DEE_GetPageStatus(bank, currentPage, STATUS_CURRENT) == PAGE_CURRENT)
            {
                pageCnt++;
            }
        }
        //If no active pages found, initialize page 0
        if (pageCnt == 0)
        {
            DEE_FlashPageErase(bank, 0);
            pageAddress = DEE_PAGE_ADDRESS(bank, 0);
            //New page: unavailable, active, reset count
            statusWord[0] = 0xF3;
            statusWord[0] = statusWord[0] << 16;
            statusWord[1] = ERASE_STATE;
            statusWord[2] = ERASE_STATE;
            statusWord[3] = ERASE_STATE;
            DEE_FlashWrite(pageAddress, statusWord);
            continue;
        }
        //If one active page, do nothing
        else if (pageCnt == 1)
        {
            continue;
        }
        //If two active pages, erase second and repack first
        else if (pageCnt == 2)
        {
            if ((DEE_GetPageStatus(bank, NUM_DATA_EE_PAGES - 1, STATUS_CURRENT) == PAGE_CURRENT) &&
                    (DEE_GetPageStatus(bank, 0, STATUS_CURRENT) == PAGE_CURRENT))
            {
                currentPage = NUM_DATA_EE_PAGES - 1;
                erasePage = 0;
            }
            else
            {
                currentPage = 0;
                while ((DEE_GetPageStatus(bank, currentPage, STATUS_CURRENT) == PAGE_NOT_CURRENT) &&
                        (currentPage < NUM_DATA_EE_PAGES))
                {
                    currentPage++;
                }
                erasePage = currentPage + 1;
                if (erasePage == NUM_DATA_EE_PAGES)
                {
                    erasePage = 0;
                }
            }
            DEE_FlashPageErase(bank, erasePage);
            if (!DEE_GetNextAvailCount(bank))
            {
                DEE_Pack(bank);
            }
            continue;
        }
        else
        {
            SetPageCorruptStatus(1);
            return (DEE_PAGE_CORRUPT);
        }
    }
    return (DEE_NO_ERROR);
}

DEE_RETURN_STATUS DEE_Read(uint16_t addr, uint32_t* dataRead)
{
    uint16_t currentPage;
    uint32_t pageAddress;
    uint8_t latchAddr;
    uint16_t i;
    uint8_t bank;
    uint32_t latchAddrData[4];

    if (addr >= DATA_EE_TOTAL_SIZE)
    {
        SetPageIllegalAddress(1);
        return (DEE_ILLEGAL_ADDRESS);
    }

    bank = addr / DATA_EE_SIZE;

    // Find the active page.
    for (currentPage = 0;
            (currentPage < NUM_DATA_EE_PAGES) &&
            (DEE_GetPageStatus(bank, currentPage, STATUS_CURRENT) == PAGE_NOT_CURRENT);
            currentPage++)
    {
    }

    if (currentPage == NUM_DATA_EE_PAGES)
    {
        SetPageCorruptStatus(1);
        return (DEE_PAGE_CORRUPT); // Error - no active page
    }

    pageAddress = DEE_PAGE_ADDRESS(bank, (currentPage + 1)) - ADDRESS_INC_FACTOR;


    i = NUMBER_OF_INSTRUCTIONS_IN_PAGE*4;

    do
    {
        DEE_FlashRead(pageAddress, 2, &latchAddrData[0]);
        latchAddr = (uint8_t)(latchAddrData[0] & 0xFF);
        *dataRead = latchAddrData[1];
        pageAddress -= ADDRESS_INC_FACTOR; // Go to the next/prev QUADWORD address
        i -= ADDRESS_INC_FACTOR;
    }
    while ((i > 0) && (latchAddr != (addr % DATA_EE_SIZE)));

    if (!i)
    {
        SetAddrNotFound(1);
        return (DEE_ILLEGAL_ADDRESS);
    }


    return (DEE_NO_ERROR);
}

DEE_RETURN_STATUS DEE_Write(uint16_t addr, uint32_t data)
{
    DEE_RETURN_STATUS status = DEE_NO_ERROR;
    uint16_t currentPage;
    uint32_t pageAddress; //Current array (page) offset of selected element (PM 16-bit word)
    uint16_t nextLoc;
    volatile uint32_t latchData;
    volatile uint8_t latchAddr;
    uint8_t dataEEFlags_sh;
    uint16_t bank;
    uint16_t pageToBeErased;
    uint32_t deeItem[4];

    if (addr >= DATA_EE_TOTAL_SIZE)
    {
        SetPageIllegalAddress(1);
        return (DEE_ILLEGAL_ADDRESS);
    }

    bank = addr / DATA_EE_SIZE;

    // Find the active page.
    for (currentPage = 0;
            (currentPage < NUM_DATA_EE_PAGES) &&
            (DEE_GetPageStatus(bank, currentPage, STATUS_CURRENT) == PAGE_NOT_CURRENT);
            currentPage++)
    {
    }

    if (currentPage == NUM_DATA_EE_PAGES)
    {
        SetPageCorruptStatus(1);
        return (DEE_PAGE_CORRUPT); // Error - no active page
    }

    pageAddress = DEE_PAGE_ADDRESS(bank, currentPage);

    nextLoc = DEE_GetNextAvailCount(bank);

    if (!nextLoc)
    {
        pageToBeErased = currentPage + 1;
        if(pageToBeErased == NUM_DATA_EE_PAGES)
        {
            pageToBeErased = 0;
        }
        DEE_FlashPageErase(bank, pageToBeErased);
        DEE_Pack(bank);

        for (currentPage = 0;
            (currentPage < NUM_DATA_EE_PAGES) &&
            (DEE_GetPageStatus(bank, currentPage, STATUS_CURRENT) == PAGE_NOT_CURRENT);
            currentPage++)
        {
        }

        if (currentPage == NUM_DATA_EE_PAGES)
        {
            SetPageCorruptStatus(1);
            return (DEE_PAGE_CORRUPT); // Error - no active page
        }

        pageAddress = DEE_PAGE_ADDRESS(bank, currentPage);
        nextLoc = DEE_GetNextAvailCount(bank);
    }

    dataEEFlags_sh = dataEEFlags.val;
    //Do not write data if it did not change
    uint32_t dataRead;
    DEE_Read(addr, &dataRead);
    if (dataRead == data)
    {
        if (GetAddrNotFound() == 0) // Check if the read was successful
        {
            dataEEFlags.val = dataEEFlags_sh;
            return (DEE_NO_ERROR);
        }
    }
    dataEEFlags.val = dataEEFlags_sh; //Restore status flags

    pageAddress = pageAddress + nextLoc;

    deeItem[0] = addr % DATA_EE_SIZE;
    deeItem[1] = data;
    deeItem[2] = ERASE_STATE;
    deeItem[3] = ERASE_STATE;
    DEE_FlashWrite(pageAddress, deeItem);

    Nop();
    Nop();

    DEE_FlashRead(pageAddress, 2, &deeItem[0]);
    latchData = deeItem[1];

    if (latchData != data)
    {
        SetPageWriteError(1);
        return (DEE_WRITE_ERROR); //Error - RAM does not match PM
    }

    latchAddr = deeItem[0];

    if (latchAddr != (addr % DATA_EE_SIZE))
    {
        SetPageWriteError(1);
        return (DEE_WRITE_ERROR); //Error - RAM does not match PM
    }

    //Pack if page is full
    if ((nextLoc + ADDRESS_INC_FACTOR) == ((NUMBER_OF_INSTRUCTIONS_IN_PAGE * FLASH_WORD_WRITE_SIZE)))
    {
        status = DEE_Pack(bank);
    }

    return (status);
}

DEE_RETURN_STATUS DEE_Pack(uint8_t bank)
{
    uint16_t currentPage; //Array row (PM page) of active DEE page
    uint16_t packedPage; //Array row (PM page) of packed page
    uint32_t currentPageAddress; //Current page address
    uint32_t packedPageAddress; //Packed page address
    uint8_t latchAddr;
    uint32_t latchData;
    uint8_t dataEEFlags_sh;
    uint32_t deeItem[4];
    uint32_t data[4];

    // Find the active page.
    for (currentPage = 0;
            (currentPage < NUM_DATA_EE_PAGES) &&
            (DEE_GetPageStatus(bank, currentPage, STATUS_CURRENT) == PAGE_NOT_CURRENT);
            currentPage++)
    {
    }


    if (currentPage == NUM_DATA_EE_PAGES)
    {
        SetPagePackBeforeInit(1);
        return (DEE_PACK_BEFORE_INIT); // Error - no active page
    }
    else
    {
        // Find the next unexpired page to use
        packedPage = currentPage + 1;
        if (packedPage == NUM_DATA_EE_PAGES)
        {
            packedPage = 0;
        }
        while (DEE_GetPageStatus(bank, packedPage, STATUS_EXPIRED) == PAGE_EXPIRED)
        {
            packedPage++;
            if (packedPage == NUM_DATA_EE_PAGES)
            {
                packedPage = 0;
            }
            if (packedPage == currentPage)
            {
                SetPageExpiredPage(1);
                return (DEE_EXPIRED_PAGE); // Error - all pages expired
            }
        }
    }

    packedPageAddress = DEE_PAGE_ADDRESS(bank, packedPage);

    if (DEE_GetNextAvailCount(bank))
    {
        SetPagePackBeforePageFull(1); // Pack called before the page was full
    }

    dataEEFlags_sh = dataEEFlags.val;
    SetAddrNotFound(0); // Initialize flag


    deeItem[0] = ERASE_STATE;
    deeItem[1] = ERASE_STATE;
    deeItem[2] = ERASE_STATE;
    deeItem[3] = ERASE_STATE;
    DEE_FlashWrite(packedPageAddress, deeItem);

    packedPageAddress += ADDRESS_INC_FACTOR;
    latchAddr = 0;

    do
    {
        DEE_Read((DATA_EE_SIZE * bank) + latchAddr, &latchData);
        if (GetAddrNotFound()) //if address is unwritten, skip to next address
        {
            SetAddrNotFound(0);
        }
        else
        {
            deeItem[0] = latchAddr;
            deeItem[1] = latchData;
            deeItem[2] = ERASE_STATE;
            deeItem[3] = ERASE_STATE;
            DEE_FlashWrite(packedPageAddress, deeItem);

            packedPageAddress += ADDRESS_INC_FACTOR;
        }
        latchAddr++;
    }
    while (latchAddr != DATA_EE_SIZE);

    dataEEFlags.val = dataEEFlags_sh; //Restore status flags

    packedPageAddress = DEE_PAGE_ADDRESS(bank, packedPage);

    DEE_FlashRead(packedPageAddress, 4, &data[0]);
    latchAddr = (uint8_t)(data[0] & 0xFF);
    latchData = data[1];
    packedPageAddress += ADDRESS_INC_FACTOR; // Do not use odd Flash words on devices with ECC or double word writes

    while (latchAddr != 0xFF)
    {
        uint32_t dataRead;
        DEE_Read((DATA_EE_SIZE * bank) + latchAddr, &dataRead);
        if (dataRead != latchData)
        {
            SetPageWriteError(1);
            return (DEE_WRITE_ERROR); //Error - data does not match
        }
        DEE_FlashRead(packedPageAddress, 2, &data[0]);
        latchAddr = (uint8_t)(data[0] & 0xFF);
        latchData = data[1];
        packedPageAddress += ADDRESS_INC_FACTOR;
    }

    //Program page status
    currentPageAddress = DEE_PAGE_ADDRESS(bank, currentPage);
    packedPageAddress = DEE_PAGE_ADDRESS(bank, packedPage);

    DEE_FlashRead(currentPageAddress, 2, &data[0]);
    latchAddr = (data[0] >> 16) & 0xFF;
    latchData = data[0] & 0xFFFF;

    if (packedPage == 0)
    {
        latchData++; //Increment E/W counter
    }

    if (latchData >= ERASE_WRITE_CYCLE_MAX - 1)
    {
        SetPageExpiredPage(1);
        latchAddr &= 0xEF;
    }

    uint32_t dataItem[4];
    dataItem[0] = latchAddr;
    dataItem[0] = (dataItem[0] << 16) | latchData;
    dataItem[1] = ERASE_STATE;
    dataItem[2] = ERASE_STATE;
    dataItem[3] = ERASE_STATE;
    DEE_FlashWrite(packedPageAddress, dataItem);
    DEE_FlashRead(packedPageAddress, 2, &data[0]);
    uint16_t readAddr =(data[0] >> 16) & 0xFF;
    uint32_t readData = data[0] & 0xFFFF;

    if ((latchAddr != readAddr) ||
            (latchData != readData))
    {
        SetPageWriteError(1);
        return (DEE_WRITE_ERROR);
    }

    //Erase active page
    DEE_FlashPageErase(bank, currentPage);

    return (GetPageExpiredPage());
}

static uint8_t DEE_GetPageStatus(uint8_t bank, uint8_t page, uint8_t field)
{
    uint32_t pageAddress;
    uint8_t statusByte;
    uint8_t status;
    uint32_t statusInfo;

    pageAddress = DEE_PAGE_ADDRESS(bank, page);
    DEE_FlashRead(pageAddress, 1, &statusInfo);
    statusByte = statusInfo >> 16 & 0xFF;
    switch (field)
    {
        case STATUS_AVAILABLE:
            status = ((statusByte & 4) >> 2);
            break;
        case STATUS_CURRENT:
            status = ((statusByte & 8) >> 3);
            break;
        case STATUS_EXPIRED:
            status = ((statusByte & 16) >> 4);
            break;
        default:
            status = 0;
            break;
    }
    return (status);
}

static void DEE_FlashPageErase(uint8_t bank, uint8_t page)
{
    uint32_t pageAddress = DEE_PAGE_ADDRESS(bank, page);
    FLASH_NonBlockingPageErase(pageAddress, FLASH_UNLOCK_KEY, FlashEraseOpHandler, NULL);
    while(FLASH_OperationStatusGet() == FLASH_OP_BUSY);
    return;
}

static void DEE_FlashWrite(flash_adr_t pageAddress, uint32_t *data)
{
    FLASH_NonBlockingWordWrite(pageAddress, data, FLASH_UNLOCK_KEY, FlashProgramOpHandler, NULL);
    while(FLASH_OperationStatusGet() == FLASH_OP_BUSY);
}

static void DEE_FlashRead(flash_adr_t pageAddress, size_t count, uint32_t *data)
{
    while(FLASH_NonBlockingRead(pageAddress, count, data) == FLASH_OP_BUSY);
}

static uint16_t DEE_GetNextAvailCount(uint8_t bank)
{
    uint16_t nextAvailCount = 0;
    uint16_t currentPage;
    uint32_t dataEEval;
    uint32_t pageAddress;
    uint32_t pageStartAddress;
    uint32_t nextPageAddress;
    uint32_t latchAddrData[2];

    // Find the active page.
    for (currentPage = 0;
            (currentPage < NUM_DATA_EE_PAGES) &&
            (DEE_GetPageStatus(bank, currentPage, STATUS_CURRENT) == PAGE_NOT_CURRENT);
            currentPage++)
    {
    }

    pageAddress = DEE_PAGE_ADDRESS(bank, currentPage);
    pageStartAddress = pageAddress;
    nextPageAddress = DEE_PAGE_ADDRESS(bank, currentPage+1);

    do
    {
        nextAvailCount += ADDRESS_INC_FACTOR;
        pageAddress += ADDRESS_INC_FACTOR;
        // Boundary check to make sure at the loop end page address does not fall into next page
        if (pageAddress == nextPageAddress) {
            pageAddress = pageStartAddress;
        }
        DEE_FlashRead(pageAddress, 2, &latchAddrData[0]);
        dataEEval = latchAddrData[1] & ERASE_STATE;
    }
    while ((nextAvailCount < (NUMBER_OF_INSTRUCTIONS_IN_PAGE * FLASH_WORD_WRITE_SIZE)) && (dataEEval != ERASE_STATE));

    if (nextAvailCount == (NUMBER_OF_INSTRUCTIONS_IN_PAGE * FLASH_WORD_WRITE_SIZE))
    {
        nextAvailCount = 0;
    }
    return (nextAvailCount);
}
