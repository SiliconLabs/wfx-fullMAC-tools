//                                                                ------------------------------------------------------------------------------
//                                                                 Copyright (c) 2011 Qualcomm Atheros, Inc.
//                                                                 All Rights Reserved.
//                                                                 Qualcomm Atheros Confidential and Proprietary.
//                                                                 Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is
//                                                                 hereby granted, provided that the above copyright notice and this permission notice appear in all copies.
//
//                                                                 THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
//                                                                 INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//                                                                 ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
//                                                                 USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//                                                                 OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//                                                                ------------------------------------------------------------------------------
//                                                                ==============================================================================
//                                                                 Author(s): ="Atheros"
//                                                                ==============================================================================

#define __VER_MAJOR_ 3  // corresponds to release X._._._._
#define __VER_MINOR_ 3  // corresponds to release _.X._._._
#define __VER_PATCH_ 0  // corresponds to release _._.X._.
#define __VER_REV_   0 // corresponds to release _._._.X._   <-- Increment This field manually for every build

/* The makear6ksdk script (used for release builds) modifies the following line. */
#define __BUILD_NUMBER_ 0

/* Format of the version number. */
#define VER_MAJOR_BIT_OFFSET        28
#define VER_MINOR_BIT_OFFSET        24
#define VER_PATCH_BIT_OFFSET        16
#define VER_BUILD_NUM_BIT_OFFSET    0

/*
 * The version has the following format:
 * Bits 28-31: Major version
 * Bits 24-27: Minor version
 * Bite 18-23: Patch number
 * Bits 8-17: Revision number (Increment this every build)
 * Bits 0-7:  build number which is automatically updated
 *
 * DO NOT split the following macro into multiple lines as this may confuse the build scripts.
 */
#define AR6K_SW_VERSION     ( (__VER_MAJOR_ << VER_MAJOR_BIT_OFFSET) + (__VER_MINOR_ << VER_MINOR_BIT_OFFSET)  + ( (__VER_PATCH_ & 0xFF) << VER_PATCH_BIT_OFFSET)  +  ( ((__BUILD_NUMBER_) & 0xFFFF) << VER_BUILD_NUM_BIT_OFFSET) )

/* ABI Version. Reflects the version of binary interface exposed by AR6K target firmware. Needs to be incremented by 1 for any change in the firmware that requires upgrade of the driver on the host side for the change to work correctly */
#define ABI_VERSION_MAJOR    (1)
#define ABI_VERSION_MINOR    (0)
#define ABI_MAJOR_OFFSET     (16)
#define ABI_MAJOR_MASK       (0xffff0000)
#define ABI_MINOR_OFFSET     (0)
#define ABI_MINOR_MASK       (0x0000ffff)
#define AR6K_ABI_VERSION ((ABI_VERSION_MAJOR << ABI_MAJOR_OFFSET) | (ABI_VERSION_MINOR << ABI_MINOR_OFFSET))
