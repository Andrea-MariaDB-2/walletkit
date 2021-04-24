//
//  BRCryptoCurrency.h
//  WalletKitCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoCurrency_h
#define BRCryptoCurrency_h

#include "BRCryptoBase.h"

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * A currency represents a specific asset and is the 'type of quantity' for a amount.
     *
     * @discussion Abstractly, a currency is anologous to 'mass' or 'time' from which units for
     * kilograms and seconds are derived.
     */
    typedef struct BRCryptoCurrencyRecord *BRCryptoCurrency;

    /**
     * Create a currency
     *
     * @param uids the 'unique identifier string'.  This will be globally unique
     * @param name the name, such as "The Breadwallet Token"
     * @param code the code, such as "BRD"
     * @param type the type, such a 'erc20'
     * @param issuer the issuer or NULL.  For currency derived from an ERC20 token, the issue must
     *    be a 'hex string' (starts with '0x') representing the Smart Contract Address.
     *
     * @return a currency
     */
    extern BRCryptoCurrency
    cryptoCurrencyCreate (const char *uids,
                          const char *name,
                          const char *code,
                          const char *type,
                          const char *issuer);

    /**
     * Get the currency's unique-identifier.  This uniquely identifes all the currencies, be they
     * Bitcoin, Ethereum or others, in WalletKit.
     */
    extern const char *
    cryptoCurrencyGetUids (BRCryptoCurrency currency);

    /**
     * Get the currency's name.  This would be the full name, such as "Bitcoin" or "Ethereum", for
     * a network currency.
     */
    extern const char *
    cryptoCurrencyGetName (BRCryptoCurrency currency);

    /**
     * Get the currency's code.  This would be "btc" or "eth" for a network's native currencu.  For
     * Ethereum ERC20 tokens or other tokens this would be a suitable symbol, like "brd".
     */
    extern const char *
    cryptoCurrencyGetCode (BRCryptoCurrency currency);

    /**
     * Return the currency's type.  This is "native" for a network's native currency; for Ethereum
     * erc20 tokens this is "erc20"
     */
    extern const char *
    cryptoCurrencyGetType (BRCryptoCurrency currency);

    /**
     * Return the currency issuer or NULL if there is none.  For an ERC20-based currency, the
     * issuer will be the Smart Contract Address.
     *
     * @param currency the currency
     *
     *@return the issuer as a string or NULL
     */
    extern const char *
    cryptoCurrencyGetIssuer (BRCryptoCurrency currency);

    /**
     * Check if a currency's uids matches the provided uids.
     */
    extern bool
    cryptoCurrencyHasUids (BRCryptoCurrency currency,
                           const char *uids);

    /**
     * Check of two currencies are identical based on their uids.
     */
    extern BRCryptoBoolean
    cryptoCurrencyIsIdentical (BRCryptoCurrency c1,
                               BRCryptoCurrency c2);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoCurrency, cryptoCurrency);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoCurrency_h */
