
/**
 * FLASH Generated Driver Header File
 *
 * @file         flash.h
 *
 * @ingroup      flashdriver
 *
 * @brief        FLASH driver using dsPIC MCUs
 *
 * @skipline @version   Firmware Driver Version 1.0.0
 *
 * @skipline @version   PLIB Version 1.0.4
 *
 * @skipline     Device : dsPIC33AK512MPS512
*/

/*disclaimer*/

#ifndef FLASH_NONBLOCKING_H
#define FLASH_NONBLOCKING_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "xc.h"

#include "flash_interface.h"

// Section: Data Type Definitions

/**
 @ingroup   flashdriver
 @brief     Structure object of type FLASH_INTERFACE assigned with name
            displayed in the Melody Driver User interface. A structure pointer can be used to achieve portability
            across the FLASH having same interface structure.
*/

extern const struct FLASH_INTERFACE flash;

// Section: Data Type Definitions

/**
 * @ingroup flashdriver
 * @brief This function initializes the nvm controller.
 * @return  void
 */
void FLASH_Initialize(void);

/**
 * @ingroup flashdriver
 * @brief This function gets the status of nvm controller.
 * @return  returns the status of the previous operation.
 */
enum FLASH_RETURN_STATUS FLASH_OperationStatusGet(void);

/**
 * @ingroup flashdriver
 * @brief This function erases a page.
 * @param[in]  flashAddress : Flash address
 * @param[in]  unlockKey : Flash unlock key
 * @param[in]  callbackHandler : Pointer to call back handler
 * @param[in]  context : Optional pointer to context to handle user contexts
 * @return  FLASH_RETURN_STATUS: returns FLASH_NO_ERROR if operation is successful, if the operation is in progress then it returns FLASH_OP_IN_PROGRESS,
 *          if there is a previous operation in progress then it returns FLASH_OP_BUSY , in case of any unsuccessful operation  it returns errors like
 *          FLASH_INVALID_ADDRESS, FLASH_INVALID_CALLBACK_HANDLER, FLASH_WRITE_ERROR
 */
enum FLASH_RETURN_STATUS FLASH_NonBlockingPageErase(flash_adr_t flashAddress, flash_key_t unlockKey, FLASH_CALLBACK callbackHandler, void* context);

/**
 * @ingroup flashdriver
 * @brief This function erases a panel.
 * @param[in]  panel : Flash panel to erase
 * @param[in]  unlockKey : Flash unlock key
 * @return  * @return  returns FLASH_NO_ERROR if operation is successful, if the operation is in progress then it returns FLASH_OP_IN_PROGRESS,
 *          if there is a previous operation in progress then it returns FLASH_OP_BUSY , in case of any unsuccessful operation  it returns errors like
 *          FLASH_INVALID_ADDRESS, FLASH_INVALID_CALLBACK_HANDLER, FLASH_WRITE_ERROR
 */
enum FLASH_RETURN_STATUS FLASH_NonBlockingBulkErase(enum FLASH_PANEL panel, flash_key_t unlockKey, FLASH_CALLBACK callbackHandler, void* context);


/**
 * @ingroup flashdriver
 * @brief This function writes the specified data to the specified address.
 * @param[in]  flashAddress : Flash address
 * @param[in]  data : Pointer of the data to be written.
 * @param[in]  unlockKey : Flash unlock Key.
 * @param[in]  callbackHandler : pointer to call back handler
 * @param[in]  context : Optional pointer to context to handle user contexts
 * @return  FLASH_RETURN_STATUS: returns FLASH_NO_ERROR if operation is successful, if the operation is in progress then it returns FLASH_OP_IN_PROGRESS,
 *          if there is a previous operation in progress then it returns FLASH_OP_BUSY , in case of any unsuccessful operation  it returns errors like
 *          FLASH_INVALID_ADDRESS, FLASH_INVALID_CALLBACK_HANDLER, FLASH_WRITE_ERROR
 */
enum FLASH_RETURN_STATUS FLASH_NonBlockingWordWrite(flash_adr_t flashAddress, flash_data_t *data, flash_key_t unlockKey, FLASH_CALLBACK callbackHandler, void* context);

/**
 * @ingroup flashdriver
 * @brief This function writes the specified data to the specified address.
 * @param[in]  flashAddress : Flash address
 * @param[in]  data : Pointer of the data to be written
 * @param[in]  unlockKey : Flash unlock Key.
 * @param[in]  callbackHandler : Pointer to call back handler
 * @param[in]  context : Optional pointer to context to handle user contexts
 * @return  FLASH_RETURN_STATUS: returns FLASH_NO_ERROR if operation is successful, if the operation is in progress then it returns FLASH_OP_IN_PROGRESS,
 *          if there is a previous operation in progress then it returns FLASH_OP_BUSY , in case of any unsuccessful operation  it returns errors like
 *          FLASH_INVALID_ADDRESS, FLASH_INVALID_CALLBACK_HANDLER, FLASH_WRITE_ERROR
 */
enum FLASH_RETURN_STATUS FLASH_NonBlockingRowWrite(flash_adr_t flashAddress, flash_data_t *data, flash_key_t unlockKey, FLASH_CALLBACK callback, void* context);

/**
 * @ingroup flashdriver
 * @brief This function reads the data from the specified address.
 * @param[in]  flashAddress : Flash address of
 * @param[out]  data : Pointer to read the data
 * @return  FLASH_RETURN_STATUS: returns FLASH_NO_ERROR if operation is successful, if the operation is in progress then it returns FLASH_OP_IN_PROGRESS,
 *          if there is a previous operation in progress then it returns FLASH_OP_BUSY , in case of any unsuccessful operation  it returns errors like
 *          FLASH_INVALID_ADDRESS, FLASH_INVALID_DATA
 */
enum FLASH_RETURN_STATUS FLASH_NonBlockingRead(flash_adr_t flashAddress, size_t count, flash_data_t *data);

/**
 * @ingroup flashdriver
 * @brief This function returns the offest from page start.
 * @param[in]  flashAddress : Flash address
 * @return  FLASH_RETURN_STATUS: returns the flash offset from page start address
 */
uint16_t FLASH_ErasePageOffsetGet(flash_adr_t flashAddress);


/**
 * @ingroup flashdriver
 * @brief This function returns the offest from page start.
 * @param[in]  flashAddress : Flash address
 * @return  FLASH_RETURN_STATUS: returns page start address for the given address
 */
uint32_t FLASH_ErasePageAddressGet(flash_adr_t flashAddress);

#endif    /* FLASH_NONBLOCKING_H */
