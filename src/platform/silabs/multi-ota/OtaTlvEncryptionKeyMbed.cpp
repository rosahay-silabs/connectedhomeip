#include "OtaTlvEncryptionKey.h"
#include <lib/support/CHIPLogging.h>
#include <lib/support/CodeUtils.h>
#include <platform/silabs/SilabsConfig.h>

#include "mbedtls/aes.h"

#include <string.h>

namespace chip {
namespace DeviceLayer {
namespace Silabs {
namespace OtaTlvEncryptionKey {

static CHIP_ERROR OtaTlvEncryptionKey::Decrypt(const ByteSpan & key, MutableByteSpan & block, uint32_t & mIVOffset)
{
    uint8_t iv[16];
    mbedtls_aes_context aes_ctx;
    uint32_t u32IVCount;
    uint32_t Offset = 0;

    memcpy(iv, au8Iv, sizeof(au8Iv));

    u32IVCount = (((uint32_t) iv[12]) << 24) | (((uint32_t) iv[13]) << 16) | (((uint32_t) iv[14]) << 8) | (iv[15]);
    u32IVCount += (mIVOffset >> 4);

    iv[12] = (uint8_t) ((u32IVCount >> 24) & 0xff);
    iv[13] = (uint8_t) ((u32IVCount >> 16) & 0xff);
    iv[14] = (uint8_t) ((u32IVCount >> 8) & 0xff);
    iv[15] = (uint8_t) (u32IVCount & 0xff);

    mbedtls_aes_init(&aes_ctx);

    // Set the AES decryption key
    if (mbedtls_aes_setkey_dec(&aes_ctx, key.value(), (kAES_CTR128_Key_Length * 8u)) != 0)
    {
        ChipLogError(DeviceLayer, "Failed to set AES decryption key");
        mbedtls_aes_free(&aes_ctx);
        return CHIP_ERROR_INTERNAL;
    }

    while (Offset + 16 <= block.size())
    {
        // Decrypt the block
        if (mbedtls_aes_crypt_ctr(&aes_ctx, 16, &u32IVCount, iv, iv, &block[Offset], &block[Offset]) != 0)
        {
            ChipLogError(DeviceLayer, "Failed to decrypt block");
            mbedtls_aes_free(&aes_ctx);
            return CHIP_ERROR_INTERNAL;
        }

        /* Increment the IV for the next block */
        u32IVCount++;

        iv[12] = (uint8_t) ((u32IVCount >> 24) & 0xff);
        iv[13] = (uint8_t) ((u32IVCount >> 16) & 0xff);
        iv[14] = (uint8_t) ((u32IVCount >> 8) & 0xff);
        iv[15] = (uint8_t) (u32IVCount & 0xff);

        Offset += 16; /* Increment the buffer offset */
        mIVOffset += 16;
    }

    ChipLogProgress(DeviceLayer, "Decrypted ciphertext");

    mbedtls_aes_free(&aes_ctx);

    return CHIP_NO_ERROR;
}

} // namespace OtaTlvEncryptionKey
} // namespace Silabs
} // namespace DeviceLayer
} // namespace chip
