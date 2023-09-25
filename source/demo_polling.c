/******************************************************************************
  * @attention
  *
  * COPYRIGHT 2019 STMicroelectronics, all rights reserved
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


/*! \file
 *
 *  \author 
 *
 *  \brief Demo application
 *
 *  This demo shows how to poll for several types of NFC cards/devices and how 
 *  to exchange data with these devices, using the RFAL library.
 *
 *  This demo does not fully implement the activities according to the standards,
 *  it performs the required to communicate with a card/device and retrieve 
 *  its UID. Also blocking methods are used for data exchange which may lead to
 *  long periods of blocking CPU/MCU.
 *  For standard compliant example please refer to the Examples provided
 *  with the RFAL library.
 * 
 */
 
/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <zephyr/kernel.h>
#include <stdbool.h>
#include <assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include "demo.h"
#include "utils.h"
#include "rfal_nfc.h"
#include "logger.h"
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
LOG_MODULE_DECLARE(st25r3916);
#if RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE
#include "demo_ce.h"
#endif /* RFAL_FEATURE_LISTEN_MODE */
#define platformLog LOG_INF
/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/

/* Definition of possible states the demo state machine could have */
#define DEMO_ST_NOTINIT               0     /*!< Demo State:  Not initialized        */
#define DEMO_ST_START_DISCOVERY       1     /*!< Demo State:  Start Discovery        */
#define DEMO_ST_DISCOVERY             2     /*!< Demo State:  Discovery              */

#define DEMO_NFCV_BLOCK_LEN           4     /*!< NFCV Block len                      */

#define DEMO_NFCV_USE_SELECT_MODE     false /*!< NFCV demonstrate select mode        */
#define DEMO_NFCV_WRITE_TAG           false /*!< NFCV demonstrate Write Single Block */

/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */

/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */

/* P2P communication data */
static uint8_t NFCID3[] = {0x01, 0xFE, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
static uint8_t GB[] = {0x46, 0x66, 0x6d, 0x01, 0x01, 0x11, 0x02, 0x02, 0x07, 0x80, 0x03, 0x02, 0x00, 0x03, 0x04, 0x01, 0x32, 0x07, 0x01, 0x03};
    
/* APDUs communication data */    
#if RFAL_FEATURE_ISO_DEP_POLL
static uint8_t ndefSelectApp[] = { 0x00, 0xA4, 0x04, 0x00, 0x07, 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01, 0x00 };
static uint8_t ccSelectFile[] = { 0x00, 0xA4, 0x00, 0x0C, 0x02, 0xE1, 0x03};
static uint8_t readBinary[] = { 0x00, 0xB0, 0x00, 0x00, 0x0F };

/* For a Payment application a Select PPSE would be needed: 
   ppseSelectApp[] = { 0x00, 0xA4, 0x04, 0x00, 0x0E, 0x32, 0x50, 0x41, 0x59, 0x2E, 0x53, 0x59, 0x53, 0x2E, 0x44, 0x44, 0x46, 0x30, 0x31, 0x00 } */
#endif /* RFAL_FEATURE_ISO_DEP_POLL */

#if RFAL_FEATURE_NFC_DEP
/* P2P communication data */    
static uint8_t ndefLLCPSYMM[] = {0x00, 0x00};
static uint8_t ndefInit[] = {0x05, 0x20, 0x06, 0x0F, 0x75, 0x72, 0x6E, 0x3A, 0x6E, 0x66, 0x63, 0x3A, 0x73, 0x6E, 0x3A, 0x73, 0x6E, 0x65, 0x70, 0x02, 0x02, 0x07, 0x80, 0x05, 0x01, 0x02};
static uint8_t ndefUriSTcom[] = {0x13, 0x20, 0x00, 0x10, 0x02, 0x00, 0x00, 0x00, 0x19, 0xc1, 0x01, 0x00, 0x00, 0x00, 0x12, 0x55, 0x00, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x77, 0x77, 0x2e, 0x73, 0x74, 0x2e, 0x63, 0x6f, 0x6d};
#endif /* RFAL_FEATURE_NFC_DEP */

#if RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE
#if RFAL_SUPPORT_MODE_LISTEN_NFCA
/* NFC-A CE config */
/* 4-byte UIDs with first byte 0x08 would need random number for the subsequent 3 bytes.
 * 4-byte UIDs with first byte 0x*F are Fixed number, not unique, use for this demo
 * 7-byte UIDs need a manufacturer ID and need to assure uniqueness of the rest.*/
static uint8_t ceNFCA_NFCID[]     = {0x5F, 'S', 'T', 'M'};    /* =_STM, 5F 53 54 4D NFCID1 / UID (4 bytes) */
static uint8_t ceNFCA_SENS_RES[]  = {0x02, 0x00};             /* SENS_RES / ATQA for 4-byte UID            */
static uint8_t ceNFCA_SEL_RES     = 0x20;                     /* SEL_RES / SAK                             */
#endif /* RFAL_SUPPORT_MODE_LISTEN_NFCA */

static uint8_t ceNFCF_nfcid2[]     = {0x02, 0xFE, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

#if RFAL_SUPPORT_MODE_LISTEN_NFCF
  /* NFC-F CE config */
static uint8_t ceNFCF_SC[]         = {0x12, 0xFC};
static uint8_t ceNFCF_SENSF_RES[]  = {0x01,                                                   /* SENSF_RES                                */
                                  0x02, 0xFE, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,             /* NFCID2                                   */
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x7F, 0x00,             /* PAD0, PAD01, MRTIcheck, MRTIupdate, PAD2 */
                                  0x00, 0x00 };                                               /* RD                                       */
#endif /* RFAL_SUPPORT_MODE_LISTEN_NFCF */
#endif /* RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE */
                                 
/* VASUP-A Command: TCI must be set according to data received via MFi Program  */
static uint8_t demoEcpVasup[] = { 0x6A,    /* VASUP-A Command             */
                                  0x02,    /* Byte1  - Format: 2.0        */
                                  0xCB,    /* Byte2  - Terminal Info      */
                                  0x02,    /* Byte3  - Terminal Type      */
                                  0x04,    /* Byte4  - Terminal Subtype   */
                                  0x02,    /* Byte5  - TCI 1              */
                                  0x11,    /* Byte6  - TCI 2              */
                                  0x00,    /* Byte7  - TCI 3              */
                                  0xb0,0x2a,0x52,0x74,0xec,0x02,0x13,0x4d,  /* Reader Identifier */
};
static uint8_t expTransacSelectApp[] = { 0x00, 0xA4, 0x04, 0x00, 0x0c, 0xA0, 0x00, 0x00, 0x08, 0x58, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00};

//static uint8_t auth0_cmd[] = {}

//uint8_t reader_ePK[64]={0};

static uint8_t reader_group_head[]={0x4d,0x10};
static uint8_t reader_group_id[8]={0xb0,0x2a,0x52,0x74,0xec,0x02,0x13,0x4d};
static uint8_t reader_group_sub_id[8];

static uint8_t transaction_id_head[]={0x4c,0x10};
static uint8_t transaction_id[16];
static uint8_t auth1_usage[]={0x93,0x04,0x41,0x5d,0x95,0x69};

static uint8_t reader_ePK[65];
static uint8_t endp_ePK[65];
static uint8_t m_signature[64];
static uint8_t m_hash[32];
static uint8_t datafield[110];//plain text for sign auth1
static psa_key_handle_t keypair_handle;
static psa_key_handle_t pub_key_handle;
/*
/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */

static rfalNfcDiscoverParam discParam;
static uint8_t              state = DEMO_ST_NOTINIT;

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/

static void demoP2P( rfalNfcDevice *nfcDev );
static void demoAPDU( void );
static void demoAPDU_apple( void );
static void demoNfcv( rfalNfcvListenDevice *nfcvDev );
static void demoNfcf( rfalNfcfListenDevice *nfcfDev );
static void demoCE( rfalNfcDevice *nfcDev );
static void demoNotif( rfalNfcState st );
ReturnCode  demoTransceiveBlocking( uint8_t *txBuf, uint16_t txBufSize, uint8_t **rxBuf, uint16_t **rcvLen, uint32_t fwt );
void AUTH0_make();
void AUTH1_make();
static ReturnCode demoPropNfcInitialize( void )
{ //platformLog("in\n");
    rfalNfcaPollerInitialize();                            /* Initialize RFAL for NFC-A */
    rfalFieldOnAndStartGT();                               /* Turns the Field On and starts GT timer */

    return ERR_NONE;
}



/*****************************************************************************/
static ReturnCode demoPropNfcTechnologyDetection( void )
{
     //platformLog("$$\n");
    /* Send VASUP-A */
    rfalTransceiveBlockingTxRx( demoEcpVasup, sizeof(demoEcpVasup), NULL, 0, NULL, RFAL_TXRX_FLAGS_DEFAULT, RFAL_NFCA_FDTMIN );

    return ERR_TIMEOUT;
}
/*!
 *****************************************************************************
 * \brief Demo Notification
 *
 *  This function receives the event notifications from RFAL
 *****************************************************************************
 */
static void demoNotif( rfalNfcState st )
{
    uint8_t       devCnt;
    rfalNfcDevice *dev;
    
    if( st == RFAL_NFC_STATE_WAKEUP_MODE )
    {
        platformLog("Wake Up mode started \r\n");
    }
    else if( st == RFAL_NFC_STATE_POLL_TECHDETECT )
    {
        platformLog("Wake Up mode terminated. Polling for devices \r\n");
    }
    else if( st == RFAL_NFC_STATE_POLL_SELECT )
    {
        /* Multiple devices were found, activate first of them */
        rfalNfcGetDevicesFound( &dev, &devCnt );
        rfalNfcSelect( 0 );
        
        platformLog("Multiple Tags detected: %d \r\n", devCnt);
    }
}

/*!
 *****************************************************************************
 * \brief Demo Ini
 *
 *  This function Initializes the required layers for the demo
 *
 * \return true  : Initialization ok
 * \return false : Initialization failed
 *****************************************************************************
 */
bool demoIni( void )
{
    ReturnCode err;
    
    err = rfalNfcInitialize();
    if( err == ERR_NONE )
    {
        discParam.compMode      = RFAL_COMPLIANCE_MODE_NFC;
        discParam.devLimit      = 1U;
        discParam.nfcfBR        = RFAL_BR_212;
        discParam.ap2pBR        = RFAL_BR_424;
        discParam.maxBR         = RFAL_BR_KEEP;

        discParam.isoDepFS = RFAL_ISODEP_FSXI_256;
        discParam.nfcDepLR = RFAL_NFCDEP_LR_254; 
        ST_MEMCPY( &discParam.nfcid3, NFCID3, sizeof(NFCID3) );
        ST_MEMCPY( &discParam.GB, GB, sizeof(GB) );
        discParam.GBLen         = sizeof(GB);
        discParam.p2pNfcaPrio   = true;

        discParam.notifyCb             = demoNotif;
        discParam.wakeupEnabled        = false;
        discParam.wakeupConfigDefault  = true;
        discParam.wakeupNPolls         = 1U;
        discParam.totalDuration        = 100U;
        discParam.techs2Find           = RFAL_NFC_TECH_NONE;          /* For the demo, enable the NFC Technlogies based on RFAL Feature switches */


#if RFAL_FEATURE_NFCA
        discParam.techs2Find          |= RFAL_NFC_POLL_TECH_A;
#endif /* RFAL_FEATURE_NFCA */

#if RFAL_FEATURE_NFCB
      //  discParam.techs2Find          |= RFAL_NFC_POLL_TECH_B;
#endif /* RFAL_FEATURE_NFCB */

#if RFAL_FEATURE_NFCF
      //  discParam.techs2Find          |= RFAL_NFC_POLL_TECH_F;
#endif /* RFAL_FEATURE_NFCF */

#if RFAL_FEATURE_NFCV
      //  discParam.techs2Find          |= RFAL_NFC_POLL_TECH_V;
#endif /* RFAL_FEATURE_NFCV */

#if RFAL_FEATURE_ST25TB
      //  discParam.techs2Find          |= RFAL_NFC_POLL_TECH_ST25TB;
#endif /* RFAL_FEATURE_ST25TB */
        discParam.techs2Find          |= RFAL_NFC_POLL_TECH_PROP;
#if ST25R95
        discParam.isoDepFS           = RFAL_ISODEP_FSXI_128;          /* ST25R95 cannot support 256 bytes of data block */
#endif /* ST25R95 */

#if RFAL_SUPPORT_MODE_POLL_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP
        //discParam.techs2Find |= RFAL_NFC_POLL_TECH_AP2P;
#endif /* RFAL_SUPPORT_MODE_POLL_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP */

#if RFAL_SUPPORT_MODE_LISTEN_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP && RFAL_FEATURE_LISTEN_MODE
       // discParam.techs2Find |= RFAL_NFC_LISTEN_TECH_AP2P;
#endif /* RFAL_SUPPORT_MODE_LISTEN_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP && RFAL_FEATURE_LISTEN_MODE */

         ST_MEMSET(&discParam.propNfc, 0x00, sizeof(rfalNfcPropCallbacks) );
        discParam.propNfc.rfalNfcpPollerInitialize          = demoPropNfcInitialize;
        discParam.propNfc.rfalNfcpPollerTechnologyDetection = demoPropNfcTechnologyDetection;
#if DEMO_CARD_EMULATION_ONLY
        discParam.totalDuration        = 60000U;              /* 60 seconds */
        discParam.techs2Find           = RFAL_NFC_TECH_NONE;  /* Overwrite any previous poller modes */
#endif /* DEMO_CARD_EMULATION_ONLY */

#if RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE & 0
        demoCeInit( ceNFCF_nfcid2 );
    
#if RFAL_SUPPORT_MODE_LISTEN_NFCA
        /* Set configuration for NFC-A CE */
        ST_MEMCPY( discParam.lmConfigPA.SENS_RES, ceNFCA_SENS_RES, RFAL_LM_SENS_RES_LEN );     /* Set SENS_RES / ATQA */
        ST_MEMCPY( discParam.lmConfigPA.nfcid, ceNFCA_NFCID, RFAL_LM_NFCID_LEN_04 );           /* Set NFCID / UID */
        discParam.lmConfigPA.nfcidLen = RFAL_LM_NFCID_LEN_04;                                  /* Set NFCID length to 7 bytes */
        discParam.lmConfigPA.SEL_RES  = ceNFCA_SEL_RES;                                        /* Set SEL_RES / SAK */

        discParam.techs2Find |= RFAL_NFC_LISTEN_TECH_A;
#endif /* RFAL_SUPPORT_MODE_LISTEN_NFCA */

#if RFAL_SUPPORT_MODE_LISTEN_NFCF
        /* Set configuration for NFC-F CE */
        ST_MEMCPY( discParam.lmConfigPF.SC, ceNFCF_SC, RFAL_LM_SENSF_SC_LEN );                 /* Set System Code */
        ST_MEMCPY( &ceNFCF_SENSF_RES[RFAL_NFCF_CMD_LEN], ceNFCF_nfcid2, RFAL_NFCID2_LEN );     /* Load NFCID2 on SENSF_RES */
        ST_MEMCPY( discParam.lmConfigPF.SENSF_RES, ceNFCF_SENSF_RES, RFAL_LM_SENSF_RES_LEN );  /* Set SENSF_RES / Poll Response */

        discParam.techs2Find |= RFAL_NFC_LISTEN_TECH_F;
#endif /* RFAL_SUPPORT_MODE_LISTEN_NFCF */
#endif /* RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE */
//AUTH0_make();
//AUTH1_make();
        /* Check for valid configuration by calling Discover once */
        err = rfalNfcDiscover( &discParam );
        rfalNfcDeactivate( false );
        
        if( err != ERR_NONE )
        {
            return false;
        }

        state = DEMO_ST_START_DISCOVERY;
        return true;
    }
    return false;
}

/*!
 *****************************************************************************
 * \brief Demo Cycle
 *
 *  This function executes the demo state machine. 
 *  It must be called periodically
 *****************************************************************************
 */
void demoCycle( void )
{
    static rfalNfcDevice *nfcDevice;
    
    rfalNfcWorker();                                    /* Run RFAL worker periodically */

#if defined(PLATFORM_USER_BUTTON_PORT) && defined(PLATFORM_USER_BUTTON_PIN)
    /*******************************************************************************/
    /* Check if USER button is pressed */
    if( platformGpioIsLow(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN))
    {
        discParam.wakeupEnabled = !discParam.wakeupEnabled;    /* enable/disable wakeup */
        state = DEMO_ST_START_DISCOVERY;                       /* restart loop          */
        platformLog("Toggling Wake Up mode %s\r\n", discParam.wakeupEnabled ? "ON": "OFF");

        /* Debounce button */
        while( platformGpioIsLow(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN) );
    }
#endif /* PLATFORM_USER_BUTTON_PIN */
    
    switch( state )
    {
        /*******************************************************************************/
        case DEMO_ST_START_DISCOVERY:

          platformLedOff(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
          platformLedOff(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
          platformLedOff(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
          platformLedOff(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
          platformLedOff(PLATFORM_LED_AP2P_PORT, PLATFORM_LED_AP2P_PIN);
          platformLedOff(PLATFORM_LED_FIELD_PORT, PLATFORM_LED_FIELD_PIN);
          
          rfalNfcDeactivate( false );
          rfalNfcDiscover( &discParam );
        
          state = DEMO_ST_DISCOVERY;
          break;

        /*******************************************************************************/
        case DEMO_ST_DISCOVERY:
        
            if( rfalNfcIsDevActivated( rfalNfcGetState() ) )
            {
                rfalNfcGetActiveDevice( &nfcDevice );
                
                switch( nfcDevice->type )
                {
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_NFCA:
                    
                        platformLedOn(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
                        switch( nfcDevice->dev.nfca.type )
                        {
                            case RFAL_NFCA_T1T:
                                platformLog("ISO14443A/Topaz (NFC-A T1T) TAG found. UID: %s\r\n", hex2str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                                break;
                            
                            case RFAL_NFCA_T4T:
                                platformLog("NFCA Passive ISO-DEP device found. UID: %s\r\n", hex2str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                            
                               //demoAPDU();
                                demoAPDU_apple();
                                break;
                            
                            case RFAL_NFCA_T4T_NFCDEP:
                            case RFAL_NFCA_NFCDEP:
                                platformLog("NFCA Passive P2P device found. NFCID: %s\r\n", hex2str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                                
                                demoP2P( nfcDevice );
                                break;
                                
                            default:
                                platformLog("ISO14443A/NFC-A card found. UID: %s\r\n", hex2str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                                break;
                        }
                        break;
                    
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_NFCB:
                        
                        platformLog("ISO14443B/NFC-B card found. UID: %s\r\n", hex2str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                        platformLedOn(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
                    
                        if( rfalNfcbIsIsoDepSupported( &nfcDevice->dev.nfcb ) )
                        {
                            demoAPDU();
                        }
                        break;
                        
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_NFCF:
                        
                        if( rfalNfcfIsNfcDepSupported( &nfcDevice->dev.nfcf ) )
                        {
                            platformLog("NFCF Passive P2P device found. NFCID: %s\r\n", hex2str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                            demoP2P( nfcDevice );
                        }
                        else
                        {
                            platformLog("Felica/NFC-F card found. UID: %s\r\n", hex2str( nfcDevice->nfcid, nfcDevice->nfcidLen ));
                            
                            demoNfcf( &nfcDevice->dev.nfcf );
                        }
                        
                        platformLedOn(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
                        break;
                    
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_NFCV:
                        {
                            uint8_t devUID[RFAL_NFCV_UID_LEN];
                            
                            ST_MEMCPY( devUID, nfcDevice->nfcid, nfcDevice->nfcidLen );   /* Copy the UID into local var */
                            REVERSE_BYTES( devUID, RFAL_NFCV_UID_LEN );                 /* Reverse the UID for display purposes */
                            platformLog("ISO15693/NFC-V card found. UID: %s\r\n", hex2str(devUID, RFAL_NFCV_UID_LEN));
                        
                            platformLedOn(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
                            
                            demoNfcv( &nfcDevice->dev.nfcv );
                        }
                        break;
                        
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_ST25TB:
                        
                        platformLog("ST25TB card found. UID: %s\r\n", hex2str( nfcDevice->nfcid, nfcDevice->nfcidLen ));
                        platformLedOn(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
                        break;
                    
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_AP2P:
                    case RFAL_NFC_POLL_TYPE_AP2P:
                        
                        platformLog("NFC Active P2P device found. NFCID3: %s\r\n", hex2str(nfcDevice->nfcid, nfcDevice->nfcidLen));
                        platformLedOn(PLATFORM_LED_AP2P_PORT, PLATFORM_LED_AP2P_PIN);
                    
                        demoP2P( nfcDevice );
                        break;
                    
                    /*******************************************************************************/
                    case RFAL_NFC_POLL_TYPE_NFCA:
                    case RFAL_NFC_POLL_TYPE_NFCF:
                        platformLog("poll\n");
                        platformLog("Activated in CE %s mode.\r\n", (nfcDevice->type == RFAL_NFC_POLL_TYPE_NFCA) ? "NFC-A" : "NFC-F");
                        platformLedOn( ((nfcDevice->type == RFAL_NFC_POLL_TYPE_NFCA) ? PLATFORM_LED_A_PORT : PLATFORM_LED_F_PORT), 
                                       ((nfcDevice->type == RFAL_NFC_POLL_TYPE_NFCA) ? PLATFORM_LED_A_PIN  : PLATFORM_LED_F_PIN)  );
                    
                       // demoCE( nfcDevice );
                       demoAPDU_apple();
                        break;
                    
                    /*******************************************************************************/
                    default:
                        break;
                }
                
                rfalNfcDeactivate( false );

#if !defined(DEMO_NO_DELAY_IN_DEMOCYCLE)
                switch( nfcDevice->type )
                {
                    case RFAL_NFC_POLL_TYPE_NFCA:
                    case RFAL_NFC_POLL_TYPE_NFCF:
                        break; /* When being in card emulation don't delay as some polling devices (phones) rely on tags to be re-discoverable */
                    default:
                        platformDelay(500); /* Delay before re-starting polling loop to not flood the UART log with re-discovered tags */
                }
#endif /* DEMO_NO_DELAY_IN_DEMOCYCLE */
                
                state = DEMO_ST_START_DISCOVERY;
            }
            break;

        /*******************************************************************************/
        case DEMO_ST_NOTINIT:
        default:
            break;
    }
}

static void demoCE( rfalNfcDevice *nfcDev )
{
#if RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE
    
    ReturnCode err;
    uint8_t *rxData;
    uint16_t *rcvLen;
    uint8_t  txBuf[150];
    uint16_t txLen;

    do
    {
        rfalNfcWorker();
        
        switch( rfalNfcGetState() )
        {
            case RFAL_NFC_STATE_ACTIVATED:
            platformLog("ce:actvated\n");
                err = demoTransceiveBlocking( NULL, 0, &rxData, &rcvLen, 0);
                break;
            
            case RFAL_NFC_STATE_DATAEXCHANGE:
            case RFAL_NFC_STATE_DATAEXCHANGE_DONE:
                 platformLog("ce:dataex ty%d\n",nfcDev->type );
                txLen = ( (nfcDev->type == RFAL_NFC_POLL_TYPE_NFCA) ? demoCeT4T( rxData, *rcvLen, txBuf, sizeof(txBuf) ) : rfalConvBytesToBits( demoCeT3T( rxData, rfalConvBitsToBytes(*rcvLen), txBuf, sizeof(txBuf) ) ) );
                err   = demoTransceiveBlocking( txBuf, txLen, &rxData, &rcvLen, RFAL_FWT_NONE );
                break;
            
            case RFAL_NFC_STATE_START_DISCOVERY:
                return;
            
            case RFAL_NFC_STATE_LISTEN_SLEEP:
            default:
                break;
        }
    }
    while( (err == ERR_NONE) || (err == ERR_SLEEP_REQ) );

#else
    NO_WARNING(nfcDev);
#endif /* RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE */
}

/*!
 *****************************************************************************
 * \brief Demo NFC-F 
 *
 * Example how to exchange read and write blocks on a NFC-F tag
 * 
 *****************************************************************************
 */
static void demoNfcf( rfalNfcfListenDevice *nfcfDev )
{
#if RFAL_FEATURE_NFCF
    
    ReturnCode                 err;
    uint8_t                    buf[ (RFAL_NFCF_NFCID2_LEN + RFAL_NFCF_CMD_LEN + (3*RFAL_NFCF_BLOCK_LEN)) ];
    uint16_t                   rcvLen;
    rfalNfcfServ               srv = RFAL_NFCF_SERVICECODE_RDWR;
    rfalNfcfBlockListElem      bl[3];
    rfalNfcfServBlockListParam servBlock;
    //uint8_t                    wrData[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
    
    servBlock.numServ   = 1;                            /* Only one Service to be used           */
    servBlock.servList  = &srv;                         /* Service Code: NDEF is Read/Writeable  */
    servBlock.numBlock  = 1;                            /* Only one block to be used             */
    servBlock.blockList = bl;
    bl[0].conf     = RFAL_NFCF_BLOCKLISTELEM_LEN_BIT;   /* Two-byte Block List Element           */
    bl[0].blockNum = 0x0001;                            /* Block: NDEF Data                      */
    
    err = rfalNfcfPollerCheck( nfcfDev->sensfRes.NFCID2, &servBlock, buf, sizeof(buf), &rcvLen);
    platformLog(" Check Block: %s Data:  %s \r\n", (err != ERR_NONE) ? "FAIL": "OK", (err != ERR_NONE) ? "" : hex2str( &buf[1], RFAL_NFCF_BLOCK_LEN) );
    
    #if 0  /* Writing example */
        err = rfalNfcfPollerUpdate( nfcfDev->sensfRes.NFCID2, &servBlock, buf , sizeof(buf), wrData, buf, sizeof(buf) );
        platformLog(" Update Block: %s Data: %s \r\n", (err != ERR_NONE) ? "FAIL": "OK", (err != ERR_NONE) ? "" : hex2str( wrData, RFAL_NFCF_BLOCK_LEN) );
        err = rfalNfcfPollerCheck( nfcfDev->sensfRes.NFCID2, &servBlock, buf, sizeof(buf), &rcvLen);
        platformLog(" Check Block:  %s Data: %s \r\n", (err != ERR_NONE) ? "FAIL": "OK", (err != ERR_NONE) ? "" : hex2str( &buf[1], RFAL_NFCF_BLOCK_LEN) );
    #endif
    
#endif /* RFAL_FEATURE_NFCF */
}

/*!
 *****************************************************************************
 * \brief Demo NFC-V Exchange
 *
 * Example how to exchange read and write blocks on a NFC-V tag
 * 
 *****************************************************************************
 */
static void demoNfcv( rfalNfcvListenDevice *nfcvDev )
{
#if RFAL_FEATURE_NFCV
    
    ReturnCode            err;
    uint16_t              rcvLen;
    uint8_t               blockNum = 1;
    uint8_t               rxBuf[ 1 + DEMO_NFCV_BLOCK_LEN + RFAL_CRC_LEN ];                        /* Flags + Block Data + CRC */
    uint8_t               *uid; 
    uint8_t               reqFlag;
#if DEMO_NFCV_WRITE_TAG
    uint8_t               wrData[DEMO_NFCV_BLOCK_LEN] = { 0x11, 0x22, 0x33, 0x99 };             /* Write block example */
#endif /* DEMO_NFCV_WRITE_TAG */
              

    uid     = nfcvDev->InvRes.UID;
    reqFlag = RFAL_NFCV_REQ_FLAG_DEFAULT;
    
    #if DEMO_NFCV_USE_SELECT_MODE
        /*
        * Activate selected state
        */
        err = rfalNfcvPollerSelect( reqFlag, nfcvDev->InvRes.UID );
        platformLog(" Select %s \r\n", (err != ERR_NONE) ? "FAIL (revert to addressed mode)": "OK" );
        if( err == ERR_NONE )
        {
            reqFlag = (RFAL_NFCV_REQ_FLAG_DEFAULT | RFAL_NFCV_REQ_FLAG_SELECT);
            uid     = NULL;
        }
    #endif /* DEMO_NFCV_USE_SELECT_MODE */

    /*
    * Read block using Read Single Block command
    * with addressed mode (uid != NULL) or selected mode (uid == NULL)
    */
    err = rfalNfcvPollerReadSingleBlock(reqFlag, uid, blockNum, rxBuf, sizeof(rxBuf), &rcvLen);
    platformLog(" Read Block: %s %s\r\n", (err != ERR_NONE) ? "FAIL": "OK Data:", (err != ERR_NONE) ? "" : hex2str( &rxBuf[1], DEMO_NFCV_BLOCK_LEN));
 
    #if DEMO_NFCV_WRITE_TAG /* Writing example */
        err = rfalNfcvPollerWriteSingleBlock(reqFlag, uid, blockNum, wrData, sizeof(wrData));
        platformLog(" Write Block: %s Data: %s\r\n", (err != ERR_NONE) ? "FAIL": "OK", hex2str( wrData, DEMO_NFCV_BLOCK_LEN) );
        err = rfalNfcvPollerReadSingleBlock(reqFlag, uid, blockNum, rxBuf, sizeof(rxBuf), &rcvLen);
        platformLog(" Read Block: %s %s\r\n", (err != ERR_NONE) ? "FAIL": "OK Data:", (err != ERR_NONE) ? "" : hex2str( &rxBuf[1], DEMO_NFCV_BLOCK_LEN));
    #endif /* DEMO_NFCV_WRITE_TAG */
        
#endif /* RFAL_FEATURE_NFCV */        
}


/*!
 *****************************************************************************
 * \brief Demo P2P Exchange
 *
 * Sends a NDEF URI record 'http://www.ST.com' via NFC-DEP (P2P) protocol.
 * 
 * This method sends a set of static predefined frames which tries to establish
 * a LLCP connection, followed by the NDEF record, and then keeps sending 
 * LLCP SYMM packets to maintain the connection.
 * 
 * 
 *****************************************************************************
 */
void demoP2P( rfalNfcDevice *nfcDev )
{
#if RFAL_FEATURE_NFC_DEP
    
    uint16_t   *rxLen;
    uint8_t    *rxData;
    ReturnCode err;
    
    /* In Listen mode retrieve the first request from Initiator */
    if( nfcDev->type == RFAL_NFC_POLL_TYPE_AP2P )
    {
        demoTransceiveBlocking( NULL, 0, &rxData, &rxLen, 0);   
        
        /* Initiator request is being ignored/discarded  */
    }

    platformLog(" Initialize device .. ");
    err = demoTransceiveBlocking( ndefInit, sizeof(ndefInit), &rxData, &rxLen, RFAL_FWT_NONE);
    if( err != ERR_NONE )
    {
        platformLog("failed.\r\n");
        return;
    }
    platformLog("succeeded.\r\n");

    platformLog(" Push NDEF Uri: www.st.com .. ");
    err = demoTransceiveBlocking( ndefUriSTcom, sizeof(ndefUriSTcom), &rxData, &rxLen, RFAL_FWT_NONE);
    if( err != ERR_NONE )
    {
        platformLog("failed.\r\n");
        return;
    }
    platformLog("succeeded.\r\n");


    platformLog(" Device present, maintaining connection ");
    while(err == ERR_NONE) 
    {
        err = demoTransceiveBlocking( ndefLLCPSYMM, sizeof(ndefLLCPSYMM), &rxData, &rxLen, RFAL_FWT_NONE);
        platformLog(".");
        platformDelay(50);
    }
    platformLog("\r\n Device removed.\r\n");
    
#endif /* RFAL_FEATURE_NFC_DEP */
}


/*!
 *****************************************************************************
 * \brief Demo APDUs Exchange
 *
 * Example how to exchange a set of predefined APDUs with PICC. The NDEF
 * application will be selected and then CC will be selected and read.
 * 
 *****************************************************************************
 */
void demoAPDU( void )
{
#if RFAL_FEATURE_ISO_DEP_POLL
    ReturnCode err;
    uint16_t   *rxLen;
    uint8_t    *rxData;

    /* Exchange APDU: NDEF Tag Application Select command */
    err = demoTransceiveBlocking( ndefSelectApp, sizeof(ndefSelectApp), &rxData, &rxLen, RFAL_FWT_NONE );
    platformLog(" Select NDEF Application: %s Data: %s\r\n", (err != ERR_NONE) ? "FAIL": "OK", (err != ERR_NONE) ? "" : hex2str( rxData, *rxLen) );

    if( (err == ERR_NONE) && rxData[0] == 0x90 && rxData[1] == 0x00)
    {
        /* Exchange APDU: Select Capability Container File */
        err = demoTransceiveBlocking( ccSelectFile, sizeof(ccSelectFile), &rxData, &rxLen, RFAL_FWT_NONE );
        platformLog(" Select CC: %s Data: %s\r\n", (err != ERR_NONE) ? "FAIL": "OK", (err != ERR_NONE) ? "" : hex2str( rxData, *rxLen) );

        /* Exchange APDU: Read Capability Container File  */
        err = demoTransceiveBlocking( readBinary, sizeof(readBinary), &rxData, &rxLen, RFAL_FWT_NONE );
        platformLog(" Read CC: %s Data: %s\r\n", (err != ERR_NONE) ? "FAIL": "OK", (err != ERR_NONE) ? "" : hex2str( rxData, *rxLen) );
    }
#endif /* RFAL_FEATURE_ISO_DEP_POLL */
}
void demoAPDU_apple( void )
{
 ReturnCode err;
    uint16_t   *rxLen;
    uint8_t    *rxData;
platformLog("apdu send\n");
    /* Exchange APDU: Unified Access APDUs */
    err = demoTransceiveBlocking( expTransacSelectApp, sizeof(expTransacSelectApp), &rxData, &rxLen, RFAL_FWT_NONE );
 platformLog(" err:%d\n",err);
    if( (err == ERR_NONE) && (*rxLen > 2) && rxData[*rxLen-2] == 0x90 && rxData[*rxLen-1] == 0x00)
    {
        /* Here more APDUs to implement the protocol are required. */
        platformLog(" continu...\n");
        AUTH0_make();
        AUTH1_make();
    }

}

/*!
 *****************************************************************************
 * \brief Demo Blocking Transceive 
 *
 * Helper function to send data in a blocking manner via the rfalNfc module 
 *  
 * \warning A protocol transceive handles long timeouts (several seconds), 
 * transmission errors and retransmissions which may lead to a long period of 
 * time where the MCU/CPU is blocked in this method.
 * This is a demo implementation, for a non-blocking usage example please 
 * refer to the Examples available with RFAL
 *
 * \param[in]  txBuf      : data to be transmitted
 * \param[in]  txBufSize  : size of the data to be transmited
 * \param[out] rxData     : location where the received data has been placed
 * \param[out] rcvLen     : number of data bytes received
 * \param[in]  fwt        : FWT to be used (only for RF frame interface, 
 *                                          otherwise use RFAL_FWT_NONE)
 *
 * 
 *  \return ERR_PARAM     : Invalid parameters
 *  \return ERR_TIMEOUT   : Timeout error
 *  \return ERR_FRAMING   : Framing error detected
 *  \return ERR_PROTO     : Protocol error detected
 *  \return ERR_NONE      : No error, activation successful
 * 
 *****************************************************************************
 */
ReturnCode demoTransceiveBlocking( uint8_t *txBuf, uint16_t txBufSize, uint8_t **rxData, uint16_t **rcvLen, uint32_t fwt )
{
    ReturnCode err;
    
    err = rfalNfcDataExchangeStart( txBuf, txBufSize, rxData, rcvLen, fwt );
    if( err == ERR_NONE )
    {
        do{
            rfalNfcWorker();
            err = rfalNfcDataExchangeGetStatus();
        }
        while( err == ERR_BUSY );
    }
    return err;
}

int crypto_finish(void)
{
	psa_status_t status;

	/* Destroy the key handle */
	status = psa_destroy_key(keypair_handle);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_destroy_key failed! (Error: %d)", status);
		return -1;
	}

	status = psa_destroy_key(pub_key_handle);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_destroy_key failed! (Error: %d)", status);
		return -1;
	}

	return 0;
}
int generate_ecdsa_keypair(void)
{
	psa_status_t status;
	size_t olen;

	LOG_INF("Generating random ECDSA keypair...");

	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	/* Configure the key attributes */
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_SIGN_HASH);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attributes, 256);

	/* Generate a random keypair. The keypair is not exposed to the application,
	 * we can use it to signing/verification the key handle.
	 */
	status = psa_generate_key(&key_attributes, &keypair_handle);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_generate_key failed! (Error: %d)", status);
		return -1;
	}

	/* Export the public key */
	status = psa_export_public_key(keypair_handle, reader_ePK, sizeof(reader_ePK), &olen);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_export_public_key failed! (Error: %d)", status);
		return -1;
	}
    LOG_INF("olen %d %d %d\n",olen,reader_ePK[0],reader_ePK[1]);
	/* After the key handle is acquired the attributes are not needed */
	psa_reset_key_attributes(&key_attributes);


	return 0;
}
void AUTH0_make()
{
    psa_status_t status;
    uint8_t auth0_head[]={0x80,0x80,0x01,0x01,0x6b,0x5c,0x02,0x02,0x00};
    uint8_t reader_ePK_head[]={0x87,0x41};


    uint8_t auth0_end[]={0x00};
    uint8_t auth0_data[113]={0};

    ReturnCode err;
    uint16_t   *rxLen;
    uint8_t    *rxData;
      platformLog("AUTH0_make start...\n");
   	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS)
		{
            platformLog("psa_crypto_init failed!\n");
            return ;
        }

    status = generate_ecdsa_keypair();
	if (status != 0) {
		platformLog("generate_ecdsa_keypair fail");
		return ;
	}
    status = psa_generate_random(transaction_id, sizeof(transaction_id));
		if (status != PSA_SUCCESS) {
			platformLog("psa_generate_random failed! (Error: %d)", status);
			return ;
		}
    status = psa_generate_random(reader_group_sub_id, sizeof(reader_group_sub_id));
		if (status != PSA_SUCCESS) {
			platformLog("psa_generate_random failed! (Error: %d)", status);
			return ;
		}
    memcpy(auth0_data,auth0_head,sizeof(auth0_head));
    memcpy(auth0_data+sizeof(auth0_head),reader_ePK_head,sizeof(reader_ePK_head));
    memcpy(auth0_data+sizeof(auth0_head)+sizeof(reader_ePK_head),reader_ePK,sizeof(reader_ePK));
    memcpy(auth0_data+sizeof(auth0_head)+sizeof(reader_ePK_head)+sizeof(reader_ePK),transaction_id_head,sizeof(transaction_id_head));
    memcpy(auth0_data+sizeof(auth0_head)+sizeof(reader_ePK_head)+sizeof(reader_ePK)+sizeof(transaction_id_head),transaction_id,sizeof(transaction_id));
    memcpy(auth0_data+sizeof(auth0_head)+sizeof(reader_ePK_head)+sizeof(reader_ePK)+sizeof(transaction_id_head)+sizeof(transaction_id),reader_group_head,sizeof(reader_group_head));
    memcpy(auth0_data+sizeof(auth0_head)+sizeof(reader_ePK_head)+sizeof(reader_ePK)+sizeof(transaction_id_head)+sizeof(transaction_id)+sizeof(reader_group_head),reader_group_id,sizeof(reader_group_id));
    memcpy(auth0_data+sizeof(auth0_head)+sizeof(reader_ePK_head)+sizeof(reader_ePK)+sizeof(transaction_id_head)+sizeof(transaction_id)+sizeof(reader_group_head)+sizeof(reader_group_id),reader_group_sub_id,sizeof(reader_group_sub_id));
    memcpy(auth0_data+sizeof(auth0_head)+sizeof(reader_ePK_head)+sizeof(reader_ePK)+sizeof(transaction_id_head)+sizeof(transaction_id)+sizeof(reader_group_head)+sizeof(reader_group_id)+sizeof(reader_group_sub_id),auth0_end,sizeof(auth0_end));
  #if 0
    printk("--------------------%d\n ",sizeof(auth0_head)+sizeof(reader_ePK_head)+sizeof(reader_ePK)+sizeof(transaction_id_head)+sizeof(transaction_id)+sizeof(reader_group_head)+sizeof(reader_group_id)+sizeof(reader_group_sub_id)+sizeof(auth0_end));
    for(int i=0;i<30;i++)
    {
        printk("%x ",auth0_data[i]);
    }
    printk("-------------------------\n ");
    for(int i=30;i<60;i++)
    {
        printk("%x ",auth0_data[i]);
    }
printk("-------------------------\n ");
    for(int i=60;i<90;i++)
    {
        printk("%x ",auth0_data[i]);
    }
printk("-------------------------\n ");
    for(int i=90;i<113;i++)
    {
        printk("%x ",auth0_data[i]);
    }
printk("-------------------------\n ");
#endif
    /* Exchange APDU: Unified Access APDUs */
    err = demoTransceiveBlocking( auth0_data, sizeof(auth0_data), &rxData, &rxLen, RFAL_FWT_NONE );
    if( (err == ERR_NONE) && (*rxLen > 2) && rxData[*rxLen-2] == 0x90 && rxData[*rxLen-1] == 0x00)
    {
        /* Here more APDUs to implement the protocol are required. */
        platformLog(" auth0 succ...%d\n",*rxLen);
        if(*rxLen>67)
        {
           /* for(int i=2;i<67;i++)
            {
                printk("%x ",rxData[i]);
            }*/
            memcpy(reader_ePK,rxData+2,65);
        }
        printk("!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n ");
    }
    //crypto_finish();
    return;
}

int sign_message(void)
{
	uint32_t output_len;
	psa_status_t status;

	LOG_INF("Signing a message using ECDSA...");

	/* Compute the SHA256 hash*/
	status = psa_hash_compute(PSA_ALG_SHA_256,
				  datafield,
				  sizeof(datafield),
				  m_hash,
				  sizeof(m_hash),
				  &output_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_hash_compute failed! (Error: %d)", status);
		return -1;
	}

	/* Sign the hash */
	status = psa_sign_hash(keypair_handle,
			       PSA_ALG_ECDSA(PSA_ALG_SHA_256),
			       m_hash,
			       sizeof(m_hash),
			       m_signature,
			       sizeof(m_signature),
			       &output_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_sign_hash failed! (Error: %d)", status);
		return -1;
	}

	LOG_INF("Signing the message successful!");
crypto_finish();
	return 0;
}
void AUTH1_make()
{
    uint8_t auth1_head[]={0x80,0x81,0x00,0x00,0x42};
    uint8_t reader_sig_head[]={0x9e,0x40};
    //uint8_t reader_sig[64]={0};
    uint8_t auth1_end[]={0x00};
   uint8_t   auth1_cmd[72];
    uint8_t endp_ePKX_h[]={0x86,0x20};
    uint8_t reader_ePKX_h[]={0x87,0x20};
    uint8_t usage[]={0x93,0x04,0x41,0x5d,0x95,0x69};
    uint8_t endp_ePKX[32];
    uint8_t reader_ePKX[32];
    ReturnCode err;
     uint16_t   *rxLen;
    uint8_t    *rxData;
    memcpy(datafield,reader_group_head,sizeof(reader_group_head));
    memcpy(datafield+sizeof(reader_group_head),reader_group_id,sizeof(reader_group_id));
    memcpy(datafield+sizeof(reader_group_head)+sizeof(reader_group_id),reader_group_sub_id,sizeof(reader_group_sub_id));
    memcpy(datafield+sizeof(reader_group_head)+sizeof(reader_group_id)+sizeof(reader_group_sub_id),endp_ePKX_h,sizeof(endp_ePKX_h));
    memcpy(datafield+sizeof(reader_group_head)+sizeof(reader_group_id)+sizeof(reader_group_sub_id)+sizeof(endp_ePKX_h),endp_ePK+1,32);
    memcpy(datafield+sizeof(reader_group_head)+sizeof(reader_group_id)+sizeof(reader_group_sub_id)+sizeof(endp_ePKX_h)+32,reader_ePKX_h,sizeof(reader_ePKX_h));
    memcpy(datafield+sizeof(reader_group_head)+sizeof(reader_group_id)+sizeof(reader_group_sub_id)+sizeof(endp_ePKX_h)+32+sizeof(reader_ePKX_h),reader_ePK+1,32);
    memcpy(datafield+sizeof(reader_group_head)+sizeof(reader_group_id)+sizeof(reader_group_sub_id)+sizeof(endp_ePKX_h)+32+sizeof(reader_ePKX_h)+32,transaction_id_head,sizeof(transaction_id_head));
    memcpy(datafield+sizeof(reader_group_head)+sizeof(reader_group_id)+sizeof(reader_group_sub_id)+sizeof(endp_ePKX_h)+32+sizeof(reader_ePKX_h)+32+sizeof(transaction_id_head),transaction_id,sizeof(transaction_id));
    memcpy(datafield+sizeof(reader_group_head)+sizeof(reader_group_id)+sizeof(reader_group_sub_id)+sizeof(endp_ePKX_h)+32+sizeof(reader_ePKX_h)+32+sizeof(transaction_id_head)+sizeof(transaction_id),usage,sizeof(usage));
   // printk("$$$%d",sizeof(reader_group_head)+sizeof(reader_group_id)+sizeof(reader_group_sub_id)+sizeof(endp_ePKX_h)+32+sizeof(reader_ePKX_h)+32+sizeof(transaction_id_head)+sizeof(transaction_id)+sizeof(usage));
    sign_message();

    memcpy(auth1_cmd,auth1_head,sizeof(auth1_head));
    memcpy(auth1_cmd+sizeof(auth1_head),reader_sig_head,sizeof(reader_sig_head));
    memcpy(auth1_cmd+sizeof(auth1_head)+sizeof(reader_sig_head),m_signature,sizeof(m_signature));
    memcpy(auth1_cmd+sizeof(auth1_head)+sizeof(reader_sig_head)+sizeof(m_signature),auth1_end,sizeof(auth1_end));
    err = demoTransceiveBlocking( auth1_cmd, sizeof(auth1_cmd), &rxData, &rxLen, RFAL_FWT_NONE );
    if( (err == ERR_NONE) && (*rxLen > 2) && rxData[*rxLen-2] == 0x90 && rxData[*rxLen-1] == 0x00)
    {
        /* Here more APDUs to implement the protocol are required. */
        platformLog(" err %d\n",err);
        for(int i=0;i<*rxLen;i++)
            {
                platformLog("%x ",rxData[i]);
            }
    }else 
    {   platformLog("err%d\n",err);
         for(int i=0;i<*rxLen;i++)
            {
                platformLog("%x ",rxData[i]);
            }
    }
}   
#if 1
int import_ecdsa_pub_key(void)
{
	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	/* Configure the key attributes */
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attributes, 256);

	status = psa_import_key(&key_attributes, m_pub_key, sizeof(m_pub_key), &pub_key_handle);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_import_key failed! (Error: %d)", status);
		return -1;
	}

	/* After the key handle is acquired the attributes are not needed */
	psa_reset_key_attributes(&key_attributes);

	return 0;
}
#endif