/******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2016 STMicroelectronics</center></h2>
  *
  * Licensed under ST MYLIBERTY SOFTWARE LICENSE AGREEMENT (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/myliberty
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied,
  * AND SPECIFICALLY DISCLAIMING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
******************************************************************************/


/*
 *      PROJECT:   
 *      $Revision: $
 *      LANGUAGE:  ISO C99
 */

/*! \file rfal_platform.h
 *
 *  \author Gustavo Patricio
 *
 *  \brief Platform specific definition layer  
 *  
 *  This should contain all platform and hardware specifics such as 
 *  GPIO assignment, system resources, locks, IRQs, etc
 *  
 *  Each distinct platform/system/board must provide this definitions 
 *  for all SW layers to use
 *  
 */

#ifndef RFAL_PLATFORM_H
#define RFAL_PLATFORM_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <stdint.h>                                            /* Include type definitions                               */
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#include "st25r3916_spi.h"                                               /* Include any glue layer needed for interfacing with HAL */

/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/

#define ST25R_COM_SINGLETXRX                                   /*!< Use single Transceive                             */ 

#if 0
#define ST25R_SS_PIN                SPI1_CS_Pin                /*!< GPIO pin used for ST25R SPI SS                    */ 
#define ST25R_SS_PORT               SPI1_CS_GPIO_Port          /*!< GPIO port used for ST25RSPI SS port               */ 
                                                                   
#define ST25R_INT_PIN               IRQ_391x_Pin               /*!< GPIO pin used for ST25R External Interrupt        */
#define ST25R_INT_PORT              IRQ_391x_GPIO_Port         /*!< GPIO port used for ST25R External Interrupt       */


#ifdef LED_FIELD_Pin
#define PLATFORM_LED_FIELD_PIN      LED_FIELD_Pin              /*!< GPIO pin used as field LED                        */
#endif

#ifdef LED_FIELD_GPIO_Port
#define PLATFORM_LED_FIELD_PORT     LED_FIELD_GPIO_Port        /*!< GPIO port used as field LED                       */
#endif

#ifdef LED_RX_Pin
#define PLATFORM_LED_RX_PIN         LED_RX_Pin                 /*!< GPIO pin used as field LED                        */
#endif

#ifdef LED_RX_GPIO_Port
#define PLATFORM_LED_RX_PORT        LED_RX_GPIO_Port           /*!< GPIO port used as field LED                       */
#endif
#endif



/*
******************************************************************************
* GLOBAL MACROS
******************************************************************************
*/
#define platformProtectST25RComm()                    
#define platformUnprotectST25RComm()                    /*!< Unprotect unique access to ST25R communication channel - IRQ enable on a single thread environment (MCU) ; Mutex unlock on a multi thread environment */

#define platformProtectST25RIrqStatus()               platformProtectST25RComm()                               /*!< Protect unique access to IRQ status var - IRQ disable on single thread environment (MCU) ; Mutex lock on a multi thread environment */
#define platformUnprotectST25RIrqStatus()             platformUnprotectST25RComm()                             /*!< Unprotect the IRQ status var - IRQ enable on a single thread environment (MCU) ; Mutex unlock on a multi thread environment         */


#define platformLedOff( port, pin )                                      /*!< Turns the given LED Off                     */
#define platformLedOn( port, pin )                                             /*!< Turns the given LED On                      */
#define platformLedToggle( port, pin )                                    /*!< Toggles the given LED                       */

#define platformGpioSet( port, pin )                              /*!< Turns the given GPIO High                   */
#define platformGpioClear( port, pin )                          /*!< Turns the given GPIO Low                    */
#define platformGpioToggle( port, pin )                                       /*!< Toggles the given GPIO                      */
//#define platformGpioIsHigh( port, pin )                        /*!< Checks if the given LED is High             */
#define platformGpioIsLow( port, pin )                                       /*!< Checks if the given LED is Low              */

#define platformTimerCreate( t )                                                    /*!< Creates a timer with the given time (ms)    */
#define platformTimerIsExpired( timer )                                                   /*!< Checks if the given timer is expired        */
#define platformDelay( t )                                                                     /*!< Performs a delay for the given time (ms)    */

#define platformGetSysTick()                                                                  /*!< Get System Tick ( 1 tick = 1 ms)            */

#define platformAssert( exp )                                                              /*!< Asserts whether the given expression is true*/
#define platformErrorHandle()                                                              /*!< Global error handle/trap                    */


#define platformSpiSelect()                                   /*!< SPI SS/CS: Chip|Slave Select                */
#define platformSpiDeselect()                                   /*!< SPI SS/CS: Chip|Slave Deselect              */



#define platformI2CTx( txBuf, len, last, txOnly )                                                              /*!< I2C Transmit                                */
#define platformI2CRx( txBuf, len )                                                                            /*!< I2C Receive                                 */
#define platformI2CStart()                                                                                     /*!< I2C Start condition                         */
#define platformI2CStop()                                                                                      /*!< I2C Stop condition                          */
#define platformI2CRepeatStart()                                                                               /*!< I2C Repeat Start                            */
#define platformI2CSlaveAddrWR(add)                                                                            /*!< I2C Slave address for Write operation       */
#define platformI2CSlaveAddrRD(add)                                                                            /*!< I2C Slave address for Read operation        */

#define platformLog(...)                                                                                       /*!< Log  method                                 */

 /*
 ******************************************************************************
 * RFAL OPTIONAL MACROS
 ******************************************************************************
 */

#ifndef platformProtectST25RIrqStatus
    #define platformProtectST25RIrqStatus()            /*!< Protect unique access to IRQ status var - IRQ disable on single thread environment (MCU) ; Mutex lock on a multi thread environment */
#endif /* platformProtectST25RIrqStatus */

#ifndef platformUnprotectST25RIrqStatus
    #define platformUnprotectST25RIrqStatus()          /*!< Unprotect the IRQ status var - IRQ enable on a single thread environment (MCU) ; Mutex unlock on a multi thread environment         */
#endif /* platformUnprotectST25RIrqStatus */

#ifndef platformProtectWorker
    #define platformProtectWorker()                    /*!< Protect RFAL Worker/Task/Process from concurrent execution on multi thread platforms   */
#endif /* platformProtectWorker */

#ifndef platformUnprotectWorker
    #define platformUnprotectWorker()                  /*!< Unprotect RFAL Worker/Task/Process from concurrent execution on multi thread platforms */
#endif /* platformUnprotectWorker */

//#ifndef platformIrqST25RPinInitialize
//    #define platformIrqST25RPinInitialize()            /*!< Initializes ST25R IRQ pin                     */
//#endif /* platformIrqST25RPinInitialize */                                                                

#ifndef platformIrqST25RSetCallback                                                                       
    #define platformIrqST25RSetCallback( cb )          /*!< Sets ST25R ISR callback                       */
#endif /* platformIrqST25RSetCallback */                                                                  

#ifndef platformLedsInitialize                                                                            
    #define platformLedsInitialize()                   /*!< Initializes the pins used as LEDs to outputs  */
#endif /* platformLedsInitialize */                                                                       

#ifndef platformLedOff                                                                                    
    #define platformLedOff( port, pin )                /*!< Turns the given LED Off                       */
#endif /* platformLedOff */                                                                               

#ifndef platformLedOn                                                                                     
    #define platformLedOn( port, pin )                 /*!< Turns the given LED On                        */
#endif /* platformLedOn */                                                                                
                                                                                                          
#ifndef platformLedToggle                                                                                 
    #define platformLedToggle( port, pin )             /*!< Toggles the given LED                         */
#endif /* platformLedToggle */                                                                            


#ifndef platformGetSysTick                                                                                
    #define platformGetSysTick()                       /*!< Get System Tick ( 1 tick = 1 ms)              */
#endif /* platformGetSysTick */                                                                           

#ifndef platformTimerDestroy                                                                              
    #define platformTimerDestroy( timer )              /*!< Stops and released the given timer            */
#endif /* platformTimerDestroy */                                                                         

#ifndef platformLog                                                                                       
    #define platformLog(...)                           /*!< Log method                                    */
#endif /* platformLog */

#ifndef platformAssert                                                                             
    #define platformAssert( exp )                      /*!< Asserts whether the given expression is true */
#endif /* platformAssert */

#ifndef platformErrorHandle                                                                             
    #define platformErrorHandle()                      /*!< Global error handler or trap                 */
#endif /* platformErrorHandle */

#define platformSpiTxRx( txBuf, rxBuf, len )          st25r3916_spiTxRx(txBuf, rxBuf, len)                               /*!< SPI transceive                              */

/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/
extern uint8_t globalCommProtectCnt;                                           /*!< Global Protection Counter provided per platform - instantiated in main.c    */





/*
******************************************************************************
* USER SPECIFIC RFAL CONFIGURATION
******************************************************************************
*/

#define RFAL_FEATURE_LISTEN_MODE               true       /*!< Enable/Disable RFAL support for Listen Mode                               */
#define RFAL_FEATURE_WAKEUP_MODE               true       /*!< Enable/Disable RFAL support for the Wake-Up mode                          */
#define RFAL_FEATURE_LOWPOWER_MODE             true       /*!< Enable/Disable RFAL support for the Low Power mode                        */
#define RFAL_FEATURE_NFCA                      true       /*!< Enable/Disable RFAL support for NFC-A (ISO14443A)                         */
#define RFAL_FEATURE_NFCB                      true       /*!< Enable/Disable RFAL support for NFC-B (ISO14443B)                         */
#define RFAL_FEATURE_NFCF                      true       /*!< Enable/Disable RFAL support for NFC-F (FeliCa)                            */
#define RFAL_FEATURE_NFCV                      true       /*!< Enable/Disable RFAL support for NFC-V (ISO15693)                          */
#define RFAL_FEATURE_T1T                       true       /*!< Enable/Disable RFAL support for T1T (Topaz)                               */
#define RFAL_FEATURE_T2T                       true       /*!< Enable/Disable RFAL support for T2T                                       */
#define RFAL_FEATURE_T4T                       true       /*!< Enable/Disable RFAL support for T4T                                       */
#define RFAL_FEATURE_ST25TB                    true       /*!< Enable/Disable RFAL support for ST25TB                                    */
#define RFAL_FEATURE_ST25xV                    true       /*!< Enable/Disable RFAL support for ST25TV/ST25DV                             */
#define RFAL_FEATURE_DYNAMIC_ANALOG_CONFIG     false      /*!< Enable/Disable Analog Configs to be dynamically updated (RAM)             */
#define RFAL_FEATURE_DPO                       false      /*!< Enable/Disable RFAL Dynamic Power Output support                          */
#define RFAL_FEATURE_ISO_DEP                   true       /*!< Enable/Disable RFAL support for ISO-DEP (ISO14443-4)                      */
#define RFAL_FEATURE_ISO_DEP_POLL              true       /*!< Enable/Disable RFAL support for Poller mode (PCD) ISO-DEP (ISO14443-4)    */
#define RFAL_FEATURE_ISO_DEP_LISTEN            true       /*!< Enable/Disable RFAL support for Listen mode (PICC) ISO-DEP (ISO14443-4)   */
#define RFAL_FEATURE_NFC_DEP                   true       /*!< Enable/Disable RFAL support for NFC-DEP (NFCIP1/P2P)                      */


#define RFAL_FEATURE_ISO_DEP_IBLOCK_MAX_LEN    256U       /*!< ISO-DEP I-Block max length. Please use values as defined by rfalIsoDepFSx */
#define RFAL_FEATURE_NFC_DEP_BLOCK_MAX_LEN     254U       /*!< NFC-DEP Block/Payload length. Allowed values: 64, 128, 192, 254           */
#define RFAL_FEATURE_NFC_RF_BUF_LEN            256U       /*!< RF buffer length used by RFAL NFC layer                                   */

#define RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN      512U       /*!< ISO-DEP APDU max length.                                                  */
#define RFAL_FEATURE_NFC_DEP_PDU_MAX_LEN       512U       /*!< NFC-DEP PDU max length.                                                   */

/*
******************************************************************************
* DEFAULT RFAL CONFIGURATION
******************************************************************************
*/
//#include "rfal_defConfig.h"

#endif /* RFAL_PLATFORM_H */


