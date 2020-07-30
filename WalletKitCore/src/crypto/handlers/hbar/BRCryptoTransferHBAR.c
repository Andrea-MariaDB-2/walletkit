//
//  BRCryptoTransferHBAR.c
//  Core
//
//  Created by Ehsan Rezaie on 2020-05-19.
//  Copyright © 2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#include "BRCryptoHBAR.h"
#include "crypto/BRCryptoAmountP.h"
#include "crypto/BRCryptoHashP.h"
#include "hedera/BRHederaTransaction.h"
#include "ethereum/util/BRUtilMath.h"

static BRCryptoTransferDirection
transferGetDirectionFromHBAR (BRHederaTransaction transaction,
                              BRHederaWallet wallet);

extern BRCryptoTransferHBAR
cryptoTransferCoerceHBAR (BRCryptoTransfer transfer) {
    assert (CRYPTO_NETWORK_TYPE_HBAR == transfer->type);
    return (BRCryptoTransferHBAR) transfer;
}

extern BRCryptoTransfer
cryptoTransferCreateAsHBAR (BRCryptoTransferListener listener,
                            BRCryptoUnit unit,
                            BRCryptoUnit unitForFee,
                            OwnershipKept BRHederaWallet wallet,
                            OwnershipGiven BRHederaTransaction hbarTransaction) {
    
    BRCryptoTransferDirection direction = transferGetDirectionFromHBAR (hbarTransaction, wallet);
    
    BRCryptoAmount amount = cryptoAmountCreateAsHBAR (unit,
                                                      CRYPTO_FALSE,
                                                      hederaTransactionGetAmount (hbarTransaction));
    
    BRCryptoAmount feeAmount = cryptoAmountCreateAsHBAR (unitForFee,
                                                         CRYPTO_FALSE,
                                                         hederaTransactionGetFee (hbarTransaction));
    BRCryptoFeeBasis feeBasisEstimated = cryptoFeeBasisCreate (feeAmount, 1.0);
    
    BRCryptoAddress sourceAddress = cryptoAddressCreateAsHBAR (hederaTransactionGetSource (hbarTransaction));
    BRCryptoAddress targetAddress = cryptoAddressCreateAsHBAR (hederaTransactionGetTarget (hbarTransaction));
    
    BRCryptoTransfer transferBase = cryptoTransferAllocAndInit (sizeof (struct BRCryptoTransferHBARRecord),
                                                                CRYPTO_NETWORK_TYPE_HBAR,
                                                                listener,
                                                                unit,
                                                                unitForFee,
                                                                feeBasisEstimated,
                                                                amount,
                                                                direction,
                                                                sourceAddress,
                                                                targetAddress);
    BRCryptoTransferHBAR transfer = cryptoTransferCoerceHBAR (transferBase);
    
    transfer->hbarTransaction = hbarTransaction;
    
    cryptoFeeBasisGive (feeBasisEstimated);
    cryptoAddressGive (sourceAddress);
    cryptoAddressGive (targetAddress);
    
    return (BRCryptoTransfer) transfer;
}

static void
cryptoTransferReleaseHBAR (BRCryptoTransfer transferBase) {
    BRCryptoTransferHBAR transfer = cryptoTransferCoerceHBAR(transferBase);
    hederaTransactionFree (transfer->hbarTransaction);
}

static BRCryptoHash
cryptoTransferGetHashHBAR (BRCryptoTransfer transferBase) {
    BRCryptoTransferHBAR transfer = cryptoTransferCoerceHBAR(transferBase);
    BRHederaTransactionHash hash = hederaTransactionGetHash (transfer->hbarTransaction);
    return cryptoHashCreateInternal (CRYPTO_NETWORK_TYPE_HBAR, sizeof (hash.bytes), hash.bytes);
}

static uint8_t *
cryptoTransferSerializeHBAR (BRCryptoTransfer transferBase,
                             BRCryptoNetwork network,
                             BRCryptoBoolean  requireSignature,
                             size_t *serializationCount) {
    assert (CRYPTO_TRUE == requireSignature);
    BRCryptoTransferHBAR transfer = cryptoTransferCoerceHBAR (transferBase);
    return hederaTransactionSerialize (transfer->hbarTransaction, serializationCount);
}

static int
cryptoTransferIsEqualHBAR (BRCryptoTransfer tb1, BRCryptoTransfer tb2) {
    return (tb1 == tb2 ||
            cryptoHashEqual (cryptoTransferGetHashHBAR(tb1),
                             cryptoTransferGetHashHBAR(tb2)));
}

static BRCryptoTransferDirection
transferGetDirectionFromHBAR (BRHederaTransaction transaction,
                              BRHederaWallet wallet) {
    BRHederaAddress source = hederaTransactionGetSource (transaction);
    BRHederaAddress target = hederaTransactionGetTarget (transaction);
    
    int isSource = hederaWalletHasAddress (wallet, source);
    int isTarget = hederaWalletHasAddress (wallet, target);
    
    return (isSource && isTarget
            ? CRYPTO_TRANSFER_RECOVERED
            : (isSource
               ? CRYPTO_TRANSFER_SENT
               : CRYPTO_TRANSFER_RECEIVED));
}

BRCryptoTransferHandlers cryptoTransferHandlersHBAR = {
    cryptoTransferReleaseHBAR,
    cryptoTransferGetHashHBAR,
    cryptoTransferSerializeHBAR,
    cryptoTransferIsEqualHBAR
};
