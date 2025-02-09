#include "shared_context.h"
#include "apdu_constants.h"
#include "common_utils.h"
#include "feature_getPublicKey.h"
#include "common_ui.h"
#include "os_io_seproxyhal.h"
#include "crypto_helpers.h"

void handleGetPublicKey(uint8_t p1,
                        uint8_t p2,
                        const uint8_t *dataBuffer,
                        uint8_t dataLength,
                        unsigned int *flags,
                        __attribute__((unused)) unsigned int *tx) {
    bip32_path_t bip32;

    if (!G_called_from_swap) {
        reset_app_context();
    }

    if ((p1 != P1_CONFIRM) && (p1 != P1_NON_CONFIRM)) {
        PRINTF("Error: Unexpected P1 (%u)!\n", p1);
        THROW(APDU_RESPONSE_INVALID_P1_P2);
    }
    if ((p2 != P2_CHAINCODE) && (p2 != P2_NO_CHAINCODE)) {
        PRINTF("Error: Unexpected P2 (%u)!\n", p2);
        THROW(APDU_RESPONSE_INVALID_P1_P2);
    }

    dataBuffer = parseBip32(dataBuffer, &dataLength, &bip32);

    if (dataBuffer == NULL) {
        THROW(APDU_RESPONSE_INVALID_DATA);
    }

    tmpCtx.publicKeyContext.getChaincode = (p2 == P2_CHAINCODE);
    if (bip32_derive_get_pubkey_256(
            CX_CURVE_256K1,
            bip32.path,
            bip32.length,
            tmpCtx.publicKeyContext.publicKey.W,
            (tmpCtx.publicKeyContext.getChaincode ? tmpCtx.publicKeyContext.chainCode : NULL),
            CX_SHA512) != CX_OK) {
        THROW(APDU_RESPONSE_UNKNOWN);
    }
    getEthAddressStringFromRawKey(tmpCtx.publicKeyContext.publicKey.W,
                                  tmpCtx.publicKeyContext.address,
                                  chainConfig->chainId);

    uint64_t chain_id = chainConfig->chainId;
    if (dataLength >= sizeof(chain_id)) {
        chain_id = u64_from_BE(dataBuffer, sizeof(chain_id));
        dataLength -= sizeof(chain_id);
        dataBuffer += sizeof(chain_id);
    }

    (void) dataBuffer;  // to prevent dead increment warning
    if (dataLength > 0) {
        PRINTF("Error: Leftover unwanted data (%u bytes long)!\n", dataLength);
        THROW(APDU_RESPONSE_INVALID_DATA);
    }
    
    if (p1 == P1_NON_CONFIRM && ((!N_storage.promptPubkey) || shouldSkipPubkeyConfirm())) {
        setPreviousAddress();
        *tx = set_result_get_publicKey();
        THROW(APDU_RESPONSE_OK);
    } else {
        snprintf(strings.common.toAddress,
             sizeof(strings.common.toAddress),
             "0x%.*s",
             40,
             tmpCtx.publicKeyContext.address);
        // don't unnecessarily pass the current app's chain ID
        ui_display_public_key(chainConfig->chainId == chain_id ? NULL : &chain_id);

        *flags |= IO_ASYNCH_REPLY;
    }
}
