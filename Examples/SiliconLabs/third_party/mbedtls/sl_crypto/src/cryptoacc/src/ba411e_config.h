/**
 * @file
 * @brief Defines macros to be used for the configuration of the BA411E AES
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */


#ifndef BA411E_CONFIG_H
#define BA411E_CONFIG_H

/** @brief BA411E offset for Configuration word in DMA Scatter-Gather Tag */
#define AES_OFFSET_CFG        0
/** @brief BA411E offset for Configuration word in DMA Scatter-Gather Tag */
#define AES_OFFSET_KEY        8
/** @brief BA411E offset for Configuration word in DMA Scatter-Gather Tag */
#define AES_OFFSET_IV        40
/** @brief BA411E offset for Configuration word in DMA Scatter-Gather Tag */
#define AES_OFFSET_IV2       56
/** @brief BA411E offset for Configuration word in DMA Scatter-Gather Tag */
#define AES_OFFSET_KEY2      72
/** @brief BA411E offset for Configuration word in DMA Scatter-Gather Tag */
#define AES_OFFSET_MASK      104

/** @brief BA411E Mode Register value for ECB mode of operation */
#define AES_MODEID_ECB        0x00000100
/** @brief BA411E Mode Register value for CBC mode of operation */
#define AES_MODEID_CBC        0x00000200
/** @brief BA411E Mode Register value for CTR mode of operation */
#define AES_MODEID_CTR        0x00000400
/** @brief BA411E Mode Register value for CFB mode of operation */
#define AES_MODEID_CFB        0x00000800
/** @brief BA411E Mode Register value for OFB mode of operation */
#define AES_MODEID_OFB        0x00001000
/** @brief BA411E Mode Register value for CCM mode of operation */
#define AES_MODEID_CCM        0x00002000
/** @brief BA411E Mode Register value for GCM mode of operation */
#define AES_MODEID_GCM        0x00004000
/** @brief BA411E Mode Register value for XTS mode of operation */
#define AES_MODEID_XTS        0x00008000
/** @brief BA411E Mode Register value for CMAC mode of operation */
#define AES_MODEID_CMA        0x00010000
/** @brief BA411E Mode Register value for AES context saving */
#define AES_MODEID_CX_SAVE    0x00000020
/** @brief BA411E Mode Register value for AES context loading */
#define AES_MODEID_CX_LOAD    0x00000010
/** @brief BA411E Mode Register value for AES no context */
#define AES_MODEID_NO_CX      0x00000000
/** @brief BA411E Mode Register value for AES keysize of 128 bits */
#define AES_MODEID_AES128     0x00000000
/** @brief BA411E Mode Register value for AES keysize of 192 bits */
#define AES_MODEID_AES192     0x00000008
/** @brief BA411E Mode Register value for AES keysize of 256 bits */
#define AES_MODEID_AES256     0x00000004
/** @brief BA411E Mode Register value for encryption mode */
#define AES_MODEID_ENCRYPT    0x00000000
/** @brief BA411E Mode Register value for decryption mode */
#define AES_MODEID_DECRYPT    0x00000001
/** @brief BA411E Mode Register value to use Key1 */
#define AES_MODEID_KEY1       0x00000040
/** @brief BA411E Mode Register value to use Key2 */
#define AES_MODEID_KEY2       0x00000080


/** @brief BA411E Mode Register mask for hardware key 1 & 2*/
#define AES_MODEID_KEYX_MASK     0x000000C0


/* AES hardware configuration - register 1*/
#define AES_HW_CFG_ECB_SUPPORTED_LSB      0
#define AES_HW_CFG_ECB_SUPPORTED_MASK     (1L<<AES_HW_CFG_ECB_SUPPORTED_LSB)
#define AES_HW_CFG_CBC_SUPPORTED_LSB      1
#define AES_HW_CFG_CBC_SUPPORTED_MASK     (1L<<AES_HW_CFG_CBC_SUPPORTED_LSB)
#define AES_HW_CFG_CTR_SUPPORTED_LSB      2
#define AES_HW_CFG_CTR_SUPPORTED_MASK     (1L<<AES_HW_CFG_CTR_SUPPORTED_LSB)
#define AES_HW_CFG_CFB_SUPPORTED_LSB      3
#define AES_HW_CFG_CFB_SUPPORTED_MASK     (1L<<AES_HW_CFG_CFB_SUPPORTED_LSB)
#define AES_HW_CFG_OFB_SUPPORTED_LSB      4
#define AES_HW_CFG_OFB_SUPPORTED_MASK     (1L<<AES_HW_CFG_OFB_SUPPORTED_LSB)
#define AES_HW_CFG_CCM_SUPPORTED_LSB      5
#define AES_HW_CFG_CCM_SUPPORTED_MASK     (1L<<AES_HW_CFG_CCM_SUPPORTED_LSB)
#define AES_HW_CFG_GCM_SUPPORTED_LSB      6
#define AES_HW_CFG_GCM_SUPPORTED_MASK     (1L<<AES_HW_CFG_GCM_SUPPORTED_LSB)
#define AES_HW_CFG_XTS_SUPPORTED_LSB      7
#define AES_HW_CFG_XTS_SUPPORTED_MASK     (1L<<AES_HW_CFG_GCM_SUPPORTED_LSB)
#define AES_HW_CFG_CMAC_SUPPORTED_LSB     8
#define AES_HW_CFG_CMAC_SUPPORTED_MASK    (1L<<AES_HW_CFG_CMAC_SUPPORTED_LSB)
#define AES_HW_CFG_CS_EN_LSB              9 /* Ciphertext Stealing */
#define AES_HW_CFG_CS_EN_MASK             (1L<<AES_HW_CFG_CS_EN_LSB)
#define AES_HW_CFG_CM_EN_LSB              10 /* Countermeasures */
#define AES_HW_CFG_CM_EN_MASK             (1L<<AES_HW_CFG_CM_EN_LSB)
#define AES_HW_CFG_KEY_SIZE_LSB           24
#define AES_HW_CFG_KEY_SIZE_MASK          (0x7<<AES_HW_CFG_KEY_SIZE_LSB)
#define AES_HW_CFG_KEY_SIZE_128_SUPPORTED_LSB 24
#define AES_HW_CFG_KEY_SIZE_128_SUPPORTED_MASK (1L<<AES_HW_CFG_KEY_SIZE_128_SUPPORTED_LSB)
#define AES_HW_CFG_KEY_SIZE_192_SUPPORTED_LSB 25
#define AES_HW_CFG_KEY_SIZE_192_SUPPORTED_MASK (1L<<AES_HW_CFG_KEY_SIZE_192_SUPPORTED_LSB)
#define AES_HW_CFG_KEY_SIZE_256_SUPPORTED_LSB 25
#define AES_HW_CFG_KEY_SIZE_256_SUPPORTED_MASK (1L<<AES_HW_CFG_KEY_SIZE_256_SUPPORTED_LSB)

#define BA411E_HW_CFG_1 (*(const volatile uint32_t*)ADDR_BA411E_HW_CFG_1)

/* AES hardware configuration - register 2*/
#define AES_HW_CFG_MAX_CTR_SIZE_LSB       0
#define AES_HW_CFG_MAX_CTR_SIZE_MASK      (0xFFFF<<AES_HW_CFG_MAX_CTR_SIZE_LSB)

#define BA411E_HW_CFG_2 (*(const volatile uint32_t*)ADDR_BA411E_HW_CFG_2)

#endif
