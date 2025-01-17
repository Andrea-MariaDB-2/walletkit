//
//  BBREthereumBlock.c
//  WalletKitCore Ethereum
//
//  Created by Ed Gamble on 3/23/2018.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "support/BRArray.h"
#include "ethereum/base/BREthereumLogic.h"
#include "BREthereumBlock.h"
#include "BREthereumLog.h"

// When looking for a block checkpoint based on a timestamp, back up by this much to ensure
// we don't miss the account's initial block.
#define BLOCK_CHECKPOINT_TIMESTAMP_SAFETY      (2 * 7 * 24 * 60 * 60)  // two weeks

//#define BLOCK_LOG_ALLOC_COUNT

#if defined (BLOCK_LOG_ALLOC_COUNT)
static unsigned int blockAllocCount = 0;
#endif

//#define BLOCK_HEADER_LOG_ALLOC_COUNT

#if defined (BLOCK_HEADER_LOG_ALLOC_COUNT)
static unsigned int blockHeaderAllocCount = 0;
#endif

// Mainnet Only
#define HOMESTEAD_FORK_BLOCK_NUMBER   (1150000)
#define DAO_FORK_BLOCK_NUMBER         (1920000)
#define EIP150_FORK_BLOCK_NUMBER      (2463000)
#define EIP155_FORK_BLOCK_NUMBER      (2675000)
#define EIP158_FORK_BLOCK_NUMBER      (2675000)
#define BYZANTIUM_FORK_BLOCK_NUMBER   (4370000)
#define ISTANBUL_FORK_BLOCK_NUMBER    (9069000)


/// MARK: - Block Status

static void
ethBlockStatusInitialize (BREthereumBlockStatus *status,
                       BREthereumHash hash) {
    memset (status, 0, sizeof (BREthereumBlockStatus));
    status->error = ETHEREUM_BOOLEAN_FALSE;
    status->hash = hash;
}

static BRRlpItem
ethBlockStatusRlpEncode (BREthereumBlockStatus status,
                      BRRlpCoder coder);

static BREthereumBlockStatus
ethBlockStatusRlpDecode (BRRlpItem item,
                      BRRlpCoder coder);

//
// Block Header
//
struct BREthereumBlockHeaderRecord {
    // THIS MUST BE FIRST to support BRSet operations.

    /**
     * The Keccak256-bit hash (of this Block).
     */
    BREthereumHash hash;

    /**
     * The Keccak256-bit hash of the parent block’s header, in its entirety; formally Hp.
     */
    BREthereumHash parentHash;

    /**
     * The Keccak 256-bit hash of the ommers list portion of this block; formally Ho.
     */
    BREthereumHash ommersHash;

    /**
     * The 160-bit address to which all fees collected from the successful mining of this block
     * be transferred; formally Hc.
     */
    BREthereumAddress beneficiary;

    /**
     * The Keccak 256-bit hash of the root node of the state trie, after all transactions are
     * executed and finalisations applied; formally Hr.
     */
    BREthereumHash stateRoot;

    /**
     * The Keccak 256-bit hash of the root node of the trie structure populated with each
     * transaction in the transactions list portion of the block; formally Ht.
     */
    BREthereumHash transactionsRoot;

    /**
     * The Keccak 256-bit hash of the root node of the trie structure populated with the receipts
     * of each transaction in the transactions list portion of the block; formally He.
     */
    BREthereumHash receiptsRoot;

    /**
     * The Bloom filter composed from indexable information (logger address and log topics)
     * contained in each log entry from the receipt of each transaction in the transactions list;
     * formally Hb.
     */
    BREthereumBloomFilter logsBloom;

    /**
     * A scalar value corresponding to the difficulty level of this block. This can be calculated
     * from the previous block’s difficulty level and the timestamp; formally Hd.
     * Also see: https://ethereum.stackexchange.com/questions/7068/difficulty-and-total-difficulty
     */
    UInt256 difficulty;

    /**
     * A scalar value equal to the number of ancestor blocks. The genesis block has a number of
     * zero; formally Hi.
     */
    uint64_t number;

    /**
     * A scalar value equal to the current limit of gas expenditure per block; formally Hl.
     */
    uint64_t gasLimit; // BREthereumGas

    /**
     * A scalar value equal to the total gas used in transactions in this block; formally Hg.
     */
    uint64_t gasUsed; // BREthereumGas

    /**
     * A scalar value equal to the reasonable output of Unix’s time() at this block’s inception;
     * formally Hs.
     */
    uint64_t timestamp;

    /**
     * An arbitrary byte array containing data relevant to this block. This must be 32 bytes or
     * fewer; formally Hx.
     */
    uint8_t extraData [32];
    uint8_t extraDataCount;

    /**
     * A 256-bit hash which, combined with the nonce, proves that a sufficient amount of
     * computation has been carried out on this block; formally Hm.
     */
    BREthereumHash mixHash;

    /**
     * A 64-bitvalue which, combined with the mixHash, proves that a sufficient amount of
     * computation has been carried out on this block; formally Hn.
     */
    uint64_t nonce;
};

static BREthereumBlockHeader
createBlockHeaderMinimal (BREthereumHash hash, uint64_t number, uint64_t timestamp, UInt256 difficulty) {
    BREthereumBlockHeader header = (BREthereumBlockHeader) calloc (1, sizeof (struct BREthereumBlockHeaderRecord));
    header->number = number;
    header->timestamp = timestamp;
    header->hash = hash;
    header->difficulty = difficulty;

#if defined (BLOCK_HEADER_LOG_ALLOC_COUNT)
    eth_log ("MEM", "Block Header Create Minimal: %d", ++blockHeaderAllocCount);
#endif

    return header;
}

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-function"
static BREthereumBlockHeader
createBlockHeader (void) {
    BREthereumBlockHeader header = (BREthereumBlockHeader) calloc (1, sizeof (struct BREthereumBlockHeaderRecord));

    //         EMPTY_DATA_HASH = sha3(EMPTY_BYTE_ARRAY);
    //         EMPTY_LIST_HASH = sha3(RLP.encodeList());

    // transactionRoot, receiptsRoot
    //         EMPTY_TRIE_HASH = sha3(RLP.encodeElement(EMPTY_BYTE_ARRAY));
#if defined (BLOCK_HEADER_LOG_ALLOC_COUNT)
    eth_log ("MEM", "Block Header Create Empty: %d", ++blockHeaderAllocCount);
#endif

    return header;
}
#pragma clang diagnostic pop
#pragma GCC diagnostic pop

extern BREthereumBlockHeader
ethBlockHeaderCopy (BREthereumBlockHeader source) {
    BREthereumBlockHeader header = (BREthereumBlockHeader) calloc (1, sizeof (struct BREthereumBlockHeaderRecord));
    *header = *source;
#if defined (BLOCK_HEADER_LOG_ALLOC_COUNT)
    eth_log ("MEM", "Block Header Copy  %d", ++blockHeaderAllocCount);
#endif
    return header;
}

extern void
ethBlockHeadersRelease (BRArrayOf(BREthereumBlockHeader) headers) {
    if (NULL != headers) {
        size_t count = array_count(headers);
        for (size_t index = 0; index < count; index++)
            ethBlockHeaderRelease(headers[index]);
        array_free (headers);
    }
}

extern void
ethBlockHeaderRelease (BREthereumBlockHeader header) {
    if (NULL != header) {
#if defined (BLOCK_HEADER_LOG_ALLOC_COUNT)
        eth_log ("MEM", "Block Header Release %d", --blockHeaderAllocCount);
#endif
        assert (ETHEREUM_BOOLEAN_IS_FALSE(ethHashEqual(header->hash, ethHashCreateEmpty())));
        memset (header, 0, sizeof(struct BREthereumBlockHeaderRecord));
        free (header);
    }
}

extern void
ethBlockHeaderReleaseForSet (void *ignore, void *item) {
    ethBlockHeaderRelease ((BREthereumBlockHeader) item);
}

extern BREthereumHash
ethBlockHeaderGetHash (BREthereumBlockHeader header) {
    return header->hash;
}

extern BREthereumHash
ethBlockHeaderGetParentHash (BREthereumBlockHeader header) {
    return header->parentHash;
}

extern uint64_t
ethBlockHeaderGetNumber (BREthereumBlockHeader header) {
    return header->number;
}

extern uint64_t
ethBlockHeaderGetTimestamp (BREthereumBlockHeader header) {
    return header->timestamp;
}

extern UInt256
ethBlockHeaderGetDifficulty (BREthereumBlockHeader header) {
    return header->difficulty;
}

extern uint64_t
ethBlockHeaderGetGasUsed (BREthereumBlockHeader header) {
    return header->gasUsed;
}

extern BREthereumHash
ethBlockHeaderGetMixHash (BREthereumBlockHeader header) {
    return header->mixHash;
}

extern uint64_t
ethBlockHeaderGetNonce (BREthereumBlockHeader header) {
    return header->nonce;
}

extern size_t
ethBlockHeaderHashValue (const void *h)
{
    return ethHashSetValue(&((BREthereumBlockHeader) h)->hash);
}

extern int
ethBlockHeaderHashEqual (const void *h1, const void *h2) {
    return h1 == h2 || ethHashSetEqual (&((BREthereumBlockHeader) h1)->hash,
                                     &((BREthereumBlockHeader) h2)->hash);
}

extern BREthereumComparison
ethBlockHeaderCompare (BREthereumBlockHeader h1,
                    BREthereumBlockHeader h2) {
    return (h1->number < h2->number
            ? ETHEREUM_COMPARISON_LT
            : (h1->number > h2->number
               ? ETHEREUM_COMPARISON_GT
               : (h1->timestamp < h2->timestamp
                  ? ETHEREUM_COMPARISON_LT
                  : (h1->timestamp > h2->timestamp
                     ? ETHEREUM_COMPARISON_GT
                     : ETHEREUM_COMPARISON_EQ))));
}

extern BREthereumBoolean
ethBlockHeaderMatch (BREthereumBlockHeader header,
                  BREthereumBloomFilter filter) {
    return ethBloomFilterMatch(header->logsBloom, filter);
}

extern BREthereumBoolean
ethBlockHeaderMatchAddress (BREthereumBlockHeader header,
                         BREthereumAddress address) {
    return AS_ETHEREUM_BOOLEAN
    (ETHEREUM_BOOLEAN_IS_TRUE (ethBlockHeaderMatch (header, ethBloomFilterCreateAddress (address))) ||
     ETHEREUM_BOOLEAN_IS_TRUE (ethBlockHeaderMatch (header, ethLogTopicGetBloomFilterAddress (address))));
}

extern uint64_t
chtRootNumberGetFromNumber (uint64_t number) {
    assert (0 != number);
    return (number - 1) >> BLOCK_HEADER_CHT_ROOT_INTERVAL_SHIFT;
}

extern uint64_t
ethBlockHeaderGetCHTRootNumber (BREthereumBlockHeader header) {
    return chtRootNumberGetFromNumber (ethBlockHeaderGetNumber(header));
}

extern BREthereumBoolean
ethBlockHeaderIsCHTRoot (BREthereumBlockHeader header) {
    return AS_ETHEREUM_BOOLEAN (0 == (header->number - 1) % BLOCK_HEADER_CHT_ROOT_INTERVAL);
}

#if defined (INCLUDE_UNUSED_FUNCTION)
static int64_t max(int64_t x, int64_t y) { return x >= y ? x : y; }
static uint64_t xbs(int64_t x) { return x < 0 ? -x : x; }

static int64_t
ethBlockHeaderCanonicalDifficulty_GetSigma2 (uint64_t number,
                                          uint64_t headerTimestamp,
                                          uint64_t parentTimestamp,
                                          uint64_t parentOmmersCount) {
    // https://github.com/ethereum/EIPs/blob/master/EIPS/eip-2.md
    if (number < HOMESTEAD_FORK_BLOCK_NUMBER)
        return (headerTimestamp - parentTimestamp < 13 ? 1 : -1);
    else if (number < BYZANTIUM_FORK_BLOCK_NUMBER)
        return max (1 - ((headerTimestamp - parentTimestamp) / 10), -99);
    else {
        // y = { 1 if parentOmmersCount is zero; otherwise 2 }
        uint64_t y = parentOmmersCount == 0 ? 1 : 2;
        return max (y - ((headerTimestamp - parentTimestamp) / 9), -99);
    }
}

static uint64_t
ethBlockHeaderCanonicalDifficulty_GetFakeBlockNumber (uint64_t number) {
    // https://github.com/ethereum/EIPs/blob/master/EIPS/eip-649.md
    // fake_block_number = max(0, block.number - 3_000_000) if block.number >= BYZANTIUM_FORK_BLKNUM else block.number
    return (number >= BYZANTIUM_FORK_BLOCK_NUMBER
            ? (number > 3000000 ? (number - 3000000) : 0)
            : number);
}

// See https://ethereum.github.io/yellowpaper/paper.pdf Section 4.3.3 'Block Header Validity
static UInt256
ethBlockHeaderCanonicalDifficulty (BREthereumBlockHeader header,
                                BREthereumBlockHeader parent,
                                size_t parentOmmersCount,
                                BREthereumBlockHeader genesis) {
    UInt256 Dzero = genesis->difficulty;

    if (0 == header->number) return Dzero;

    uint32_t rem; int overflow = 0;

    // z = P(H)_Hd / 2048
    UInt256 x = uint256Div_Small(parent->difficulty, 2048, &rem);

    int64_t sigma_2 = ethBlockHeaderCanonicalDifficulty_GetSigma2 (header->number,
                                                                header->timestamp,
                                                                parent->timestamp,
                                                                parentOmmersCount);
    assert (INT32_MIN <= sigma_2 && sigma_2 <= INT32_MAX);

    // H-prime_i = max (Hi - 3000000 , 0)
    uint64_t fake_block_number = ethBlockHeaderCanonicalDifficulty_GetFakeBlockNumber (header->number);

    // epsilon_exponent = (H-prime_i / 1000000) - 2
    int64_t epsilon_exponent = (fake_block_number / 100000) - 2;
    assert (-256 < epsilon_exponent && epsilon_exponent < 256);

    // epsilon = floor (2 ^ epsilon_exponent)
     UInt256 epsilon = (epsilon_exponent < 0
                        ? UINT256_ZERO
                        : uint256CreatePower2 (epsilon_exponent));

    // D(H) = P(H)d + x * sigma_2 + epsilon

    UInt256 x_sigma = uint256Mul_Small(x, (uint32_t) xbs (sigma_2), &overflow);
    assert (0 == overflow);

    UInt256 r = (sigma_2 >= 0
                 ? uint256Add_Overflow (parent->difficulty, x_sigma, &overflow)
                 : uint256Sub_Negative( parent->difficulty, x_sigma, &overflow));
    assert (0 == overflow);

    r = uint256Add_Overflow(r, epsilon, &overflow);
    assert (0 == overflow);

    return uint256GT (r, Dzero) ? r : Dzero;
}
#endif

static int
ethBlockHeaderValidateTimestamp (BREthereumBlockHeader this,
                              BREthereumBlockHeader parent) {
    return this->timestamp > parent->timestamp;
}

static int
ethBlockHeaderValidateNumber (BREthereumBlockHeader this,
                           BREthereumBlockHeader parent) {
    return this->number == 1 + parent->number;
}

static int
ethBlockHeaderValidateGasLimit (BREthereumBlockHeader this,
                             BREthereumBlockHeader parent) {
    return (this->gasLimit < parent->gasLimit + (parent->gasLimit / 1024) &&
            this->gasLimit > parent->gasLimit - (parent->gasLimit / 1024) &&
            this->gasLimit >= 5000);
}

static int
ethBlockHeaderValidateGasUsed (BREthereumBlockHeader this,
                            BREthereumBlockHeader parent) {
    return this->gasUsed <= this->gasLimit;
}

static int
ethBlockHeaderValidateExtraData (BREthereumBlockHeader this,
                              BREthereumBlockHeader parent) {
    return this->extraDataCount <= 32;
}

#if defined (INCLUDE_UNUSED_FUNCTION)
static int
ethBlockHeaderValidateDifficulty (BREthereumBlockHeader this,
                               BREthereumBlockHeader parent,
                               size_t parentOmmersCount,
                               BREthereumBlockHeader genesis) {
    return uint256EQL (this->difficulty,
                      ethBlockHeaderCanonicalDifficulty (this, parent, parentOmmersCount, genesis));
}
#endif

static int
ethBlockHeaderValidatePoWMixHash (BREthereumBlockHeader this,
                               BREthereumHash mixHash) {
    return (ETHEREUM_BOOLEAN_IS_TRUE (ethHashEqual (mixHash, ETHEREUM_EMPTY_HASH_INIT)) || // no POW
            ETHEREUM_BOOLEAN_IS_TRUE (ethHashEqual (mixHash, this->mixHash)));
}

static int
ethBlockHeaderValidatePoWNFactor (BREthereumBlockHeader this,
                               UInt256 powNFactor) {

    // Special case for no POW
    if (1 == UInt256Eq (powNFactor, UINT256_ZERO)) return 1;

    // validate as n_factor <= 2^256 / difficulty
    //
    // We'll compute as: `n_factor * difficulty <= 2^256` and notice that 2^256 is the smallest
    // 512-bit number.  Thus we'll just multiple nonce and difficulty and look for overflow.
    //
    // TODO: Does this account for '='?  What are the odds? (guaranteed to 'hit them')

    int overflow = 0;
    uint256Mul_Overflow (powNFactor, this->difficulty, &overflow);
    return 0 == overflow; /* || result == 2^256 */
}

static int
ethBlockHeaderValidateAll (BREthereumBlockHeader this,
                        BREthereumBlockHeader parent,
                        size_t parentOmmersCount,
                        BREthereumBlockHeader genesis,
                        BREthereumProofOfWork pow) {
    assert (NULL != parent);

    UInt256 n = UINT256_ZERO;
    BREthereumHash m = ETHEREUM_EMPTY_HASH_INIT;

    if (NULL != pow)
        proofOfWorkCompute (pow, this, &n, &m);

    return (ethBlockHeaderValidateTimestamp  (this, parent) &&
            ethBlockHeaderValidateNumber     (this, parent) &&
            ethBlockHeaderValidateGasLimit   (this, parent) &&
            ethBlockHeaderValidateGasUsed    (this, parent) &&
            ethBlockHeaderValidateExtraData  (this, parent) &&
            // TODO: Disabled, see CORE-203 (parentOmmersCount isn't correct if non-zero).
            // ethBlockHeaderValidateDifficulty (this, parent, parentOmmersCount, genesis) &&
            (NULL == pow || ethBlockHeaderValidatePoWMixHash (this, m)) &&
            (NULL == pow || ethBlockHeaderValidatePoWNFactor (this, n)));
}

extern BREthereumBoolean
ethBlockHeaderIsInternallyValid (BREthereumBlockHeader header) {
    return ETHEREUM_BOOLEAN_TRUE;
}

extern BREthereumBoolean
ethBlockHeaderIsValid (BREthereumBlockHeader header,
                    BREthereumBlockHeader parent,
                    size_t parentOmmersCount,
                    BREthereumBlockHeader genesis,
                    BREthereumProofOfWork pow) {

//    // Hah! See CORE-203
//    while (0 == ethBlockHeaderValidateAll (header, parent, parentOmmersCount, genesis, pow))
//        parentOmmersCount += 1;

    // See https://ethereum.github.io/yellowpaper/paper.pdf Section 4.3.3 'Block Header Validity
    return AS_ETHEREUM_BOOLEAN (NULL == parent ||
                                ethBlockHeaderValidateAll (header, parent, parentOmmersCount, genesis, pow));
}

//
// Block Header RLP Encode / Decode
//
//
extern BRRlpItem
ethBlockHeaderRlpEncode (BREthereumBlockHeader header,
                      BREthereumBoolean withNonce,
                      BREthereumRlpType type,
                      BRRlpCoder coder) {
    BRRlpItem items[15];
    size_t itemsCount = ETHEREUM_BOOLEAN_IS_TRUE(withNonce) ? 15 : 13;

    items[ 0] = ethHashRlpEncode(header->parentHash, coder);
    items[ 1] = ethHashRlpEncode(header->ommersHash, coder);
    items[ 2] = ethAddressRlpEncode(header->beneficiary, coder);
    items[ 3] = ethHashRlpEncode(header->stateRoot, coder);
    items[ 4] = ethHashRlpEncode(header->transactionsRoot, coder);
    items[ 5] = ethHashRlpEncode(header->receiptsRoot, coder);
    items[ 6] = ethBloomFilterRlpEncode(header->logsBloom, coder);
    items[ 7] = rlpEncodeUInt256 (coder, header->difficulty, 0);
    items[ 8] = rlpEncodeUInt64(coder, header->number, 0);
    items[ 9] = rlpEncodeUInt64(coder, header->gasLimit, 0);
    items[10] = rlpEncodeUInt64(coder, header->gasUsed, 0);
    items[11] = rlpEncodeUInt64(coder, header->timestamp, 0);
    items[12] = rlpEncodeBytes(coder, header->extraData, header->extraDataCount);

    if (ETHEREUM_BOOLEAN_IS_TRUE(withNonce)) {
        items[13] = ethHashRlpEncode(header->mixHash, coder);
        items[14] = rlpEncodeUInt64(coder, header->nonce, 0);
    }

    return rlpEncodeListItems(coder, items, itemsCount);
}

extern BREthereumBlockHeader
ethBlockHeaderRlpDecode (BRRlpItem item,
                      BREthereumRlpType type,
                      BRRlpCoder coder) {
    BREthereumBlockHeader header = (BREthereumBlockHeader) calloc (1, sizeof(struct BREthereumBlockHeaderRecord));

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);
    assert (13 == itemsCount || 15 == itemsCount);

    header->hash = ethHashCreateEmpty();

    header->parentHash = ethHashRlpDecode(items[0], coder);
    header->ommersHash = ethHashRlpDecode(items[1], coder);
    header->beneficiary = ethAddressRlpDecode(items[2], coder);
    header->stateRoot = ethHashRlpDecode(items[3], coder);
    header->transactionsRoot = ethHashRlpDecode(items[4], coder);
    header->receiptsRoot = ethHashRlpDecode(items[5], coder);
    header->logsBloom = ethBloomFilterRlpDecode(items[6], coder);
    header->difficulty = rlpDecodeUInt256(coder, items[7], 0);
    header->number = rlpDecodeUInt64(coder, items[8], 0);
    header->gasLimit = rlpDecodeUInt64(coder, items[9], 0);
    header->gasUsed = rlpDecodeUInt64(coder, items[10], 0);
    header->timestamp = rlpDecodeUInt64(coder, items[11], 0);

    BRRlpData extraData = rlpDecodeBytes(coder, items[12]);
    memset (header->extraData, 0, 32);
    memcpy (header->extraData, extraData.bytes, extraData.bytesCount);
    header->extraDataCount = extraData.bytesCount;
    rlpDataRelease(extraData);

    if (15 == itemsCount) {
        header->mixHash = ethHashRlpDecode(items[13], coder);
        header->nonce = rlpDecodeUInt64(coder, items[14], 0);
    }

#if defined (BLOCK_HEADER_LOG_ALLOC_COUNT)
    eth_log ("MEM", "Block Header Create RLP: %d", ++blockHeaderAllocCount);
#endif

    BRRlpData data = rlpItemGetDataSharedDontRelease(coder, item);
    header->hash = ethHashCreateFromData(data);
    // Safe to ignore data release.

    return header;

}

/// MARK: - Block

//
// Block
//
struct BREthereumBlockRecord {
    // THIS MUST BE FIRST to support BRSet operations. (its first field is BREthereumHash)
    BREthereumBlockStatus status;

    /**
     * ... the collection of relevant pieces of information (known as the block header), H,
     */
    BREthereumBlockHeader header;

    /**
     * ... together with information corresponding to the comprised transactions, T,
     */
    BREthereumTransaction *transactions;

    /**
     * ... and a set of other block headers U that are known to have a parent equal to the present
     * block’s parent’s parent (such blocks are known as ommers).
     * Also, see: https://ethereum.stackexchange.com/questions/34/what-is-an-uncle-ommer-block
     */
    BREthereumBlockHeader *ommers;

    /**
     * ... the totalDifficulty as a) zero or b) the header difficulty + next's totalDifficulty
     */
    UInt256 totalDifficulty;

    /**
     * The next block in the chain via the 'parent'
     */
    BREthereumBlock next;
};

extern BREthereumBlock
ethBlockCreateMinimal(BREthereumHash hash,
                   uint64_t number,
                   uint64_t timestamp,
                   UInt256 difficulty) {
    return ethBlockCreate(createBlockHeaderMinimal(hash, number, timestamp, difficulty));
}

extern BREthereumBlock
ethBlockCreateFull (BREthereumBlockHeader header,
                 BREthereumBlockHeader ommers[], size_t ommersCount,
                 BREthereumTransaction transactions[], size_t transactionCount) {
    BREthereumBlock block = ethBlockCreate(header);

    BREthereumBlockHeader *blockOmmers;
    array_new (blockOmmers, ommersCount);
    for (int i = 0; i < ommersCount; i++)
        array_add (blockOmmers, ommers[i]);

    BREthereumTransaction *blockTransactions;
    array_new (blockTransactions, transactionCount);
    for (int i = 0; i < transactionCount; i++)
        array_add (blockTransactions,  transactions[i]);

    ethBlockUpdateBody(block, blockOmmers, blockTransactions);

    return block;
}

extern BREthereumBlock
ethBlockCreate (BREthereumBlockHeader header) {
    BREthereumBlock block = (BREthereumBlock) calloc (1, sizeof (struct BREthereumBlockRecord));

    block->header = header;
    block->ommers = NULL;
    block->transactions = NULL;
    block->totalDifficulty = UINT256_ZERO;

    ethBlockStatusInitialize(&block->status, ethBlockHeaderGetHash(block->header));
    block->next = BLOCK_NEXT_NONE;

#if defined (BLOCK_LOG_ALLOC_COUNT)
    eth_log ("MEM", "Block Create: %d", ++blockAllocCount);
#endif
    return block;
}

extern void
ethBlockUpdateBody (BREthereumBlock block,
                 BRArrayOf(BREthereumBlockHeader) ommers,
                 BRArrayOf(BREthereumTransaction) transactions) {
    block->ommers = ommers;
    block->transactions = transactions;
}

extern void
ethBlockRelease (BREthereumBlock block) {
    ethBlockHeaderRelease(block->header);

    ethBlockHeadersRelease (block->ommers);
    block->ommers = NULL;

    transactionsRelease (block->transactions);
    block->transactions = NULL;

    ethBlockReleaseStatus (block, ETHEREUM_BOOLEAN_TRUE, ETHEREUM_BOOLEAN_TRUE);
    block->next = BLOCK_NEXT_NONE;

#if defined (BLOCK_LOG_ALLOC_COUNT)
    eth_log ("MEM", "Block Release: %d", --blockAllocCount);
#endif

    free (block);
}

extern BREthereumBlockHeader
ethBlockGetHeader (BREthereumBlock block) {
    return block->header;
}

extern unsigned long
ethBlockGetTransactionsCount (BREthereumBlock block) {
    return array_count(block->transactions);
}

extern BREthereumTransaction
ethBlockGetTransaction (BREthereumBlock block, size_t index) {
    return (index < array_count(block->transactions)
            ? block->transactions[index]
            : NULL);
}

#if defined (INCLUDE_UNUSED_FUNCTION)
// This has minimal bearing on Ethereum
static BREthereumHash
ethBlockGetTransactionMerkleRootRecurse (BREthereumHash *hashes,
                                    size_t count) {
    // If no hash, return the hash of <something> (we know the hash).
    if (0 == count) return hashCreate ("0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");

    // If one hash, we are done.
    if (1 == count) return hashes[0];

    // If count is odd, increment it and duplicate the last hash
    if (1 == count % 2) {
        hashes[count] = hashes[count-1];
        count++;
    };
    // count is now surely even
    assert (0 == count % 2);

    // We'll pair two consecutive hashes together and then has the pair.
    struct ConcatenatedHashPair {
        BREthereumHash hashLeft;
        BREthereumHash hashRight;
    } concatenatedHashPair;

    BRRlpData concatenatedHashes = { sizeof (concatenatedHashPair), (uint8_t*) &concatenatedHashPair };

    for (size_t index = 0; index < count; index += 2) {
        concatenatedHashPair.hashLeft  = hashes [index];
        concatenatedHashPair.hashRight = hashes [index + 1];

        // Hash over the pair AND reassign into hashes (at a 'safe' index) - double hash?
        hashes[index] = hashCreateFromData(concatenatedHashes);
    }

    return ethBlockGetTransactionMerkleRootRecurse(hashes, count / 2);
}

// This has no/minimal bearing on Ethereum
static BREthereumHash
ethBlockGetTransactionMerkleRoot (BREthereumBlock block) {
    size_t count = array_count (block->transactions);
    // Note: count can be 0.

    // Ensure hashes has an even length at least as large a transactions.
    BREthereumHash hashes[0 == count % 2 ? count : (count + 1)];

    // Copy in the transaction hashes.
    for (size_t index = 0; index < count; index++)
        hashes[index] = ethTransactionGetHash (block->transactions[index]);

    return ethBlockGetTransactionMerkleRootRecurse (hashes, count);
}
#endif

static BREthereumHash
ethBlockGetTransactionTrieRoot (BREthereumBlock block) {
    // TODO: Implement...
    return block->header->transactionsRoot;  // assume correct.
}

extern BREthereumBoolean
ethBlockTransactionsAreValid (BREthereumBlock block) {
    return AS_ETHEREUM_BOOLEAN (ETHEREUM_COMPARISON_EQ == ethHashCompare(block->header->transactionsRoot, ethBlockGetTransactionTrieRoot(block)));
}

extern unsigned long
ethBlockGetOmmersCount (BREthereumBlock block) {
    return NULL == block->ommers ? 0 : array_count(block->ommers);
}

extern BREthereumBlockHeader
ethBlockGetOmmer (BREthereumBlock block, unsigned int index) {
    return (index < array_count(block->ommers)
            ? block->ommers[index]
            : NULL);
}

extern BREthereumHash
ethBlockGetHash (BREthereumBlock block) {
    return  block->header->hash;
}

extern uint64_t
ethBlockGetNumber (BREthereumBlock block) {
    return block->header->number;
}

extern uint64_t
ethBlockGetTimestamp (BREthereumBlock block) {
    return block->header->timestamp;
}
extern UInt256
ethBlockGetDifficulty (BREthereumBlock block) {
    return block->header->difficulty;
}

extern UInt256
ethBlockGetTotalDifficulty (BREthereumBlock block) {
    return block->totalDifficulty;
}

extern  void
ethBlockSetTotalDifficulty (BREthereumBlock block,
                         UInt256 totalDifficulty) {
    block->totalDifficulty = totalDifficulty;
}

extern void
ethBlockClrTotalDifficulty (BREthereumBlock block) {
    block->totalDifficulty = UINT256_ZERO;
}

extern BREthereumBoolean
ethBlockHasTotalDifficulty (BREthereumBlock block) {
    return AS_ETHEREUM_BOOLEAN (!UInt256Eq (block->totalDifficulty, UINT256_ZERO));
}

extern UInt256
ethBlockRecursivelyPropagateTotalDifficulty (BREthereumBlock block) {
    // If we don't have a total difficulty then compute it
    if (UInt256Eq (block->totalDifficulty, UINT256_ZERO)) {
        int overflow;

        // Note: on overflow the return value is UINT256_ZERO
        block->totalDifficulty = (NULL != block->next
                                  ? uint256Add_Overflow (ethBlockGetDifficulty (block),
                                                         ethBlockRecursivelyPropagateTotalDifficulty (block->next),
                                                         &overflow)
                                  : UINT256_ZERO);
    }
    return block->totalDifficulty;
}

extern void
ethBlockLinkLogsWithTransactions (BREthereumBlock block) {
    assert (ETHEREUM_BOOLEAN_IS_TRUE (ethBlockHasStatusLogsRequest(block, BLOCK_REQUEST_COMPLETE)) &&
            ETHEREUM_BOOLEAN_IS_TRUE (ethBlockHasStatusTransactionsRequest(block, BLOCK_REQUEST_COMPLETE)));

    // Having both transaction and logs (from receipts):

    // First, we can only now set gasUsed for each transaction;
    size_t transactionsCount = (NULL == block->status.transactions ? 0 : array_count(block->status.transactions));
    for (size_t index = 0; index < transactionsCount; index++) {
        BREthereumTransaction transaction = block->status.transactions[index];
        BREthereumTransactionStatus status =ethTransactionGetStatus(transaction);

        uint64_t transactionIndex, blockNumber;
        if (ethTransactionStatusExtractIncluded(&status, NULL, &blockNumber, &transactionIndex, NULL, NULL)) {
            status.u.included.gasUsed = block->status.gasUsed[transactionIndex];
            status.u.included.blockTimestamp = ethBlockGetTimestamp(block);
            ethTransactionSetStatus(transaction, status);
        }
    }

    // Second, we can initialize the log identifier, as { transactionHash, logIndex }
    size_t logsCount = (NULL == block->status.logs ? 0 : array_count(block->status.logs));
    for (size_t index = 0; index < logsCount; index++) {
        BREthereumLog log = block->status.logs[index];
        BREthereumTransactionStatus status = ethLogGetStatus(log);
        uint64_t transactionIndex; size_t logIndex;

        int transactionIncluded = ethTransactionStatusExtractIncluded (&status, NULL, NULL, &transactionIndex, NULL, NULL);
        assert (transactionIncluded);

        // Importantly, note that the log has no reference to the transaction itself.  And, if only
        // implicitly, we assume that `block` has the correct transaction at transactionIndex.
        BREthereumTransaction transaction = block->transactions[transactionIndex];

        // The logIndex was assigned (w/o the transaction hash) from the receipts
        ethLogExtractIdentifier(log, NULL, &logIndex);

        // Finally, a fully identified log
        ethLogInitializeIdentifier(log, ethTransactionGetHash(transaction), logIndex);
    }
}

//
// Block RLP Encode / Decode
//
static BRRlpItem
ethBlockTransactionsRlpEncode (BREthereumBlock block,
                            BREthereumNetwork network,
                            BREthereumRlpType type,
                            BRRlpCoder coder) {
    size_t itemsCount = (NULL == block->transactions ? 0 : array_count(block->transactions));

    // If there are no items, skip out immediately.
    if (0 == itemsCount) return rlpEncodeList(coder, 0);

    BRRlpItem items[itemsCount];

    for (int i = 0; i < itemsCount; i++)
        items[i] = ethTransactionRlpEncode(block->transactions[i],
                                        network,
                                        type,
                                        coder);

    return rlpEncodeListItems(coder, items, itemsCount);
}

extern BRArrayOf(BREthereumTransaction)
ethBlockTransactionsRlpDecode (BRRlpItem item,
                            BREthereumNetwork network,
                            BREthereumRlpType type,
                            BRRlpCoder coder) {
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);

    BRArrayOf(BREthereumTransaction) transactions;
    array_new(transactions, itemsCount);

    for (int i = 0; i < itemsCount; i++) {
        BREthereumTransaction transaction = ethTransactionRlpDecode(items[i],
                                                                     network,
                                                                     type,
                                                                     coder);
        array_add (transactions, transaction);
    }

    return transactions;
}

static BRRlpItem
ethBlockOmmersRlpEncode (BREthereumBlock block,
                      BREthereumRlpType type,
                      BRRlpCoder coder) {
    size_t itemsCount = (NULL == block->ommers ? 0 : array_count(block->ommers));

    // If there are no items, skip out immediately.
    if (0 == itemsCount) return rlpEncodeList(coder, 0);

    BRRlpItem items[itemsCount];

    for (int i = 0; i < itemsCount; i++)
        items[i] = ethBlockHeaderRlpEncode (block->ommers[i],
                                         ETHEREUM_BOOLEAN_TRUE,
                                         type,
                                         coder);

    return rlpEncodeListItems(coder, items, itemsCount);
}

extern BRArrayOf (BREthereumBlockHeader)
ethBlockOmmersRlpDecode (BRRlpItem item,
                      BREthereumNetwork network,
                      BREthereumRlpType type,
                      BRRlpCoder coder) {
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);

    BRArrayOf (BREthereumBlockHeader) headers;
    array_new(headers, itemsCount);

    for (int i = 0; i < itemsCount; i++) {
        BREthereumBlockHeader header = ethBlockHeaderRlpDecode(items[i], type, coder);
        array_add (headers, header);
    }

    return headers;
}

//
// Block Encode
//
extern BRRlpItem
ethBlockRlpEncode (BREthereumBlock block,
                BREthereumNetwork network,
                BREthereumRlpType type,
                BRRlpCoder coder) {
    BRRlpItem items[5];

    items[0] = ethBlockHeaderRlpEncode(block->header, ETHEREUM_BOOLEAN_TRUE, type, coder);
    items[1] = ethBlockTransactionsRlpEncode(block, network, RLP_TYPE_TRANSACTION_SIGNED, coder);
    items[2] = ethBlockOmmersRlpEncode(block, type, coder);

    if (RLP_TYPE_ARCHIVE == type) {
        items[3] = rlpEncodeUInt256 (coder, block->totalDifficulty, 0);
        items[4] = ethBlockStatusRlpEncode(block->status, coder);
    }

    return rlpEncodeListItems(coder, items, (RLP_TYPE_ARCHIVE == type ? 5 : 3));
}

//
// Block Decode
//
extern BREthereumBlock
ethBlockRlpDecode (BRRlpItem item,
                BREthereumNetwork network,
                BREthereumRlpType type,
                BRRlpCoder coder) {
    BREthereumBlock block = calloc (1, sizeof(struct BREthereumBlockRecord));

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);
    assert ((3 == itemsCount && RLP_TYPE_NETWORK == type) ||
            (5 == itemsCount && RLP_TYPE_ARCHIVE == type));

    block->header = ethBlockHeaderRlpDecode(items[0], type, coder);
    block->transactions = ethBlockTransactionsRlpDecode(items[1], network, RLP_TYPE_TRANSACTION_SIGNED, coder);
    block->ommers = ethBlockOmmersRlpDecode(items[2], network, type, coder);

    if (RLP_TYPE_ARCHIVE == type) {
        block->totalDifficulty = rlpDecodeUInt256 (coder, items[3], 0);
        block->status = ethBlockStatusRlpDecode (items[4], coder);
    }
    else {
        ethBlockClrTotalDifficulty (block);
        ethBlockStatusInitialize (&block->status, ethBlockHeaderGetHash(block->header));
    }

    block->next = BLOCK_NEXT_NONE;

#if defined (BLOCK_LOG_ALLOC_COUNT)
    eth_log ("MEM", "Block Create RLP: %d", ++blockAllocCount);
#endif

    return block;
}

/// MARK: - Block As Set

extern size_t
ethBlockHashValue (const void *b)
{
    return ethHashSetValue(& ((BREthereumBlock) b)->status.hash);
}

extern int
ethBlockHashEqual (const void *b1, const void *b2) {
    return b1 == b2 || ethHashSetEqual (& ((BREthereumBlock) b1)->status.hash,
                                     & ((BREthereumBlock) b2)->status.hash);
}

extern void
ethBlockReleaseForSet (void *ignore, void *item) {
    ethBlockRelease((BREthereumBlock) item);
}

extern void
blocksRelease (OwnershipGiven BRArrayOf(BREthereumBlock) blocks) {
    if (NULL != blocks) {
        size_t count = array_count(blocks);
        for (size_t index = 0; index < count; index++)
            ethBlockRelease(blocks[index]);
        array_free (blocks);
    }
}

/// MARK: - Block Next (Chaining)

extern BREthereumBlock
ethBlockGetNext (BREthereumBlock block) {
    return block->next;
}

extern BREthereumBlock // old 'next'
ethBlockSetNext (BREthereumBlock block,
              BREthereumBlock newNext) {
    BREthereumBlock oldNext = block->next;
    block->next = newNext;
    return oldNext;
}

extern BREthereumBoolean
ethBlockHasNext (BREthereumBlock block) {
    return AS_ETHEREUM_BOOLEAN (BLOCK_NEXT_NONE != block->next);
}

/// MARK: - Block Body Pair

extern void
ethBlockBodyPairRelease (BREthereumBlockBodyPair *pair) {
    ethBlockHeadersRelease (pair->uncles);
    transactionsRelease (pair->transactions);
}

extern void
ethBlockBodyPairsRelease (BRArrayOf(BREthereumBlockBodyPair) pairs) {
    if (NULL != pairs) {
        size_t count = array_count(pairs);
        for (size_t index = 0; index < count; index++)
            ethBlockBodyPairRelease(&pairs[index]);
        array_free (pairs);
    }
}

/// MARK: - Block Status

extern BREthereumBlockStatus
ethBlockGetStatus (BREthereumBlock block) {
    return block->status;
}

extern BREthereumBoolean
ethBlockHasStatusComplete (BREthereumBlock block) {
    return AS_ETHEREUM_BOOLEAN (block->status.transactionRequest  != BLOCK_REQUEST_PENDING &&
                                block->status.logRequest          != BLOCK_REQUEST_PENDING &&
                                block->status.accountStateRequest != BLOCK_REQUEST_PENDING &&
                                block->status.headerProofRequest  != BLOCK_REQUEST_PENDING);
}

extern BREthereumBoolean
ethBlockHasStatusError (BREthereumBlock block) {
    return block->status.error;
}

extern void
ethBlockReportStatusError (BREthereumBlock block,
                        BREthereumBoolean error) {
    block->status.error = error;
}

//
// Transaction Request
//
extern BREthereumBoolean
ethBlockHasStatusTransactionsRequest (BREthereumBlock block,
                                   BREthereumBlockRequestState request) {
    return AS_ETHEREUM_BOOLEAN(block->status.transactionRequest == request);
}

extern void
ethBlockReportStatusTransactionsRequest (BREthereumBlock block,
                                      BREthereumBlockRequestState request) {
    block->status.transactionRequest = request;
}

extern void
ethBlockReportStatusTransactions (BREthereumBlock block,
                               OwnershipGiven BRArrayOf(BREthereumTransaction) transactions) {
    assert (block->status.transactionRequest == BLOCK_REQUEST_PENDING);
    block->status.transactionRequest = BLOCK_REQUEST_COMPLETE;
    block->status.transactions = transactions;
}

extern BREthereumBoolean
ethBlockHasStatusTransaction (BREthereumBlock block,
                           BREthereumTransaction transaction) {
    if (block->status.transactionRequest != BLOCK_REQUEST_COMPLETE) return ETHEREUM_BOOLEAN_FALSE;

    for (size_t index = 0; index < array_count(block->status.transactions); index++)
        if (ethTransactionHashEqual(transaction, block->status.transactions[index]))
            return ETHEREUM_BOOLEAN_TRUE;

    return ETHEREUM_BOOLEAN_FALSE;
}

//
// Gas Used
//
extern void
ethBlockReportStatusGasUsed (BREthereumBlock block,
                          OwnershipGiven BRArrayOf(BREthereumGas) gasUsed) {
    block->status.gasUsed = gasUsed;
}

//
// Log Request
//
extern BREthereumBoolean
ethBlockHasStatusLogsRequest (BREthereumBlock block,
                           BREthereumBlockRequestState request) {
    return AS_ETHEREUM_BOOLEAN(block->status.logRequest == request);
}

extern void
ethBlockReportStatusLogsRequest (BREthereumBlock block,
                              BREthereumBlockRequestState request) {
    block->status.logRequest = request;
}

extern void
ethBlockReportStatusLogs (BREthereumBlock block,
                       OwnershipGiven BRArrayOf(BREthereumLog) logs) {
    assert (block->status.logRequest == BLOCK_REQUEST_PENDING);
    block->status.logRequest = BLOCK_REQUEST_COMPLETE;
    block->status.logs = logs;
}

extern BREthereumBoolean
ethBlockHasStatusLog (BREthereumBlock block,
                   BREthereumLog log) {
    if (block->status.logRequest != BLOCK_REQUEST_COMPLETE) return ETHEREUM_BOOLEAN_FALSE;

    for (size_t index = 0; index < array_count(block->status.logs); index++)
        if (ethLogHashEqual(log, block->status.logs[index]))
            return ETHEREUM_BOOLEAN_TRUE;

    return ETHEREUM_BOOLEAN_FALSE;
}

//
// Account State Request
//
extern BREthereumBoolean
ethBlockHasStatusAccountStateRequest (BREthereumBlock block,
                                   BREthereumBlockRequestState request) {
    return AS_ETHEREUM_BOOLEAN(block->status.accountStateRequest == request);
}

extern void
ethBlockReportStatusAccountStateRequest (BREthereumBlock block,
                                      BREthereumBlockRequestState request) {
    block->status.accountStateRequest = request;
}

extern void
ethBlockReportStatusAccountState (BREthereumBlock block,
                               BREthereumAccountState accountState) {
    assert (block->status.accountStateRequest == BLOCK_REQUEST_PENDING);
    block->status.accountStateRequest = BLOCK_REQUEST_COMPLETE;
    block->status.accountState = accountState;
}

//
// Header Proof
//
extern BREthereumBoolean
ethBlockHasStatusHeaderProofRequest (BREthereumBlock block,
                                  BREthereumBlockRequestState request) {
    return AS_ETHEREUM_BOOLEAN (block->status.headerProofRequest == request);
}

extern void
ethBlockReportStatusHeaderProofRequest (BREthereumBlock block,
                                     BREthereumBlockRequestState request) {
    block->status.headerProofRequest = request;
}

extern void
ethBlockReportStatusHeaderProof (BREthereumBlock block,
                              BREthereumBlockHeaderProof proof) {
    assert (block->status.headerProofRequest == BLOCK_REQUEST_PENDING);
    block->status.headerProofRequest = BLOCK_REQUEST_COMPLETE;
    block->status.headerProof = proof;
}


extern void
ethBlockReleaseStatus (BREthereumBlock block,
                    BREthereumBoolean releaseTransactions,
                    BREthereumBoolean releaseLogs) {
    // We might be releasing before the status is complete.  That happens if BCS/LES shuts down
    // and started releasing blocks - and that is okay.  But, if status is released while the
    // block is being completed - then that is problem.

    // Transactions
    if (ETHEREUM_BOOLEAN_IS_TRUE(releaseTransactions))
        transactionsRelease(block->status.transactions);
    else if (NULL != block->status.transactions)
        array_free (block->status.transactions);
    block->status.transactions = NULL;

    // Logs
    if (ETHEREUM_BOOLEAN_IS_TRUE(releaseLogs))
        logsRelease(block->status.logs);
    else if (NULL != block->status.logs)
        array_free (block->status.logs);
    block->status.logs = NULL;

    // Gas Used
    if (NULL != block->status.gasUsed)
        array_free (block->status.gasUsed);
    block->status.gasUsed = NULL;

    // Account State - nothing to do
    // Header Proof  - nothing to do
}

static BRRlpItem
ethBlockStatusRlpEncode (BREthereumBlockStatus status,
                      BRRlpCoder coder) {
    BRRlpItem items[8];

    uint64_t flags = ((status.transactionRequest << 6) |
                      (status.logRequest << 4) |
                      (status.accountStateRequest << 2) |
                      (status.headerProofRequest << 0));

    items[0] = ethHashRlpEncode(status.hash, coder);
    items[1] = rlpEncodeUInt64(coder, flags, 1);

    // TODO: Fill out
    items[2] = rlpEncodeString(coder, "");   // transactions
    items[3] = rlpEncodeString(coder, "");   // logs
    items[4] = rlpEncodeString(coder, "");   // gasUsed
    items[5] = ethAccountStateRlpEncode(status.accountState, coder);
    items[6] = rlpEncodeString(coder, "");   // headerProof

    items[7] = rlpEncodeUInt64(coder, status.error, 0);

    return rlpEncodeListItems(coder, items, 8);
}

static BREthereumBlockStatus
ethBlockStatusRlpDecode (BRRlpItem item,
                      BRRlpCoder coder) {
    BREthereumBlockStatus status;

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);
    assert (8 == itemsCount);

    status.hash = ethHashRlpDecode(items[0], coder);

    uint64_t flags = rlpDecodeUInt64(coder, items[1], 1);
    status.transactionRequest  = 0x3 & (flags >> 6);
    status.logRequest          = 0x3 & (flags >> 4);
    status.accountStateRequest = 0x3 & (flags >> 2);
    status.headerProofRequest  = 0x3 & (flags >> 0);

    // TODO: Fill Out
    status.transactions = NULL;  // items [2]
    status.logs = NULL; // items [3]
    status.gasUsed = NULL; // items [4]
    status.accountState = ethAccountStateRlpDecode(items[5], coder);
    status.headerProof = (BREthereumBlockHeaderProof) { ETHEREUM_EMPTY_HASH_INIT, UINT256_ZERO }; // items[6]

    status.error = (BREthereumBoolean) rlpDecodeUInt64(coder, items[7], 0);
    return status;
}

/// MARK: - Genesis Blocks

// We should extract these blocks from the Ethereum Blockchain so as to have the definitive
// data.  

/*
 Appendix I. Genesis Block and is specified thus:
 The genesis block is 15 items,
 (0-256 , KEC(RLP()), 0-160 , stateRoot, 0, 0, 0-2048 , 2^17 , 0, 0, 3141592, time, 0, 0-256 , KEC(42), (), ()􏰂
 Where 0256 refers to the parent hash, a 256-bit hash which is all zeroes; 0160 refers to the
 beneficiary address, a 160-bit hash which is all zeroes; 02048 refers to the log bloom,
 2048-bit of all zeros; 217 refers to the difficulty; the transaction trie root, receipt
 trie root, gas used, block number and extradata are both 0, being equivalent to the empty
 byte array. The sequences of both ommers and transactions are empty and represented by
 (). KEC(42) refers to the Keccak hash of a byte array of length one whose first and only byte
 is of value 42, used for the nonce. KEC(RLP()) value refers to the hash of the ommer list in
 RLP, both empty lists.

 The proof-of-concept series include a development premine, making the state root hash some
 value stateRoot. Also time will be set to the initial timestamp of the genesis block. The
 latest documentation should be consulted for those values.
 */

// Ideally, we'd statically initialize the Genesis blocks.  But, we don't have static
// initializer for BREthereumHash, BREthereumAddress nor BREthereumBlookFilter.  Therefore,
// will define `initializeGenesisBlocks(void)` to convert hex-strings into the binary values.
//
// The following is expanded for illustration only; it is filled below.
static struct BREthereumBlockHeaderRecord genesisMainnetBlockHeaderRecord = {

    // BREthereumHash hash;
    ETHEREUM_EMPTY_HASH_INIT,

    // BREthereumHash parentHash;
    ETHEREUM_EMPTY_HASH_INIT,

    // BREthereumHash ommersHash;
    ETHEREUM_EMPTY_HASH_INIT,

    // BREthereumAddressRaw beneficiary;
    ETHEREUM_EMPTY_ADDRESS_INIT,

    // BREthereumHash stateRoot;
    ETHEREUM_EMPTY_HASH_INIT,

    // BREthereumHash transactionsRoot;
    ETHEREUM_EMPTY_HASH_INIT,

    // BREthereumHash receiptsRoot;
    ETHEREUM_EMPTY_HASH_INIT,

    // BREthereumBloomFilter logsBloom;
    ETHEREUM_EMPTY_BLOOM_FILTER_INIT,

    // uint256_t difficulty;
    UINT256_INIT (1 << 17),

    // uint64_t number;
    0,

    // uint64_t gasLimit;
    0x1388,

    // uint64_t gasUsed;
    0,

    // uint64_t timestamp;
    0,

    // uint8_t extraData [32];
    { 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
      0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0 },

    // uint8_t extraDataCount;
    0,

#if BLOCK_HEADER_NEEDS_SEED_HASH == 1
//    BREthereumHash seedHash;
    ETHEREUM_EMPTY_HASH_INIT,
#endif

    // BREthereumHash mixHash;
    ETHEREUM_EMPTY_HASH_INIT,

    // uint64_t nonce;
    0x0000000000000042
};
static struct BREthereumBlockHeaderRecord genesisTestnetBlockHeaderRecord;
static struct BREthereumBlockHeaderRecord genesisRinkebyBlockHeaderRecord;

const BREthereumBlockHeader ethereumMainnetBlockHeader = &genesisMainnetBlockHeaderRecord;
const BREthereumBlockHeader ethereumTestnetBlockHeader = &genesisTestnetBlockHeaderRecord;
const BREthereumBlockHeader ethereumRinkebyBlockHeader = &genesisRinkebyBlockHeaderRecord;

static void
initializeGenesisBlocks (void);

extern BREthereumBlockHeader
networkGetGenesisBlockHeader (BREthereumNetwork network) {
    static int needInitializeGenesisBlocks = 1;

    if (needInitializeGenesisBlocks) {
        needInitializeGenesisBlocks = 0;
        initializeGenesisBlocks();
    }

    BREthereumBlockHeader genesisHeader =
    (network == ethNetworkMainnet
     ? ethereumMainnetBlockHeader
     : (network == ethNetworkTestnet
        ? ethereumTestnetBlockHeader
        : (network == ethNetworkRinkeby
           ? ethereumRinkebyBlockHeader
           : NULL)));

    return genesisHeader == NULL ? NULL : ethBlockHeaderCopy(genesisHeader);
}

extern BREthereumBlock
networkGetGenesisBlock (BREthereumNetwork network) {
    BREthereumBlock block = ethBlockCreate(networkGetGenesisBlockHeader(network));
    ethBlockSetTotalDifficulty (block, ethBlockGetDifficulty(block));
    return block;
}

static void
initializeGenesisBlocks (void) {
    BREthereumBlockHeader header;

    // Mainnet
    /*
    {"jsonrpc":"2.0","id":1,"result":
        {
            "hash":"0xd4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3",
            "parentHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
            "sha3Uncles":"0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347",
            "miner":"0x0000000000000000000000000000000000000000",
            "stateRoot":"0xd7f8974fb5ac78d9ac099b9ad5018bedc2ce0a72dad1827a1709da30580f0544",
            "transactionsRoot":"0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
            "receiptsRoot":"0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
            "logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",

            "difficulty":"0x400000000",
            "number":"0x0",
            "gasLimit":"0x1388",
            "gasUsed":"0x0",
            "timestamp":"0x0",

            "extraData":"0x11bbe8db4e347b4e8c937c1c8370e4b5ed33adb3db69cbdb7a38e1e50b1b82fa",
            "mixHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
            "nonce":"0x0000000000000042",

            "size":"0x21c",
            "totalDifficulty":"0x400000000",

            "transactions":[],
            "uncles":[]
        }}
     */
    header = &genesisMainnetBlockHeaderRecord;
    header->hash = ethHashCreate("0xd4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3");
    header->parentHash = ethHashCreate("0x0000000000000000000000000000000000000000000000000000000000000000");
    header->ommersHash = ethHashCreate("0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347");
    header->beneficiary = ethAddressCreate("0x0000000000000000000000000000000000000000");
    header->stateRoot = ethHashCreate("0xd7f8974fb5ac78d9ac099b9ad5018bedc2ce0a72dad1827a1709da30580f0544");
    header->transactionsRoot = ethHashCreate("0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
    header->receiptsRoot = ethHashCreate("0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
    header->logsBloom = ethBloomFilterCreateString("00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    header->difficulty = uint256Create (0x400000000);
    header->number = 0x0;
    header->gasLimit = 0x1388;
    header->gasUsed = 0x0;
    header->timestamp = 0x0;
    hexDecode(header->extraData, 32, "11bbe8db4e347b4e8c937c1c8370e4b5ed33adb3db69cbdb7a38e1e50b1b82fa", 64);
    header->extraDataCount = 32;
    header->mixHash = ethHashCreate("0x0000000000000000000000000000000000000000000000000000000000000000");
    header->nonce = 0x0000000000000042;

    // Testnet
    /*
    {"jsonrpc":"2.0","id":1,"result":
        {"difficulty":"0x100000",
            "extraData":"0x3535353535353535353535353535353535353535353535353535353535353535",
            "gasLimit":"0x1000000",
            "gasUsed":"0x0",
            "hash":"0x41941023680923e0fe4d74a34bdac8141f2540e3ae90623718e47d66d1ca4a2d",
            "logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "miner":"0x0000000000000000000000000000000000000000",
            "mixHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
            "nonce":"0x0000000000000042",
            "number":"0x0",
            "parentHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
            "receiptsRoot":"0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
            "sha3Uncles":"0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347",
            "size":"0x21c",
            "stateRoot":"0x217b0bbcfb72e2d57e28f33cb361b9983513177755dc3f33ce3e7022ed62b77b",
            "timestamp":"0x0",
            "totalDifficulty":"0x100000",
            "transactions":[],
            "transactionsRoot": "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
            "uncles":[]}}
     */
    header = &genesisTestnetBlockHeaderRecord;
    header->hash = ethHashCreate("0x41941023680923e0fe4d74a34bdac8141f2540e3ae90623718e47d66d1ca4a2d");
    header->parentHash = ethHashCreate("0x0000000000000000000000000000000000000000000000000000000000000000");
    header->ommersHash = ethHashCreate("0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347");
    header->beneficiary = ethAddressCreate("0x0000000000000000000000000000000000000000");
    header->stateRoot = ethHashCreate("0x217b0bbcfb72e2d57e28f33cb361b9983513177755dc3f33ce3e7022ed62b77b");
    header->transactionsRoot = ethHashCreate("0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
    header->receiptsRoot = ethHashCreate("0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
    header->logsBloom = ethBloomFilterCreateString("00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    header->difficulty = uint256Create (0x100000);
    header->number = 0x0;
    header->gasLimit = 0x1000000;
    header->gasUsed = 0x0;
    header->timestamp = 0x0;
    hexDecode(header->extraData, 32, "3535353535353535353535353535353535353535353535353535353535353535", 64);
    header->extraDataCount = 32;
    header->mixHash = ethHashCreate("0x0000000000000000000000000000000000000000000000000000000000000000");
    header->nonce = 0x0000000000000042;

    // Rinkeby
    /*
    {"jsonrpc":"2.0","id":1,"result":
        {"difficulty":"0x1",
            "extraData":"0x52657370656374206d7920617574686f7269746168207e452e436172746d616e42eb768f2244c8811c63729a21a3569731535f067ffc57839b00206d1ad20c69a1981b489f772031b279182d99e65703f0076e4812653aab85fca0f00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "gasLimit":"0x47b760",
            "gasUsed":"0x0",
            "hash":"0x6341fd3daf94b748c72ced5a5b26028f2474f5f00d824504e4fa37a75767e177",
            "logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "miner":"0x0000000000000000000000000000000000000000",
            "mixHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
            "nonce":"0x0000000000000000",
            "number":"0x0",
            "parentHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
            "receiptsRoot":"0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
            "sha3Uncles":"0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347",
            "size":"0x29a",
            "stateRoot":"0x53580584816f617295ea26c0e17641e0120cab2f0a8ffb53a866fd53aa8e8c2d",
            "timestamp":"0x58ee40ba",
            "totalDifficulty":"0x1",
            "transactions":[],
            "transactionsRoot":"0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
            "uncles":[]}}
     */
    header = &genesisRinkebyBlockHeaderRecord;
    header->hash = ethHashCreate("0x6341fd3daf94b748c72ced5a5b26028f2474f5f00d824504e4fa37a75767e177");
    header->parentHash = ethHashCreate("0x0000000000000000000000000000000000000000000000000000000000000000");
    header->ommersHash = ethHashCreate("0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347");
    header->beneficiary = ethAddressCreate("0x0000000000000000000000000000000000000000");
    header->stateRoot = ethHashCreate("0x53580584816f617295ea26c0e17641e0120cab2f0a8ffb53a866fd53aa8e8c2d");
    header->transactionsRoot = ethHashCreate("0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
    header->receiptsRoot = ethHashCreate("0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
    header->logsBloom = ethBloomFilterCreateString("00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    header->difficulty = uint256Create (0x1);
    header->number = 0x0;
    header->gasLimit = 0x47b760;
    header->gasUsed = 0x0;
    header->timestamp = 0x58ee40ba;
    // TODO: Rinkeby ExtraData is oversized... ignore.
    hexDecode(header->extraData, 32, "3535353535353535353535353535353535353535353535353535353535353535", 64);
    header->extraDataCount = 32;
    header->mixHash = ethHashCreate("0x0000000000000000000000000000000000000000000000000000000000000000");
    header->nonce = 0x0000000000000000;
}

/// MARK: - Block Checkpoint

static BREthereumBlockCheckpoint *ethereumMainnetCheckpoints = NULL;
static size_t checkpointMainnetCount = 0;

static BREthereumBlockCheckpoint *ethereumTestnetCheckpoints = NULL;
static size_t checkpointTestnetCount = 0;

static BREthereumBlockCheckpoint *ethereumRinkebyCheckpoints = NULL;
static size_t checkpointRinkebyCount = 0;

static void ethBlockCheckpointInitialize (void) {

    BRCoreParseStatus status;

    // One time initialization of checkpoints
    BREthereumBlockCheckpoint ethMainnetCp [] = {
        {       0, ETHEREUM_HASH_INIT("d4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3"), { .std = "17179869184"              },          0 },
        { 4000000, ETHEREUM_HASH_INIT("b8a3f7f5cfc1748f91a684f20fe89031202cbadcd15078c49b85ec2a57f43853"), { .std = "469024977881526938386"    }, 1499633567 },
        { 4500000, ETHEREUM_HASH_INIT("43340a6d232532c328211d8a8c0fa84af658dbff1f4906ab7a7d4e41f82fe3a3"), { .std = "1386905480746946772236"   }, 1509953783 },
        { 5000000, ETHEREUM_HASH_INIT("7d5a4369273c723454ac137f48a4f142b097aa2779464e6505f1b1c5e37b5382"), { .std = "2285199027754071740498"   }, 1517319693 },
        { 5500000, ETHEREUM_HASH_INIT("2d3a154eee9f90666c6e824f11e15f2d60b05323a81254f60075c34a61ef124d"), { .std = "3825806101695195923560"   }, 1524611221 },
        { 6000000, ETHEREUM_HASH_INIT("be847be2bceb74e660daf96b3f0669d58f59dc9101715689a00ef864a5408f43"), { .std = "5484495551037046114587"   }, 1532118564 },
        { 6500000, ETHEREUM_HASH_INIT("70c81c3cb256b5b930f05b244d095cb4845e9808c48d881e3cc31d18ae4c3ae5"), { .std = "7174074700595750315193"   }, 1539330275 },
        { 7000000, ETHEREUM_HASH_INIT("17aa411843cb100e57126e911f51f295f5ddb7e9a3bd25e708990534a828c4b7"), { .std = "8545742390070173675989"   }, 1546466952 },
        { 7500000, ETHEREUM_HASH_INIT("b3cd718643c475c18efe4e2177777d38bdca3b9fb1eb9d9155db1365d6e5e8fd"), { .std = "9703290027978634211190"   }, 1554358137 },
        { 8000000, ETHEREUM_HASH_INIT("70c81c3cb256b5b930f05b244d095cb4845e9808c48d881e3cc31d18ae4c3ae5"), { .std = "10690776258913596267754"  }, 1561100149 },
        { 8500000, ETHEREUM_HASH_INIT("86ea48a5ae641bd1830426c45d14315862488a749c9e93fb228c23ca07237dab"), { .std = "11798521320484609094896"  }, 1567820847 },
        { 9000000, ETHEREUM_HASH_INIT("388f34dd94b899f65bbd23006ee93d61434a2f2a57053c9870466d8e142960e3"), { .std = "13014076996386893192616"  }, 1574706444 },
    };

    BREthereumBlockCheckpoint ethTestnetCp [] = {
        {       0, ETHEREUM_HASH_INIT("41941023680923e0fe4d74a34bdac8141f2540e3ae90623718e47d66d1ca4a2d"), { .std = "0x100000" },  0 }, // 1061 days  6 hrs ago (Jul-30-2015 03:26:13 PM +UTC)
    };

    BREthereumBlockCheckpoint ethRinkebyCp [] = {
        {       0, ETHEREUM_HASH_INIT("6341fd3daf94b748c72ced5a5b26028f2474f5f00d824504e4fa37a75767e177"), { .std = "0x01" },  0 }, //  439 days  6 hrs ago (Apr-12-2017 03:20:50 PM +UTC)
    };

    checkpointMainnetCount = sizeof(ethMainnetCp) / sizeof(BREthereumBlockCheckpoint);
    checkpointTestnetCount = sizeof(ethTestnetCp) / sizeof(BREthereumBlockCheckpoint);
    checkpointRinkebyCount = sizeof(ethRinkebyCp) / sizeof(BREthereumBlockCheckpoint);

    ethereumMainnetCheckpoints = calloc (sizeof(ethMainnetCp), 1);
    ethereumTestnetCheckpoints = calloc (sizeof(ethTestnetCp), 1);
    ethereumRinkebyCheckpoints = calloc (sizeof(ethRinkebyCp), 1);

    memcpy (ethereumMainnetCheckpoints, ethMainnetCp, sizeof(ethMainnetCp));
    memcpy (ethereumTestnetCheckpoints, ethTestnetCp, sizeof(ethTestnetCp));
    memcpy (ethereumRinkebyCheckpoints, ethRinkebyCp, sizeof(ethRinkebyCp));

    for (size_t index = 0; index < checkpointMainnetCount; index++) {
        BREthereumBlockCheckpoint *cp = &ethereumMainnetCheckpoints[index];
        cp->u.td = uint256CreateParse (cp->u.std, 0, &status);
        assert (CORE_PARSE_OK == status);
    }

    for (size_t index = 0; index < checkpointTestnetCount; index++) {
        BREthereumBlockCheckpoint *cp = &ethereumTestnetCheckpoints[index];
        cp->u.td = uint256CreateParse (cp->u.std, 0, &status);
        assert (CORE_PARSE_OK == status);
    }

    for (size_t index = 0; index < checkpointRinkebyCount; index++) {
        BREthereumBlockCheckpoint *cp = &ethereumRinkebyCheckpoints[index];
        cp->u.td = uint256CreateParse (cp->u.std, 0, &status);
        assert (CORE_PARSE_OK == status);
    }
}

// Run once initializer guarantees
static pthread_once_t ethNetworkParmsInitOnce = PTHREAD_ONCE_INIT;
static const BREthereumBlockCheckpoint *
ethBlockCheckpointFindForNetwork (BREthereumNetwork network,
                               size_t *count) {

    pthread_once (&ethNetworkParmsInitOnce, ethBlockCheckpointInitialize);
    assert (NULL != count);

    if (network == ethNetworkMainnet) {
        *count = checkpointMainnetCount;
        return ethereumMainnetCheckpoints;
    }

    if (network == ethNetworkTestnet) {
        *count = checkpointTestnetCount;
        return ethereumTestnetCheckpoints;
    }

    if (network == ethNetworkRinkeby) {
        *count = checkpointRinkebyCount;
        return ethereumRinkebyCheckpoints;
    }
    *count = 0;
    return NULL;
}

extern const BREthereumBlockCheckpoint *
ethBlockCheckpointLookupLatest (BREthereumNetwork network) {
    size_t count;
    const BREthereumBlockCheckpoint *checkpoints = ethBlockCheckpointFindForNetwork(network, &count);
    return &checkpoints[count - 1];
}

extern const BREthereumBlockCheckpoint *
ethBlockCheckpointLookupByNumber (BREthereumNetwork network,
                               uint64_t number) {
    size_t count;
    const BREthereumBlockCheckpoint *checkpoints = ethBlockCheckpointFindForNetwork(network, &count);
    for (size_t index = count; index > 0; index--)
        if (checkpoints[index - 1].number <= number)
            return &checkpoints[index - 1];
    return NULL;
}

extern const BREthereumBlockCheckpoint *
ethBlockCheckpointLookupByTimestamp (BREthereumNetwork network,
                                  uint64_t timestamp) {

    // Backup the timestamp.
    if (timestamp >= BLOCK_CHECKPOINT_TIMESTAMP_SAFETY)
        timestamp -= BLOCK_CHECKPOINT_TIMESTAMP_SAFETY;

    size_t count;
    const BREthereumBlockCheckpoint *checkpoints = ethBlockCheckpointFindForNetwork(network, &count);
    for (size_t index = count; index > 0; index--)
        if (checkpoints[index - 1].timestamp <= timestamp)
            return &checkpoints[index - 1];
    return NULL;
}

extern BREthereumBlockHeader
ethBlockCheckpointCreatePartialBlockHeader (const BREthereumBlockCheckpoint *checkpoint) {
    return createBlockHeaderMinimal (checkpoint->hash, checkpoint->number, checkpoint->timestamp, UINT256_ZERO);
}


// GETH:
/*
 type Header struct {
      ParentHash  common.Hash    `json:"parentHash"       gencodec:"required"`
      UncleHash   common.Hash    `json:"sha3Uncles"       gencodec:"required"`
      Coinbase    common.Address `json:"miner"            gencodec:"required"`
      Root        common.Hash    `json:"stateRoot"        gencodec:"required"`
      TxHash      common.Hash    `json:"transactionsRoot" gencodec:"required"`
      ReceiptHash common.Hash    `json:"receiptsRoot"     gencodec:"required"`
      Bloom       Bloom          `json:"logsBloom"        gencodec:"required"`
      Difficulty  *big.Int       `json:"difficulty"       gencodec:"required"`
      Number      *big.Int       `json:"number"           gencodec:"required"`
      GasLimit    uint64         `json:"gasLimit"         gencodec:"required"`
      GasUsed     uint64         `json:"gasUsed"          gencodec:"required"`
      Time        *big.Int       `json:"timestamp"        gencodec:"required"`
      Extra       []byte         `json:"extraData"        gencodec:"required"`
      MixDigest   common.Hash    `json:"mixHash"          gencodec:"required"`
      Nonce       BlockNonce     `json:"nonce"            gencodec:"required"`
 }

 // HashNoNonce returns the hash which is used as input for the proof-of-work search.
 func (h *Header) HashNoNonce() common.Hash {
   return rlpHash([]interface{}{
     h.ParentHash,
      h.UncleHash,
      h.Coinbase,
      h.Root,
      h.TxHash,
      h.ReceiptHash,
      h.Bloom,
      h.Difficulty,
      h.Number,
      h.GasLimit,
      h.GasUsed,
      h.Time,
      h.Extra,
   })
 }
 */

/* Block Headers (10)
 ETH: LES:   L 10: [
 ETH: LES:     L 15: [
 ETH: LES:       I 32: 0xb853f283c777f628e28be62a80850d98a5a5e9c4e86afb0e785f7a222ebf67f8
 ETH: LES:       I 32: 0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347
 ETH: LES:       I 20: 0xb2930b35844a230f00e51431acae96fe543a0347
 ETH: LES:       I 32: 0x24dc4e9f66f026b0569270e8ef95d34c275721ff6eecab029afe11c43249e046
 ETH: LES:       I 32: 0xa7f796d7cd98ed2f3fb70ecb9a48939825f1a3d0364eb995c49151761ce9659c
 ETH: LES:       I 32: 0xed45b3ad4bb2b46bf1e49a7925c63042aa41d5af7372db334142152d5a7ec422
 ETH: LES:       I256: 0x00a61039e24688a200002e10102021116002220040204048308206009928412802041520200201115014888000c00080020a002021308850c60d020d00200188062900c83288401115821a1c101200d00318080088df000830c1938002a018040420002a22201000680a391c91610e4884682a00910446003da000b9501020009c008205091c0b04108c000410608061a07042141001820440d404042002a4234f00090845c1544820140430552592100352140400039000108e052110088800000340422064301701c8212008820c4648a020a482e90a0268480000400021800110414680020205002400808012c6248120027c4121119802240010a2181983
 ETH: LES:       I  7: 0x0baf848614eb16
 ETH: LES:       I  3: 0x5778a9
 ETH: LES:       I  3: 0x7a121d
 ETH: LES:       I  3: 0x793640
 ETH: LES:       I  4: 0x5b1596f8
 ETH: LES:       I  5: 0x73696e6731
 ETH: LES:       I 32: 0xbcefde2594b8b501c985c2f0f411c69baee727c4f90a74ef28b7f2b59a00f7c2
 ETH: LES:       I  8: 0x07ab2de40005d6f7
 ETH: LES:     ]
 ETH: LES:     L 15: [
 ETH: LES:       I 32: 0x8996b1f91f060302350f1cb9014a11d48fd1b42eeeacf18ce4762b94c69656fa
 ETH: LES:       I 32: 0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347
 ETH: LES:       I 20: 0xea674fdde714fd979de3edf0f56aa9716b898ec8
 ETH: LES:       I 32: 0x4a91466c8a43f6c61d47ae2680072ec0ca9d4077752b947bc0913f6f52823476
 ETH: LES:       I 32: 0xfc9df67c1dc39852692763387da8039d824e9956d936338184f51c6734e8cc9f
 ETH: LES:       I 32: 0x2cfe437b4cac28ccbb68373b6107e4e1c8fabdbfe8941d256367d4ab9e97e3e4
 ETH: LES:       I256: 0xe01310094f66290e1101240443c1f801110021114120280c00088524322a016c2c2212b0012302001094b000009404a10018c03040208082c600c64c01280101039141e008a2c9198186044c1882541400580c36026194088033a15a08003400c5200624020c010033453168429059cd066310252a04680618226215548466e4006180038a24544804c209e11046012c008046b100065c648050084c0a15222aba6e800030c0148c2301162034298812550c060c20018470190a4141280920c110124052001d31444a30030116a42b0001c36427a888c817281110482046a04003183121a4b00042a4c6008048208fa444200204280222cc008c148446101092
 ETH: LES:       I  7: 0x0bab23f73e93e6
 ETH: LES:       I  3: 0x5778a7
 ETH: LES:       I  3: 0x7a121d
 ETH: LES:       I  3: 0x7a0453
 ETH: LES:       I  4: 0x5b1596ed
 ETH: LES:       I 21: 0x65746865726d696e652d6177732d61736961312d33
 ETH: LES:       I 32: 0x057ca051122a8e18dbd6dadaade0f7c5b877623a4ae706facce5de7d1f924858
 ETH: LES:       I  8: 0xcd9b80400100b43c
 ETH: LES:     ]
 ETH: LES:    ...
 ETH: LES:  ]
 */


/* BLock Bodies
 ETH: LES:   L  2: [
 ETH: LES:     L  2: [
 ETH: LES:       L117: [
 ETH: LES:         L  9: [
 ETH: LES:           I  1: 0x09
 ETH: LES:           I  5: 0x0ba43b7400
 ETH: LES:           I  2: 0x5208
 ETH: LES:           I 20: 0x5521a68d4f8253fc44bfb1490249369b3e299a4a
 ETH: LES:           I  8: 0x154f1f6cc6457c00
 ETH: LES:           I  0: 0x
 ETH: LES:           I  1: 0x26
 ETH: LES:           I 32: 0x317428668e86eedc101a2ac3344c66dca791b078557e018a4524d86da3529de2
 ETH: LES:           I 32: 0x40446aa978d382ad2a12549713ad94cbe654aa4853ab68414566a0638689f6a9
 ETH: LES:         ]
 ETH: LES:         L  9: [
 ETH: LES:           I  2: 0x0308
 ETH: LES:           I  5: 0x0ba43b7400
 ETH: LES:           I  2: 0xb78d
 ETH: LES:           I 20: 0x58a4884182d9e835597f405e5f258290e46ae7c2
 ETH: LES:           I  0: 0x
 ETH: LES:           I 68: 0xa9059cbb0000000000000000000000004a2ce805877250dd17e14f4421d66d2a9717725a0000000000000000000000000000000000000000000000273f96e31883e34000
 ETH: LES:           I  1: 0x26
 ETH: LES:           I 32: 0xf02eac3ab7c93ccdea73b6cbcc3483d0d03cdb374c07b3021461e8bb108234fa
 ETH: LES:           I 32: 0x511e4c85506e11c46198fd61982f5cc05c9c06f792aaee47f5ddf14ced764b1d
 ETH: LES:         ]
 ETH: LES:         ...
 ETH: LES:       ]
 ETH: LES:       L  0: []
 ETH: LES:     ]
 ETH: LES:    ...
 ETH: LES   ]
 */
