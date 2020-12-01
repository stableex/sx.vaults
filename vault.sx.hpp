#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>

#include <optional>

using namespace eosio;
using namespace std;

static constexpr int64_t asset_mask{(1LL << 62) - 1};
static constexpr int64_t asset_max{ asset_mask }; //  4611686018427387903
static constexpr name TOKEN_CONTRACT = "token.sx"_n;

namespace sx {
class [[eosio::contract("vault.sx")]] vault : public eosio::contract {
public:
    using contract::contract;
    /**
     * ## TABLE `vault`
     *
     * - `{asset} balance` - vault deposit balance
     * - `{asset} staked` - vault staked balance
     * - `{asset} supply` - vault active supply
     * - `{name} [account="vault.sx"]` - (optional) account to/from deposit balance
     * - `{time_point_sec} last_updated` - last updated timestamp
     *
     * ### example
     *
     * ```json
     * {
     *   "balance": {"quantity": "100.0000 EOS", "contract": "eosio.token"},
     *   "staked": {"quantity": "0.0000 EOS", "contract": "eosio.token"},
     *   "supply": {"quantity": "1000000.0000 SXEOS", "contract": "token.sx"},
     *   "account": "vault.sx",
     *   "last_updated": "2020-11-23T00:00:00"
     * }
     * ```
     */
    struct [[eosio::table("vault")]] vault_row {
        extended_asset          balance;
        extended_asset          staked;
        extended_asset          supply;
        name                    account;
        time_point_sec          last_updated;

        uint64_t primary_key() const { return balance.quantity.symbol.code().raw(); }
        uint64_t by_supply() const { return supply.quantity.symbol.code().raw(); }
    };
    typedef eosio::multi_index< "vault"_n, vault_row,
        indexed_by<"bysupply"_n, const_mem_fun<vault_row, uint64_t, &vault_row::by_supply>>
    > vault_table;

    /**
     * ## ACTION `setvault`
     *
     * Set initial vault deposit balance & supply
     *
     * - **authority**: `get_self()`
     *
     * ### params
     *
     * - `{extended_symbol} deposit` - deposit symbol
     * - `{symbol_code} supply_id` - liquidity supply symbol
     * - `{name} [account="vault.sx"]` - (optional) account to/from deposit balance
     *
     * ### Example
     *
     * ```bash
     * $ cleos push action vault.sx setvault '[["4,EOS", "eosio.token"], "SXEOS"]' -p vault.sx
     * ```
     */
    [[eosio::action]]
    void setvault( const extended_symbol ext_deposit, const symbol_code supply_id, const name account = "vault.sx"_n );

    [[eosio::action]]
    void update( const symbol_code id );

    /**
     * Notify contract when any token transfer notifiers relay contract
     */
    [[eosio::on_notify("*::transfer")]]
    void on_transfer( const name from, const name to, const asset quantity, const std::string memo );

    // static actions
    using setvault_action = eosio::action_wrapper<"setvault"_n, &sx::vault::setvault>;

private:
    // eosio.token helper
    void transfer( const name from, const name to, const extended_asset value, const string memo );
    void create( const extended_symbol value );
    void retire( const extended_asset value, const string memo );
    void issue( const extended_asset value, const string memo );

    // vault
    extended_asset calculate_issue( const symbol_code id, const asset payment );
    extended_asset calculate_retire( const symbol_code id, const asset payment );
};
}
