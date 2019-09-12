/**
 * @file
 * @brief Defines the procedures to make operations with
 *          the BA412 DES core
 * @copyright Copyright (c) 2017-2018 Silex Insight. All Rights reserved
 */


#ifndef BA412_CONFIG_H
#define BA412_CONFIG_H

/** @brief BA412 offset for Configuration word in DMA Scatter-Gather Tag */
#define DES_OFFSET_CFG        0
/** @brief BA412 offset for Configuration word in DMA Scatter-Gather Tag */
#define DES_OFFSET_KEY        4
/** @brief BA412 offset for Configuration word in DMA Scatter-Gather Tag */
#define DES_OFFSET_IV        28

/** @brief BA412 Mode Register value for encryption mode */
#define DES_MODEID_ENCRYPT    0x00000000
/** @brief BA412 Mode Register value for decryption mode */
#define DES_MODEID_DECRYPT    0x00000010
/** @brief BA412 Mode Register value for ECB mode of operation */
#define DES_MODEID_ECB        0x00000000
/** @brief BA412 Mode Register value for CBC mode of operation */
#define DES_MODEID_CBC        0x00000004
/** @brief BA412 Mode Register value for MAC mode of operation */
#define DES_MODEID_MAC        0x00000008
/** @brief BA412 Mode Register value for Triple DES keysize */
#define DES_MODEID_TDES       0x00000003

#endif //BA412_CONFIG_H
