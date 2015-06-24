#ifndef _SAM3X_DMA_H_
#define _SAM3X_DMA_H_



/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/


/**
 * @brief   STM32 DMA ISR function type.
 *
 * @param[in] p         parameter for the registered function
 * @param[in] flags     pre-shifted content of the ISR register, the bits
 *                      are aligned to bit zero
 */
typedef void (*sam3x_dmaisr_t)(uint32_t channel, uint32_t err, void *arg);


typedef struct _dmatrans {
    uint32_t srcbuf;
    uint32_t dstbuf;
    uint32_t ctrla;
    uint32_t ctrlb;
    uint32_t cfg;
} dmatrans_t;


#define DMA_M2M_TRANSFER(dmat,size,from,to)\
    (dmat)->srcbuf = (uint32_t)(from); \
    (dmat)->dstbuf = (uint32_t)(to); \
    (dmat)->ctrla = DMAC_CTRLA_BTSIZE(size) | DMAC_CTRLA_SRC_WIDTH_BYTE | DMAC_CTRLA_DST_WIDTH_BYTE; \
    (dmat)->ctrlb = DMAC_CTRLB_SRC_DSCR_FETCH_DISABLE | DMAC_CTRLB_DST_DSCR_FETCH_DISABLE | \
                    DMAC_CTRLB_FC_MEM2MEM_DMA_FC | DMAC_CTRLB_SRC_INCR_INCREMENTING | DMAC_CTRLB_DST_INCR_INCREMENTING


#define DMA_M2P_TRANSFER(dmat,size,to,buf)\
    (dmat)->srcbuf = (uint32_t)(buf); \
    (dmat)->dstbuf = (uint32_t)(to); \
    (dmat)->ctrla = DMAC_CTRLA_BTSIZE(size) | DMAC_CTRLA_SRC_WIDTH_BYTE | DMAC_CTRLA_DST_WIDTH_BYTE; \
    (dmat)->ctrlb = DMAC_CTRLB_SRC_DSCR_FETCH_DISABLE | DMAC_CTRLB_DST_DSCR_FETCH_DISABLE | \
                    DMAC_CTRLB_FC_MEM2PER_DMA_FC | DMAC_CTRLB_SRC_INCR_INCREMENTING | DMAC_CTRLB_DST_INCR_FIXED

#define DMA_P2M_TRANSFER(dmat,size,to,buf)\
    (dmat)->srcbuf = (uint32_t)(buf); \
    (dmat)->dstbuf = (uint32_t)(to); \
    (dmat)->ctrla = DMAC_CTRLA_BTSIZE(size) | DMAC_CTRLA_SRC_WIDTH_BYTE | DMAC_CTRLA_DST_WIDTH_BYTE; \
    (dmat)->ctrlb = DMAC_CTRLB_SRC_DSCR_FETCH_DISABLE | DMAC_CTRLB_DST_DSCR_FETCH_DISABLE | \
                    DMAC_CTRLB_FC_PER2MEM_DMA_FC | DMAC_CTRLB_DST_INCR_INCREMENTING | DMAC_CTRLB_SRC_INCR_FIXED


#define DMA_M2P_TRANSFER_EXT(dmat,size,from,to,nil_a,nil_b)\
    (dmat)->srcbuf = (uint32_t)(from); \
    (dmat)->dstbuf = (uint32_t)(to); \
    (dmat)->ctrla = DMAC_CTRLA_BTSIZE(size) | DMAC_CTRLA_SRC_WIDTH_BYTE | DMAC_CTRLA_DST_WIDTH_BYTE; \
    (dmat)->ctrlb = DMAC_CTRLB_SRC_DSCR_FETCH_DISABLE | DMAC_CTRLB_DST_DSCR_FETCH_DISABLE | \
                    DMAC_CTRLB_FC_MEM2PER_DMA_FC | \
                    ( (nil_a) ? (DMAC_CTRLB_SRC_INCR_FIXED):(DMAC_CTRLB_SRC_INCR_INCREMENTING)) | \
                    ( (nil_b) ? (DMAC_CTRLB_DST_INCR_FIXED):(DMAC_CTRLB_DST_INCR_INCREMENTING))

#define DMA_P2M_TRANSFER_EXT(dmat,size,from,to,nil_a,nil_b)\
    (dmat)->srcbuf = (uint32_t)(from); \
    (dmat)->dstbuf = (uint32_t)(to); \
    (dmat)->ctrla = DMAC_CTRLA_BTSIZE(size) | DMAC_CTRLA_SRC_WIDTH_BYTE | DMAC_CTRLA_DST_WIDTH_BYTE; \
    (dmat)->ctrlb = DMAC_CTRLB_SRC_DSCR | DMAC_CTRLB_DST_DSCR | \
                    DMAC_CTRLB_FC_PER2MEM_DMA_FC | \
                    ( (nil_a) ? (DMAC_CTRLB_SRC_INCR_FIXED):(DMAC_CTRLB_SRC_INCR_INCREMENTING)) | \
                    ( (nil_b) ? (DMAC_CTRLB_DST_INCR_FIXED):(DMAC_CTRLB_DST_INCR_INCREMENTING))


#define DMA_PMCFG_SETUP(dmat,ph) (dmat)->cfg = DMAC_CFG_SRC_PER(ph) | DMAC_CFG_SRC_H2SEL | DMAC_CFG_SOD | DMAC_CFG_FIFOCFG_ALAP_CFG
#define DMA_MPCFG_SETUP(dmat,ph) (dmat)->cfg = DMAC_CFG_DST_PER(ph) | DMAC_CFG_DST_H2SEL | DMAC_CFG_SOD | DMAC_CFG_FIFOCFG_ALAP_CFG
#define DMA_MCFG_SETUP(dmat) (dmat)->cfg = DMAC_CFG_SOD_ENABLE | DMAC_CFG_AHB_PROT(1) | DMAC_CFG_FIFOCFG_ALAP_CFG


#define DMA_FIXED 1
#define DMA_INCR 0


/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/


#define Dmac DMAC

/**
 * \brief Set DMA priority mode.
 *
 * \param p_dmac Pointer to a DMAC peripheral instance.
 * \param mode   Priority mode.
 */
#define dma_set_priority_mode(mode) (Dmac->DMAC_GCFG = (Dmac->DMAC_GCFG & (~DMAC_GCFG_ARB_CFG)) | (mode))

/**
 * \brief Enable DMA Controller.
 *
 * \param p_dmac  Pointer to a DMAC peripheral instance.
 */
#define dma_enable() Dmac->DMAC_EN = DMAC_EN_ENABLE

/**
 * \brief Disable DMA Controller.
 *
 * \param p_dmac Pointer to a DMAC peripheral instance.
 */
#define dma_disable() Dmac->DMAC_EN &= (~DMAC_EN_ENABLE)

/**
 * \brief Enable DMA interrupt.
 *
 * \param p_dmac  Pointer to a DMAC peripheral instance.
 * \param ul_mask Interrupt to be enabled.
 */
#define dma_enable_interrupt(ch) Dmac->DMAC_EBCIER = (1u<<(ch))

/**
 * \brief Disable DMA interrupt.
 *
 * \param p_dmac  Pointer to a DMAC peripheral instance.
 * \param ul_mask Interrupt to be disabled.
 */
#define dma_disable_interrupt(ch) Dmac->DMAC_EBCIDR = (1u<<(ch))


/**
 * \brief Get DMAC Interrupt Mask.
 *
 * \param p_dmac Pointer to a DMAC peripheral instance.
 *
 * \return DMAC Interrupt mask.
 */
#define dma_get_interrupt_mask()  (Dmac->DMAC_EBCIMR)


/**
 * \brief Get DMAC transfer status.
 *
 * \param p_dmac Pointer to a DMAC peripheral instance.
 *
 * \return DMAC transfer status.
 */
#define dma_get_status() Dmac->DMAC_EBCISR

/**
 * \brief Enable the relevant channel.
 *
 * \param p_dmac Pointer to a DMAC peripheral instance.
 * \param ul_num Channel number.
 */
#define dma_channel_enable(ch)  Dmac->DMAC_CHER = DMAC_CHER_ENA0 << (ch)

/**
 * \brief Disable the relevant channel.
 *
 * \param p_dmac Pointer to a DMAC peripheral instance.
 * \param ul_num Channel number.
 */
#define dma_channel_disable(ch)  Dmac->DMAC_CHDR = DMAC_CHDR_DIS0 << (ch)

/**
 * \brief Check if the relevant channel is enabled.
 *
 * \param p_dmac Pointer to a DMAC peripheral instance.
 * \param ul_num Channel number.
 *
 * \retval 0: disabled.
 * \retval 1: enabled.
 */
#define dma_channel_is_enabled(ch) ((Dmac->DMAC_CHSR & (DMAC_CHSR_ENA0 << (ch))) ? 1:0)

/**
 * \brief Suspend the specified channel and its current context.
 *
 * \param p_dmac Pointer to a DMAC peripheral instance.
 * \param ul_num Channel number.
 */
#define dma_channel_suspend(ch) Dmac->DMAC_CHER = DMAC_CHER_SUSP0 << (ch)

/**
 * \brief Resume the specified channel transfer (restoring its context).
 *
 * \param p_dmac Pointer to a DMAC peripheral instance.
 * \param ul_num Channel number.
 */
#define dma_channel_resume(ch) Dmac->DMAC_CHDR = DMAC_CHDR_RES0 << (ch)

/**
 * \brief Resume the specified channel from an automatic stall state.
 *
 * \param p_dmac Pointer to a DMAC peripheral instance.
 * \param ul_num Channel number.
 */
#define dma_channel_keep(ch) Dmac->DMAC_CHER = DMAC_CHER_KEEP0 << (ch)

/**
 * \brief Get DMAC channel handler status.
 *
 * \param p_dmac Pointer to a DMAC peripheral instance.
 *
 * \return DMAC channel handler status register.
 */
#define dma_channel_get_status() (Dmac->DMAC_CHSR)


/**
 * \brief Set DMAC source address of the DMAC channel.
 *
 * \param p_dmac      Pointer to a DMAC peripheral instance.
 * \param ul_num      Channel number.
 * \param ul_addr     Source address.
 *
 * \note This register must be aligned with the source transfer width.
 */
#define dma_channel_set_source_addr(ch, addr) Dmac->DMAC_CH_NUM[ch].DMAC_SADDR = (addr)


/**
 * \brief Set DMAC destination address of the DMAC channel.
 *
 * \param p_dmac      Pointer to a DMAC peripheral instance.
 * \param ul_num      Channel number.
 * \param ul_addr     Destination address.
 *
 * \note This register must be aligned with the source transfer width.
 */
#define dma_channel_set_destination_addr(ch, addr) Dmac->DMAC_CH_NUM[ch].DMAC_DADDR = (addr)

/**
 * \brief Set DMAC descriptor address of the DMAC channel.
 *
 * \param p_dmac      Pointer to a DMAC peripheral instance.
 * \param ul_num      Channel number.
 * \param ul_desc     Descriptor address.
 */
#define dma_channel_set_descriptor_addr(ch, desc) Dmac->DMAC_CH_NUM[ch].DMAC_DSCR = (desc)

/**
 * \brief Set DMAC control A of the DMAC channel.
 *
 * \param p_dmac      Pointer to a DMAC peripheral instance.
 * \param ul_num      Channel number.
 * \param ul_ctrlA    Configuration of control A register.
 */
#define dma_channel_set_ctrlA(ch, ctrlA) Dmac->DMAC_CH_NUM[ch].DMAC_CTRLA = (ctrlA)


/**
 * \brief Set DMAC control B of the DMAC channel.
 *
 * \param p_dmac      Pointer to a DMAC peripheral instance.
 * \param ul_num      Channel number.
 * \param ul_ctrlB    Configuration of control B register.
 */
#define dma_channel_set_ctrlB(ch, ctrlB) Dmac->DMAC_CH_NUM[ch].DMAC_CTRLB = (ctrlB)

/**
 * \brief Set DMAC configuration register of the DMAC channel.
 *
 * \param p_dmac      Pointer to a DMAC peripheral instance.
 * \param ul_num      Channel number.
 * \param ul_cfg      Configuration of CFG register.
 */
#define dma_channel_set_configuration(ch,cfg) Dmac->DMAC_CH_NUM[ch].DMAC_CFG = (cfg)



/**
 * \brief Check if the transfer of the DMAC channel is done.
 * This function is used for polling mode.
 *
 * \param p_dmac      Pointer to a DMAC peripheral instance.
 * \param ul_num      Channel number.
 *
 * \retval 0 - Transferring.
 * \retval 1 - Transfer is done.
 */
#define dma_channel_is_transfer_done(ch) ((dma_channel_get_status() & (DMAC_CHSR_ENA0 << (ch)))  ? 0:1)



#define dmaStart(ch) dma_channel_enable(ch)
#define dmaStop(ch) dma_channel_disable(ch)
#define dmaEnsureStop(ch) dmaStop(ch)
// do { dma_channel_disable(ch); while(dma_channel_is_enabled(ch));} while(0)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/


#ifdef __cplusplus
extern "C" {
#endif
void dmaInit(void);
int dmaChannelActivate(uint32_t channel, sam3x_dmaisr_t isr, void *arg, dmatrans_t *dmat);
void dmaChannelDeactivate(uint32_t channel);
void dmaSetupTransfer(uint32_t channel, dmatrans_t *dmat);
#ifdef __cplusplus
}
#endif


#endif