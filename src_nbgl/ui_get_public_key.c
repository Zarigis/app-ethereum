#include <nbgl_page.h>
#include "shared_context.h"
#include "ui_callbacks.h"
#include "ui_nbgl.h"
#include "network.h"
#include "network_icons.h"

static char previousAddress[41];

// was the previously-requested public address used in a transaction?
// cleared if a different address is used in a tx or if a different
// public key is requested
static bool addressWasUsed;

bool shouldSkipPubkeyConfirm(){
    return addressWasUsed && strncmp(previousAddress,tmpCtx.publicKeyContext.address,41) == 0;
}

void setPreviousAddress(){
    if (strncmp(previousAddress,tmpCtx.publicKeyContext.address,41) == 0) {
        return;
    } else {
        strcpy(previousAddress, tmpCtx.publicKeyContext.address);
        addressWasUsed = false;
    }
    
}

void setAddressUsed(){
    if (strncmp(previousAddress,strings.common.fromAddress + 2,41) == 0){
        addressWasUsed = true;
    } else {
        addressWasUsed = false;
    }
}

static void review_choice(bool confirm) {
    if (confirm) {
        setPreviousAddress();
        io_seproxyhal_touch_address_ok(NULL);
    } else {
        io_seproxyhal_touch_address_cancel(NULL);
    }
}

void ui_display_public_key(const uint64_t *chain_id) {


    const nbgl_icon_details_t *icon;

    // - if a chain_id is given and it's - known, we specify its network name
    //                                   - unknown, we don't specify anything
    // - if no chain_id is given we specify the APPNAME (legacy behaviour)
    strlcpy(g_stax_shared_buffer, "Verify ", sizeof(g_stax_shared_buffer));
    if (chain_id != NULL) {
        if (chain_is_ethereum_compatible(chain_id)) {
            strlcat(g_stax_shared_buffer,
                    get_network_name_from_chain_id(chain_id),
                    sizeof(g_stax_shared_buffer));
            strlcat(g_stax_shared_buffer, "\n", sizeof(g_stax_shared_buffer));
        }
        icon = get_network_icon_from_chain_id(chain_id);
    } else {
        strlcat(g_stax_shared_buffer, APPNAME "\n", sizeof(g_stax_shared_buffer));
        icon = get_app_icon(false);
    }
    strlcat(g_stax_shared_buffer, "address", sizeof(g_stax_shared_buffer));
    nbgl_useCaseChoice(&C_Warning_64px, "Derive public key?", strings.common.toAddress, "Allow", "Cancel", review_choice);
}
