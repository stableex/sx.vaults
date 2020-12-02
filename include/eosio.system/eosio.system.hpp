#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

#include <optional>
#include <string>

using namespace eosio;
using namespace std;

namespace eosiosystem {

// Voter info. Voter info stores information about the voter:
// - `owner` the voter
// - `proxy` the proxy set by the voter, if any
// - `producers` the producers approved by this voter if no proxy set
// - `staked` the amount staked
struct [[eosio::table, eosio::contract("eosio.system")]] voter_info {
    name                owner;     /// the voter
    name                proxy;     /// the proxy set by the voter, if any
    std::vector<name>   producers; /// the producers approved by this voter if no proxy set
    int64_t             staked = 0;

    //  Every time a vote is cast we must first "undo" the last vote weight, before casting the
    //  new vote weight.  Vote weight is calculated as:
    //  stated.amount * 2 ^ ( weeks_since_launch/weeks_per_year)
    double              last_vote_weight = 0; /// the vote weight cast the last time the vote was updated

    // Total vote weight delegated to this voter.
    double              proxied_vote_weight= 0; /// the total vote weight delegated to this voter as a proxy
    bool                is_proxy = 0; /// whether the voter is a proxy for others

    uint32_t            flags1 = 0;
    uint32_t            reserved2 = 0;
    eosio::asset        reserved3;

    uint64_t primary_key()const { return owner.value; }

    enum class flags1_fields : uint32_t {
        ram_managed = 1,
        net_managed = 2,
        cpu_managed = 4
    };

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE( voter_info, (owner)(proxy)(producers)(staked)(last_vote_weight)(proxied_vote_weight)(is_proxy)(flags1)(reserved2)(reserved3) )
};

typedef eosio::multi_index< "voters"_n, voter_info >  voters_table;

// `rex_pool` structure underlying the rex pool table. A rex pool table entry is defined by:
// - `version` defaulted to zero,
// - `total_lent` total amount of CORE_SYMBOL in open rex_loans
// - `total_unlent` total amount of CORE_SYMBOL available to be lent (connector),
// - `total_rent` fees received in exchange for lent  (connector),
// - `total_lendable` total amount of CORE_SYMBOL that have been lent (total_unlent + total_lent),
// - `total_rex` total number of REX shares allocated to contributors to total_lendable,
// - `namebid_proceeds` the amount of CORE_SYMBOL to be transferred from namebids to REX pool,
// - `loan_num` increments with each new loan
struct [[eosio::table,eosio::contract("eosio.system")]] rex_pool {
    uint8_t    version = 0;
    asset      total_lent;
    asset      total_unlent;
    asset      total_rent;
    asset      total_lendable;
    asset      total_rex;
    asset      namebid_proceeds;
    uint64_t   loan_num = 0;

    uint64_t primary_key()const { return 0; }
};

typedef eosio::multi_index< "rexpool"_n, rex_pool > rex_pool_table;

// `rex_fund` structure underlying the rex fund table. A rex fund table entry is defined by:
// - `version` defaulted to zero,
// - `owner` the owner of the rex fund,
// - `balance` the balance of the fund.
struct [[eosio::table,eosio::contract("eosio.system")]] rex_fund {
    uint8_t version = 0;
    name    owner;
    asset   balance;

    uint64_t primary_key()const { return owner.value; }
};

typedef eosio::multi_index< "rexfund"_n, rex_fund > rex_fund_table;


struct [[eosio::table, eosio::contract("eosio.system")]] refund_request {
    name            owner;
    time_point_sec  request_time;
    eosio::asset    net_amount;
    eosio::asset    cpu_amount;

    bool is_empty()const { return net_amount.amount == 0 && cpu_amount.amount == 0; }
    uint64_t  primary_key()const { return owner.value; }

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE( refund_request, (owner)(request_time)(net_amount)(cpu_amount) )
};
typedef eosio::multi_index< "refunds"_n, refund_request >      refunds_table;

// `rex_balance` structure underlying the rex balance table. A rex balance table entry is defined by:
// - `version` defaulted to zero,
// - `owner` the owner of the rex fund,
// - `vote_stake` the amount of CORE_SYMBOL currently included in owner's vote,
// - `rex_balance` the amount of REX owned by owner,
// - `matured_rex` matured REX available for selling
struct [[eosio::table,eosio::contract("eosio.system")]] rex_balance {
    uint8_t version = 0;
    name    owner;
    asset   vote_stake;
    asset   rex_balance;
    int64_t matured_rex = 0;
    std::deque<std::pair<time_point_sec, int64_t>> rex_maturities; /// REX daily maturity buckets

    uint64_t primary_key()const { return owner.value; }
};

typedef eosio::multi_index< "rexbal"_n, rex_balance > rex_balance_table;

/**
* The EOSIO system contract. The EOSIO system contract governs ram market, voters, producers, global state.
*/
class [[eosio::contract("eosio.system")]] system_contract {
        /**
         * Deposit to REX fund action. Deposits core tokens to user REX fund.
         * All proceeds and expenses related to REX are added to or taken out of this fund.
         * An inline transfer from 'owner' liquid balance is executed.
         * All REX-related costs and proceeds are deducted from and added to 'owner' REX fund,
         *    with one exception being buying REX using staked tokens.
         * Storage change is billed to 'owner'.
         *
         * @param owner - REX fund owner account,
         * @param amount - amount of tokens to be deposited.
         */
        [[eosio::action]]
        void deposit( const name& owner, const asset& amount );

        /**
         * Withdraw from REX fund action, withdraws core tokens from user REX fund.
         * An inline token transfer to user balance is executed.
         *
         * @param owner - REX fund owner account,
         * @param amount - amount of tokens to be withdrawn.
         */
        [[eosio::action]]
        void withdraw( const name& owner, const asset& amount );

        /**
         * Buyrex action, buys REX in exchange for tokens taken out of user's REX fund by transfering
         * core tokens from user REX fund and converts them to REX stake. By buying REX, user is
         * lending tokens in order to be rented as CPU or NET resourses.
         * Storage change is billed to 'from' account.
         *
         * @param from - owner account name,
         * @param amount - amount of tokens taken out of 'from' REX fund.
         *
         * @pre A voting requirement must be satisfied before action can be executed.
         * @pre User must vote for at least 21 producers or delegate vote to proxy before buying REX.
         *
         * @post User votes are updated following this action.
         * @post Tokens used in purchase are added to user's voting power.
         * @post Bought REX cannot be sold before 4 days counting from end of day of purchase.
         */
        [[eosio::action]]
        void buyrex( const name& from, const asset& amount );

        /**
         * Sellrex action, sells REX in exchange for core tokens by converting REX stake back into core tokens
         * at current exchange rate. If order cannot be processed, it gets queued until there is enough
         * in REX pool to fill order, and will be processed within 30 days at most. If successful, user
         * votes are updated, that is, proceeds are deducted from user's voting power. In case sell order
         * is queued, storage change is billed to 'from' account.
         *
         * @param from - owner account of REX,
         * @param rex - amount of REX to be sold.
         */
        [[eosio::action]]
        void sellrex( const name& from, const asset& rex );

        /**
         * Updaterex action, updates REX owner vote weight to current value of held REX tokens.
         *
         * @param owner - REX owner account.
         */
        [[eosio::action]]
        void updaterex( const name& owner );

        /**
         * Vote producer action, votes for a set of producers. This action updates the list of `producers` voted for,
         * for `voter` account. If voting for a `proxy`, the producer votes will not change until the
         * proxy updates their own vote. Voter can vote for a proxy __or__ a list of at most 30 producers.
         * Storage change is billed to `voter`.
         *
         * @param voter - the account to change the voted producers for,
         * @param proxy - the proxy to change the voted producers for,
         * @param producers - the list of producers to vote for, a maximum of 30 producers is allowed.
         *
         * @pre Producers must be sorted from lowest to highest and must be registered and active
         * @pre If proxy is set then no producers can be voted for
         * @pre If proxy is set then proxy account must exist and be registered as a proxy
         * @pre Every listed producer or proxy must have been previously registered
         * @pre Voter must authorize this action
         * @pre Voter must have previously staked some EOS for voting
         * @pre Voter->staked must be up to date
         *
         * @post Every producer previously voted for will have vote reduced by previous vote weight
         * @post Every producer newly voted for will have vote increased by new vote amount
         * @post Prior proxy will proxied_vote_weight decremented by previous vote weight
         * @post New proxy will proxied_vote_weight incremented by new vote weight
         */
        [[eosio::action]]
        void voteproducer( const name& voter, const name& proxy, const std::vector<name>& producers );

        //  action wrappers
        using deposit_action = eosio::action_wrapper<"deposit"_n, &system_contract::deposit>;
        using withdraw_action = eosio::action_wrapper<"withdraw"_n, &system_contract::withdraw>;
        using buyrex_action = eosio::action_wrapper<"buyrex"_n, &system_contract::buyrex>;
        using sellrex_action = eosio::action_wrapper<"sellrex"_n, &system_contract::sellrex>;
        using updaterex_action = eosio::action_wrapper<"updaterex"_n, &system_contract::updaterex>;
        using voteproducer_action = eosio::action_wrapper<"voteproducer"_n, &system_contract::voteproducer>;
};
}