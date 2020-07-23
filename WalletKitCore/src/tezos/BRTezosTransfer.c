//
//  BRTezosTransfer.c
//  Core
//
//  Created by Ehsan Rezaie on 2020-06-17.
//  Copyright © 2020 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#include <stdlib.h>
#include <assert.h>

#include "BRTezosTransfer.h"
#include "BRTezosAccount.h"
#include "BRTezosTransaction.h"
#include "BRTezosFeeBasis.h"

//const char * fee = "__fee__";

struct BRTezosTransferRecord {
    BRTezosAddress sourceAddress;
    BRTezosAddress targetAddress;
    BRTezosUnitMutez amount;
    BRTezosUnitMutez fee;
    BRTezosTransactionHash transactionId;
    uint64_t timestamp;
    uint64_t blockHeight;
    int error;
    BRTezosTransaction transaction;
};

extern BRTezosTransfer /* caller must free - tezosTransferFree */
tezosTransferCreate(BRTezosAddress from,
                    BRTezosAddress to,
                    BRTezosUnitMutez amount,
                    BRTezosUnitMutez fee,
                    BRTezosTransactionHash hash,
                    uint64_t timestamp, uint64_t blockHeight, int error)
{
    BRTezosTransfer transfer = (BRTezosTransfer) calloc (1, sizeof (struct BRTezosTransferRecord));
    transfer->sourceAddress = tezosAddressClone (from);
    transfer->targetAddress = tezosAddressClone (to);
    transfer->amount = amount;
    transfer->fee = fee;
    transfer->transactionId = hash;
    transfer->timestamp = timestamp;
    transfer->blockHeight = blockHeight;
    transfer->error = error;
    transfer->transaction = NULL;
    return transfer;
}

extern BRTezosTransfer /* caller must free - tezosTransferFree */
tezosTransferCreateNew(BRTezosAddress from,
                       BRTezosAddress to,
                       BRTezosUnitMutez amount)
{
    BRTezosTransfer transfer = (BRTezosTransfer) calloc (1, sizeof (struct BRTezosTransferRecord));
    transfer->sourceAddress = tezosAddressClone (from);
    transfer->targetAddress = tezosAddressClone (to);
    transfer->amount = amount;
    BRTezosFeeBasis feeBasis = tezosDefaultFeeBasis();
    transfer->fee = feeBasis.fee;
    transfer->transaction = tezosTransactionCreateNew (from, to, amount, feeBasis);
    return transfer;
}

extern BRTezosTransfer
tezosTransferClone (BRTezosTransfer transfer) {
    BRTezosTransfer clone = (BRTezosTransfer) calloc (1, sizeof (struct BRTezosTransferRecord));
    memcpy (clone, transfer, sizeof (struct BRTezosTransferRecord));
    
    if (transfer->sourceAddress)
        clone->sourceAddress = tezosAddressClone (transfer->sourceAddress);
    
    if (transfer->targetAddress)
        clone->targetAddress = tezosAddressClone (transfer->targetAddress);
    
    if (transfer->transaction)
        clone->transaction = tezosTransactionClone (transfer->transaction);
    
    return clone;
}

extern void tezosTransferFree(BRTezosTransfer transfer)
{
    assert(transfer);
    if (transfer->sourceAddress) tezosAddressFree (transfer->sourceAddress);
    if (transfer->targetAddress) tezosAddressFree (transfer->targetAddress);
    if (transfer->transaction) {
        tezosTransactionFree(transfer->transaction);
    }
    free(transfer);
}

// Getters for all the values
extern BRTezosTransactionHash tezosTransferGetTransactionId(BRTezosTransfer transfer)
{
    assert(transfer);
    if (transfer->transaction) {
        // If we have an embedded transaction that means that we created a new tx
        // which by now should have been serialized
        return tezosTransactionGetHash(transfer->transaction);
    } else {
        return transfer->transactionId;
    }
}
extern BRTezosUnitMutez tezosTransferGetAmount(BRTezosTransfer transfer)
{
    assert(transfer);
    return transfer->amount;
}
extern BRTezosAddress tezosTransferGetSource(BRTezosTransfer transfer)
{
    assert(transfer);
    return tezosAddressClone (transfer->sourceAddress);
}
extern BRTezosAddress tezosTransferGetTarget(BRTezosTransfer transfer)
{
    assert(transfer);
    return tezosAddressClone (transfer->targetAddress);
}

extern BRTezosUnitMutez tezosTransferGetFee(BRTezosTransfer transfer)
{
    assert(transfer);
    return transfer->fee;
}

extern BRTezosFeeBasis tezosTransferGetFeeBasis (BRTezosTransfer transfer) {
    return (BRTezosFeeBasis) {
        transfer->fee,
        1
    };
}

extern BRTezosTransaction tezosTransferGetTransaction(BRTezosTransfer transfer)
{
    assert(transfer);
    return transfer->transaction;
}

extern int
tezosTransferHasError(BRTezosTransfer transfer) {
    return transfer->error;
}

extern int tezosTransferHasSource (BRTezosTransfer transfer,
                                   BRTezosAddress source) {
    return tezosAddressEqual (transfer->sourceAddress, source);
}

extern uint64_t tezosTransferGetBlockHeight (BRTezosTransfer transfer) {
    assert(transfer);
    return transfer->blockHeight;
}
