
#include "ch.h"
#include "hal.h"


/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define DMA_CHANNELS 5

#define DMA_CH_INACTIVE   0
#define DMA_CH_ACTIVE     1
#define DMA_CH_STARTED    2
#define DMA_CH_FINISHED   3

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/


/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

static sam3x_dmaisr_t dma_callbacks[DMA_CHANNELS];
static void *dma_callbacks_args[DMA_CHANNELS];
static uint8_t dma_status[DMA_CHANNELS];

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/* ISR reg is organized as follows in SAM3X:
    [0:5] -> BTC ( buffer transfer complete )
    [8:13] -> CBTC ( chained buffer transfer complete: don't used by this driver)
    [16:21] -> ERR ( eventual r/w error )
*/

#define BTC_CH(ch) (1u<< (ch))
#define ERR_CH(ch) (1u<< ((ch)+16))

CH_IRQ_HANDLER(DMA_Handler) {
    uint32_t status;
    uint32_t i;
    CH_IRQ_PROLOGUE();
    status = dma_get_status();

    for (i = 0; i < DMA_CHANNELS; i++) {
        if ((status & BTC_CH(i)) || (status & ERR_CH(i))) {
            /* BTC bit set, do callback */
            if (dma_callbacks[i] && dma_status[i]) {
                dma_callbacks[i](i, status & ERR_CH(i),dma_callbacks_args[i]);
                dma_status[i] = DMA_CH_FINISHED;
            }
        }
    }

    CH_IRQ_EPILOGUE();
}



/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/


void dmaInit(void) {
    int i;
    for (i = 0; i < DMA_CHANNELS; i++) {
        dma_callbacks[i] = NULL;
        dma_status[i] = DMA_CH_INACTIVE;
    }
    pmc_enable_periph_clk(ID_DMAC);
    dma_disable();
    dma_set_priority_mode(DMAC_GCFG_ARB_CFG_ROUND_ROBIN);
    dma_enable();

    nvicEnableVector(DMAC_IRQn, CORTEX_PRIORITY_MASK(SAM3X_DMA_IRQ_PRIORITY));

}


int dmaChannelActivate(uint32_t channel, sam3x_dmaisr_t isr, void* arg, dmatrans_t *dmat) {
    /* channel already active */
    if (dma_status[channel])
        return 1;

    dma_status[channel] = DMA_CH_ACTIVE;
    dma_callbacks[channel] = isr;
    dma_callbacks_args[channel] = arg;

    /* configure channel */
    dma_channel_set_configuration(channel, dmat->cfg);
    dma_enable_interrupt(channel);

    return 0;
}

void dmaChannelDeactivate(uint32_t channel) {
    if (dma_status[channel]) {
        dma_status[channel] = DMA_CH_INACTIVE;
        dma_disable_interrupt(channel);
        dma_channel_disable(channel);
        /* wait for eventual pending operation to finish */
        while (dma_channel_is_enabled(channel));
    }
}


/**
 * \brief Initialize DMAC channel of single buffer transfer.
 *
 * \param p_dmac      Pointer to a DMAC peripheral instance.
 * \param ul_num      Channel number.
 * \param p_desc      Pointer to a transfer descriptor.
 */
void dmaSetupTransfer(uint32_t channel, dmatrans_t *dmat) {

    /* Clear any pending interrupts */
    Dmac->DMAC_EBCISR;

    dma_channel_set_source_addr(channel, dmat->srcbuf);
    dma_channel_set_destination_addr(channel, dmat->dstbuf);
    dma_channel_set_descriptor_addr(channel, 0);
    dma_channel_set_ctrlA(channel, dmat->ctrla);
    dma_channel_set_ctrlB(channel, dmat->ctrlb);
    dma_channel_set_configuration(channel, dmat->cfg);
    

    dma_status[channel] = DMA_CH_STARTED;
}



