/*****************************************************************************
 *   Ledger App Boilerplate.
 *   (c) 2020 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#include <stdint.h>   // uint*_t
#include <string.h>   // memset, explicit_bzero
#include <stdbool.h>  // bool

#include "crypto.h"

#include "globals.h"

int crypto_derive_private_key(cx_ecfp_private_key_t *private_key,
                              uint8_t chain_code[static 32],
                              const uint32_t *bip32_path,
                              uint8_t bip32_path_len) {
    uint8_t raw_private_key[32] = {0};
    int error = 0;

    BEGIN_TRY {
        TRY {
            // derive the seed with bip32_path
            os_perso_derive_node_bip32(CX_CURVE_256K1,
                                       bip32_path,
                                       bip32_path_len,
                                       raw_private_key,
                                       chain_code);
            // new private_key from raw
            cx_ecfp_init_private_key(CX_CURVE_256K1,
                                     raw_private_key,
                                     sizeof(raw_private_key),
                                     private_key);
        }
        CATCH_OTHER(e) {
            error = e;
        }
        FINALLY {
            explicit_bzero(&raw_private_key, sizeof(raw_private_key));
        }
    }
    END_TRY;

    return error;
}

void crypto_init_public_key(cx_ecfp_private_key_t *private_key,
                            cx_ecfp_public_key_t *public_key,
                            uint8_t raw_public_key[static 65]) {
    // generate corresponding public key
    cx_ecfp_generate_pair(CX_CURVE_256K1, public_key, private_key, 1);

    memmove(raw_public_key, public_key->W , 65);
}

void crypto_compress_public_key(uint8_t *public_key, uint8_t *compressed_public_key) {
  memmove(compressed_public_key, public_key, 33);
  compressed_public_key[0] = ((public_key[64] & 1) ? 0x03 : 0x02);
}


void format_signature_out() {
	uint8_t buff[MAX_DER_SIG_LEN]; 
	uint8_t *signature = G_context.tx_info.signature;
	uint8_t buff_offset = 0;
    uint8_t xoffset = 4;  // point to r value

    uint8_t xlength = signature[xoffset - 1];
    if (xlength == 33) {
        xlength = 32;
        xoffset++;
    }
	
    memmove(buff + buff_offset,  signature + xoffset, xlength);
	buff_offset += xlength;
    xoffset += xlength + 2;  // move over rvalue and TagLEn
    // copy s value
    xlength = signature[xoffset - 1];
    if (xlength == 33) {
        xlength = 32;
        xoffset++;
    }

	
    memmove(buff + buff_offset, signature + xoffset, xlength);
	buff_offset += buff_offset;


	memmove(G_context.tx_info.signature,  buff, buff_offset);
	G_context.tx_info.signature_len = buff_offset;
}



int crypto_sign_message(bool is_format) {
    cx_ecfp_private_key_t private_key = {0};
    uint32_t info = 0;
    int sig_len = 0;


	

    // derive private key according to BIP32 path
    int error = crypto_derive_private_key(&private_key,
                                          G_context.pk_info.chain_code,
                                          G_context.bip32_path,
                                          G_context.bip32_path_len);
    if (error != 0) {
        return error;
    }

	//PRINTF("SIGN-PK: %d: %.*H\n",private_key.curve, private_key.d_len, private_key.d);

    BEGIN_TRY {
        TRY {
            sig_len = cx_ecdsa_sign(&private_key,
                                    CX_RND_RFC6979 | CX_LAST,
                                    CX_SHA256,
                                    G_context.tx_info.m_hash,
                                    sizeof(G_context.tx_info.m_hash),
                                    G_context.tx_info.signature,
                                    sizeof(G_context.tx_info.signature),
                                    &info);
            PRINTF("Signature: %.*H\n", sig_len, G_context.tx_info.signature);
        }
        CATCH_OTHER(e) {
            error = e;
        }
        FINALLY {
            explicit_bzero(&private_key, sizeof(private_key));
        }
    }
    END_TRY;

    if (error == 0) {
		if(is_format) {
				format_signature_out();
				
				G_context.tx_info.signature[G_context.tx_info.signature_len] = 27;
				
				if (info & CX_ECCINFO_PARITY_ODD) {
					G_context.tx_info.signature[G_context.tx_info.signature_len] += 1;
				}
				if (info & CX_ECCINFO_xGTn) {
					G_context.tx_info.signature[G_context.tx_info.signature_len] += 2;
				}

				G_context.tx_info.signature_len +=1;

		} else {
			G_context.tx_info.signature_len = sig_len;
			G_context.tx_info.v = (uint8_t)(info & CX_ECCINFO_PARITY_ODD);

		}

    }

    return error;
}
