
/******************************************************************************
  * @attention
  *
  * COPYRIGHT 2016 STMicroelectronics, all rights reserved
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
 *      PROJECT:   ST25R3916 firmware
 *      Revision: 
 *      LANGUAGE:  ISO C99
 */

/*! \file
 *
 *  \author Gustavo Patricio
 *
 *  \brief ST25R3916 Interrupt handling
 *
 */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/spinlock.h>
#include <zephyr/logging/log.h>


#include "st25r3916_irq.h"
#include "st25r3916_com.h"
#include "st25r3916_led.h"
#include "st25r3916.h"
#include "st25r3916_dt.h"

LOG_MODULE_DECLARE(st25r3916);

/*
 ******************************************************************************
 * LOCAL DATA TYPES
 ******************************************************************************
 */

/*! Holds current and previous interrupt callback pointer as well as current Interrupt status and mask */
typedef struct
{
    void      (*prevCallback)(void); /*!< call back function for ST25R3916 interrupt          */
    void      (*callback)(void);     /*!< call back function for ST25R3916 interrupt          */
    uint32_t  status;                /*!< latest interrupt status                             */
    uint32_t  mask;                  /*!< Interrupt mask. Negative mask = ST25R3916 mask regs */
} st25r3916Interrupt;


/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/

/*! Length of the interrupt registers       */
#define ST25R3916_INT_REGS_LEN          ( (ST25R3916_REG_IRQ_TARGET - ST25R3916_REG_IRQ_MAIN) + 1U )

/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/

static volatile st25r3916Interrupt   st25r3916interrupt; /*!< Instance of ST25R3916 interrupt */
static const struct gpio_dt_spec irq_gpio =
	GPIO_DT_SPEC_GET(ST25R3911B_NODE, irq_gpios);

static struct gpio_callback gpio_cb;
static int platformGpioIsHigh(void)
{	
	int value = 0;
	value = gpio_pin_get_dt(&irq_gpio);
	value = (value < 0) ? 0 : value;
	return value;
}
static void st25r3916Isr(const struct device *gpiob, struct gpio_callback *cb,
		       uint32_t pins)
{
	st25r3916CheckForReceivedInterrupts();
    
    // Check if callback is set and run it
    if( NULL != st25r3916interrupt.callback )
    {
        st25r3916interrupt.callback();
    }
}

static int platformIrqST25RPinInitialize(void)
{
	int err;

	LOG_DBG("Setting up interrupts on %s pin %d", irq_gpio.port->name,
		irq_gpio.pin);

	if (!device_is_ready(irq_gpio.port)) {
		LOG_ERR("IRQ GPIO device not ready");
		return -ENODEV;
	}

	/* Configure IRQ pin */
	err = gpio_pin_configure_dt(&irq_gpio, GPIO_INPUT);
	if (err) {
		return err;
	}

	gpio_init_callback(&gpio_cb, st25r3916Isr, BIT(irq_gpio.pin));

	err = gpio_add_callback(irq_gpio.port, &gpio_cb);
	if (err) {
		return err;
	};

	return gpio_pin_interrupt_configure_dt(&irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
void st25r3916InitInterrupts( void )
{
    platformIrqST25RPinInitialize();
    platformIrqST25RSetCallback( st25r3916Isr );
    
    
    st25r3916interrupt.callback     = NULL;
    st25r3916interrupt.prevCallback = NULL;
    st25r3916interrupt.status       = ST25R3916_IRQ_MASK_NONE;
    st25r3916interrupt.mask         = ST25R3916_IRQ_MASK_NONE;
}


/******************************************************************************
void st25r3916Isr( void )
{
 st25r3916CheckForReceivedInterrupts();
    
    // Check if callback is set and run it
    if( NULL != st25r3916interrupt.callback )
    {
        st25r3916interrupt.callback();
    }
}

*/

/*******************************************************************************/
void st25r3916CheckForReceivedInterrupts( void )
{
    uint8_t  iregs[ST25R3916_INT_REGS_LEN];
    uint32_t irqStatus;
    
    /* Initialize iregs */
    irqStatus = ST25R3916_IRQ_MASK_NONE;
    RFAL_MEMSET( iregs, (int32_t)(ST25R3916_IRQ_MASK_ALL & 0xFFU), ST25R3916_INT_REGS_LEN );
    
    
    /* In case the IRQ is Edge (not Level) triggered read IRQs until done */
   while( platformGpioIsHigh() )
   {
       st25r3916ReadMultipleRegisters( ST25R3916_REG_IRQ_MAIN, iregs, ST25R3916_INT_REGS_LEN );
       
       irqStatus |= (uint32_t)iregs[0];
       irqStatus |= (uint32_t)iregs[1]<<8;
       irqStatus |= (uint32_t)iregs[2]<<16;
       irqStatus |= (uint32_t)iregs[3]<<24;
   }
   
   /* Forward all interrupts, even masked ones to application */
   platformProtectST25RIrqStatus();
   st25r3916interrupt.status |= irqStatus;
   platformUnprotectST25RIrqStatus();
   
   /* Send an IRQ event to LED handling */
   st25r3916ledEvtIrq( st25r3916interrupt.status );
}


/*******************************************************************************/
void st25r3916ModifyInterrupts(uint32_t clr_mask, uint32_t set_mask)
{
    uint8_t  i;
    uint32_t old_mask;
    uint32_t new_mask;
    

    old_mask = st25r3916interrupt.mask;
    new_mask = ((~old_mask & set_mask) | (old_mask & clr_mask));
    st25r3916interrupt.mask &= ~clr_mask;
    st25r3916interrupt.mask |= set_mask;
    
    for(i=0; i<ST25R3916_INT_REGS_LEN; i++)
    { 
        if( ((new_mask >> (8U*i)) & 0xFFU) == 0U )
        {
            continue;
        }
        
        st25r3916WriteRegister(ST25R3916_REG_IRQ_MASK_MAIN + i, (uint8_t)((st25r3916interrupt.mask>>(8U*i)) & 0xFFU) );
    }
    return;
}


/*******************************************************************************/
uint32_t st25r3916WaitForInterruptsTimed( uint32_t mask, uint16_t tmo )
{
    uint32_t tmrDelay;
    uint32_t status;
    
    tmrDelay = platformTimerCreate( tmo );
    
    /* Run until specific interrupt has happen or the timer has expired */
    do 
    {
        status = (st25r3916interrupt.status & mask);
    } while( ( (!platformTimerIsExpired( tmrDelay )) || (tmo == 0U)) && (status == 0U) );
    
    platformTimerDestroy( tmrDelay );

    status = st25r3916interrupt.status & mask;
    
    platformProtectST25RIrqStatus();
    st25r3916interrupt.status &= ~status;
    platformUnprotectST25RIrqStatus();
    
    return status;
}


/*******************************************************************************/
uint32_t st25r3916GetInterrupt( uint32_t mask )
{
    uint32_t irqs;

    irqs = (st25r3916interrupt.status & mask);
    if(irqs != ST25R3916_IRQ_MASK_NONE)
    {
        platformProtectST25RIrqStatus();
        st25r3916interrupt.status &= ~irqs;
        platformUnprotectST25RIrqStatus();
    }

    return irqs;
}


/*******************************************************************************/
void st25r3916ClearAndEnableInterrupts( uint32_t mask )
{
    st25r3916GetInterrupt( mask );
    st25r3916EnableInterrupts( mask );
}


/*******************************************************************************/
void st25r3916EnableInterrupts(uint32_t mask)
{
    st25r3916ModifyInterrupts(mask, 0);
}


/*******************************************************************************/
void st25r3916DisableInterrupts(uint32_t mask)
{
    st25r3916ModifyInterrupts(0, mask);
}

/*******************************************************************************/
void st25r3916ClearInterrupts( void )
{
    uint8_t iregs[ST25R3916_INT_REGS_LEN];

    st25r3916ReadMultipleRegisters(ST25R3916_REG_IRQ_MAIN, iregs, ST25R3916_INT_REGS_LEN);

    platformProtectST25RIrqStatus();
    st25r3916interrupt.status = ST25R3916_IRQ_MASK_NONE;
    platformUnprotectST25RIrqStatus();
    return;
}

/*******************************************************************************/
void st25r3916IRQCallbackSet( void (*cb)(void) )
{
    st25r3916interrupt.prevCallback = st25r3916interrupt.callback;
    st25r3916interrupt.callback     = cb;
}

/*******************************************************************************/
void st25r3916IRQCallbackRestore( void )
{
    st25r3916interrupt.callback     = st25r3916interrupt.prevCallback;
    st25r3916interrupt.prevCallback = NULL;
}

