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
#include "stdio.h"
#include "mcc_generated_files/dee/dee.h"
#include "mcc_generated_files/system/system.h"
#include "mcc_generated_files/uart/uart2.h"

/*
    Main application
 */
#define NORMAL  "\x0F\x1B[0m\033[0m"                    // Cancels any active formatting modes and returns to a default font face
#define RED     "\033[0;31m"                            // Selects red font face
#define GREEN   "\033[0;32m"                            // Selects green font face
#define LOOPCOUNT 1200
#define DATA 0x12345678

unsigned long __attribute__((space(prog), address(0x83FFF0), keep)) fbtseq1 = 0xffffe001;
unsigned long __attribute__((space(prog), address(0xC3FFF0), keep)) fbtseq2 = 0xffffd002;


void __attribute__((address(__PROGRAM_BASE), keep, naked, noinline)) executionSwitch(void) {
    asm volatile("\n    mov.sl      #%0, w1"            
                 "\n    mov.l       [w1], w1"           // w1 = Inactive Partition FBTSEQ value (first word of last row)
                 "\n    mov.sl      #0xC00000, w0"
                 "\n    mov.l       [w0], w0"           // w0 = Inactive Partition reset vector destination (__reset label on Inactive Partition)
                 "\n    bootswp     w1"
                 "\n    goto        w0" 
                 : /* no outputs */ : "i"(0x400000ul | (__PROGRAM_BASE + __PROGRAM_LENGTH - 0x10ul)) : "w0", "w1");
}

int main(void) {
    int addr0 = 0, addr1 = 1, addr2 = 2, i;
    uint32_t valueAtAddr0 = 0;
    uint32_t valueAtAddr1 = 0;
    uint32_t valueAtAddr2 = 0;
    uint32_t expectedValueAtAddr0 = DATA+LOOPCOUNT;
    uint32_t expectedValueAtAddr1 = DATA+LOOPCOUNT+1;
    uint32_t expectedValueAtAddr2 = DATA+LOOPCOUNT+2;

    SYSTEM_Initialize();
    printf(NORMAL"Application in Partition1: Data EEPROM Emulation is in progress...\r\n");
    DEE_Init();

    for (i = 0; i < LOOPCOUNT; i++) {
        DEE_Write(addr0, DATA + i + 1);
        DEE_Write(addr1, DATA + i + 2);
        DEE_Write(addr2, DATA + i + 3);
    }

    DEE_Read(addr0, &valueAtAddr0);
    DEE_Read(addr1, &valueAtAddr1);
    DEE_Read(addr2, &valueAtAddr2);

    if (valueAtAddr0 == expectedValueAtAddr0 && valueAtAddr1 == expectedValueAtAddr1 && valueAtAddr2 == expectedValueAtAddr2) {
        printf(GREEN" Application in Partition1: Data EEPROM Emulation successful.\r\n");
        printf(NORMAL" Value at address 0 is 0x%X \r\n Value at address 1 is 0x%X \r\n Value at address 2 is 0x%X \r\n", valueAtAddr0, valueAtAddr1, valueAtAddr2);
    } else {
        printf(RED" Application in Partition1: Data EEPROM Emulation failed.\r\n");
        printf(NORMAL" Value at address 0 is 0x%X \r\n Value at address 1 is 0x%X \r\n Value at address 2 is 0x%X \r\n", valueAtAddr0, valueAtAddr1, valueAtAddr2);
    }
    Nop();
    Nop();
    Nop();
    Nop();
    
    executionSwitch();
    while(1);
    return 0;

}