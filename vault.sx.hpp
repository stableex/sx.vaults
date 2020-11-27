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
     * ## TABLE `pairs`
     *
     * - `{extended_symbol} id` - vault symbol ID
     * - `{extended_symbol} deposit` - accepted deposit symbol
     * - `{asset} balance` - vault balance
     * - `{asset} supply` - vault supply
     * - `{time_point_sec} last_updated` - last updated timestamp
     *
     * ### example
     *
     * ```json
     * {
     *   "id": {"symbol": "4,SXEOS", "contract": "token.sx"},
     *   "deposit": {"symbol": "4,EOS", "contract": "eosio.token"},
     *   "balance": "100.0000 EOS",
     *   "supply": "1000000.0000 SXEOS",
     *   "last_updated": "2020-11-23T00:00:00"
     * }
     * ```
     */
    struct [[eosio::table("pairs")]] pairs_row {
        extended_symbol     id;
        extended_symbol     deposit;
        asset               balance;
        asset               supply;
        time_point_sec      last_updated;

        uint64_t primary_key() const { return id.get_symbol().code().raw(); }
        uint64_t by_deposit() const { return deposit.get_symbol().code().raw(); }
    };
    typedef eosio::multi_index< "pairs"_n, pairs_row,
        indexed_by<"bydeposit"_n, const_mem_fun<pairs_row, uint64_t, &pairs_row::by_deposit>>
    > pairs_table;

    [[eosio::action]]
    void setpair( const symbol_code id, const extended_symbol deposit );

    /**
     * Notify contract when any token transfer notifiers relay contract
     */
    [[eosio::on_notify("*::transfer")]]
    void on_transfer( const name from, const name to, const asset quantity, const std::string memo );

    // static actions
    using setpair_action = eosio::action_wrapper<"setpair"_n, &sx::vault::setpair>;

private:
    // token contract
    void create( const extended_symbol value );
    void transfer( const name from, const name to, const extended_asset value, const string memo );
    void retire( const extended_asset value, const string memo );
    void issue( const extended_asset value, const string memo );

    // vault
    extended_asset calculate_issue( const extended_symbol id, const asset payment );
    extended_asset calculate_retire( const extended_symbol id, const asset payment );
};
}