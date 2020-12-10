#include <eosio.token/eosio.token.hpp>
#include <eosio.system/eosio.system.hpp>

#include "vaults.sx.hpp"

[[eosio::action]]
void sx::vaults::update( const symbol_code id )
{
    sx::vaults::vault_table _vault( get_self(), get_self().value );

    auto& vault = _vault.get( id.raw(), "vault does not exist" );

    // helpers
    const name contract = vault.deposit.contract;
    const symbol sym = vault.deposit.quantity.symbol;
    const name account = vault.account;

    // only vault account or contract is allowed to update the account staked/deposit balance externally
    if ( !has_auth( get_self() ) ) require_auth( account );

    // get balance from account
    const asset balance = eosio::token::get_balance( contract, account, sym.code() );
    asset staked = { 0, balance.symbol };

    // ADD STAKING EXCEPTIONS (REX/staked/deposit)
    if ( sym == EOS ) {
        staked.amount += get_eos_voters_staked( account );
        staked.amount += get_eos_rex_fund( account );
        staked.amount += get_eos_refund( account );
    }

    // update balance
    _vault.modify( vault, get_self(), [&]( auto& row ) {
        row.deposit.quantity = balance + staked;
        row.staked.quantity = staked;
        row.last_updated = current_time_point();
    });
}

int64_t sx::vaults::get_eos_refund( const name owner )
{
    eosiosystem::refunds_table _refunds( "eosio"_n, owner.value );
    auto itr = _refunds.find( owner.value );
    if ( itr != _refunds.end() ) return itr->net_amount.amount + itr->cpu_amount.amount;
    return 0;
}

int64_t sx::vaults::get_eos_voters_staked( const name owner )
{
    eosiosystem::voters_table _voters( "eosio"_n, "eosio"_n.value );
    auto itr = _voters.find( owner.value );
    if ( itr != _voters.end() ) return itr->staked;
    return 0;
}

int64_t sx::vaults::get_eos_rex_fund( const name owner )
{
    eosiosystem::rex_fund_table _rex_fund( "eosio"_n, "eosio"_n.value );
    auto itr = _rex_fund.find( owner.value );
    if ( itr != _rex_fund.end() ) return itr->balance.amount;
    return 0;
}

/**
 * Notify contract when any token transfer notifiers relay contract
 */
[[eosio::on_notify("*::transfer")]]
void sx::vaults::on_transfer( const name from, const name to, const asset quantity, const string memo )
{
    // authenticate incoming `from` account
    require_auth( from );

    // ignore outgoing transfers
    if ( to != get_self() ) return;

    // incoming token contract
    const name contract = get_first_receiver();

    // table & index
    sx::vaults::vault_table _vault( get_self(), get_self().value );
    auto _vault_by_supply = _vault.get_index<"bysupply"_n>();

    // iterators
    auto deposit_itr = _vault.find( quantity.symbol.code().raw() );
    auto supply_itr = _vault_by_supply.find( quantity.symbol.code().raw() );

    // ignore incoming transfer from vault account
    if ( from == deposit_itr->account || from == supply_itr->account ) return;

    // deposit - handle issuance (ex: EOS => SXEOS)
    if ( deposit_itr != _vault.end() ) {
        // input validation
        check( contract == deposit_itr->deposit.contract, "deposit token contract does not match" );
        const symbol_code id = deposit_itr->deposit.quantity.symbol.code();
        const name account = deposit_itr->account;

        // calculate issuance supply token by providing balance
        const extended_asset out = calculate_issue( id, quantity );

        // update internal balance & supply
        _vault.modify( deposit_itr, get_self(), [&]( auto& row ) {
            row.deposit.quantity += quantity;
            row.supply += out;
            row.last_updated = current_time_point();
        });

        // (OPTIONAL) send funds to vault account
        if ( account != get_self() ) transfer( get_self(), account, { quantity, contract }, get_self().to_string() );

        // issue & transfer to sender
        issue( out, "issue" );
        transfer( get_self(), from, out, get_self().to_string() );

    // withdraw - handle retire (ex: SXEOS => EOS)
    } else if ( supply_itr != _vault_by_supply.end() ) {
        // input validation
        check( contract == supply_itr->supply.contract, "supply token contract does not match" );
        const symbol_code id = supply_itr->deposit.quantity.symbol.code();
        const name account = supply_itr->account;

        // retire - burn vault tokens (ex: SXEOS => 🔥)
        if ( memo == "🔥" ) {
            _vault_by_supply.modify( supply_itr, get_self(), [&]( auto& row ) {
                row.supply.quantity -= quantity;
                row.last_updated = current_time_point();
            });
        // redeem - calculate amount from retiring supply token
        } else {
            const extended_asset out = calculate_retire( id, quantity );

            // update internal deposit & supply
            _vault_by_supply.modify( supply_itr, get_self(), [&]( auto& row ) {
                row.deposit -= out;
                row.supply.quantity -= quantity;
                row.last_updated = current_time_point();

                // deposit (liquid balance) must be equal or above staked amount
                check( row.deposit >= row.staked, "maximum withdraw is " + row.deposit.quantity.to_string() + ", please wait for deposit balance to equal or exceed staked amount");
            });
            // (OPTIONAL) retrieve funds from vault account
            if ( account != get_self() ) transfer( account, get_self(), out, get_self().to_string() );

            // send underlying assets to sender
            transfer( get_self(), from, out, get_self().to_string() );
        }

        // retire vault liquidity supply token
        retire( { quantity, contract }, "retire" );
    } else {
        check( false, "incoming transfer asset symbol not supported");
    }
}

[[eosio::action]]
void sx::vaults::setvault( const extended_symbol deposit, const symbol_code supply_id, const name account )
{
    require_auth( get_self() );
    sx::vaults::vault_table _vault( get_self(), get_self().value );
    eosio::token::stats _stats( get_self(), get_self().value );

    // ID must use same symbol precision as deposit
    const extended_symbol supply_symbol = {{ supply_id, deposit.get_symbol().precision() }, TOKEN_CONTRACT };

    // deposit token must exists
    const symbol_code id = deposit.get_symbol().code();
    const asset supply = eosio::token::get_supply( deposit.get_contract(), id );
    check( supply.amount > 0, "deposit has no supply");
    check( deposit.get_symbol() == supply.symbol, "deposit symbol precision mismatch");

    // vault account must exists
    check( is_account(account), "account does not exists");

    // create vault supply token if does not exists
    int64_t supply_amount = 0;
    const auto stats = _stats.find( supply_id.raw() );
    if ( stats == _stats.end() ) create( supply_symbol );
    else supply_amount = eosio::token::get_supply( TOKEN_CONTRACT, supply_id ).amount;

    // vault content
    auto insert = [&]( auto & row ) {
        row.deposit = { 0, deposit };
        row.staked = { 0, deposit };
        row.supply = { supply_amount, supply_symbol };
        row.account = account;
        row.last_updated = current_time_point();
    };

    // create/modify vault
    auto itr = _vault.find( id.raw() );
    if ( itr == _vault.end() ) _vault.emplace( get_self(), insert );
    else _vault.modify( itr, get_self(), insert );

    // update deposit & staked asset balances
    update( id );
}

extended_asset sx::vaults::calculate_issue( const symbol_code id, const asset payment )
{
    sx::vaults::vault_table _vault( get_self(), get_self().value );
    const auto vault = _vault.get( id.raw(), "id does not exists in vault" );
    const int64_t ratio = 10000;

    // initialize vault supply
    if ( vault.supply.quantity.amount == 0 ) return { payment.amount * ratio, vault.supply.get_extended_symbol() };

    // issue & redeem supply calculation
    // calculations based on fill REX order
    // https://github.com/EOSIO/eosio.contracts/blob/f6578c45c83ec60826e6a1eeb9ee71de85abe976/contracts/eosio.system/src/rex.cpp#L775-L779
    const int64_t S0 = vault.deposit.quantity.amount; // vault
    const int64_t S1 = S0 + payment.amount; // payment
    const int64_t R0 = vault.supply.quantity.amount; // supply
    const int64_t R1 = (uint128_t(S1) * R0) / S0;

    return { R1 - R0, vault.supply.get_extended_symbol() };
}

extended_asset sx::vaults::calculate_retire( const symbol_code id, const asset payment )
{
    sx::vaults::vault_table _vault( get_self(), get_self().value );
    const auto vault = _vault.get( id.raw(), "id does not exists in vault" );

    // issue & redeem supply calculation
    // calculations based on add to REX pool
    // https://github.com/EOSIO/eosio.contracts/blob/f6578c45c83ec60826e6a1eeb9ee71de85abe976/contracts/eosio.system/src/rex.cpp#L772
    const int64_t S0 = vault.deposit.quantity.amount;
    const int64_t R0 = vault.supply.quantity.amount;
    const int64_t p  = (uint128_t(payment.amount) * S0) / R0;

    return { p, vault.deposit.get_extended_symbol() };
}

void sx::vaults::create( const extended_symbol value )
{
    eosio::token::create_action create( value.get_contract(), { value.get_contract(), "active"_n });
    create.send( get_self(), asset{ asset_max, value.get_symbol() } );
}

void sx::vaults::issue( const extended_asset value, const string memo )
{
    eosio::token::issue_action issue( value.contract, { get_self(), "active"_n });
    issue.send( get_self(), value.quantity, memo );
}

void sx::vaults::retire( const extended_asset value, const string memo )
{
    eosio::token::retire_action retire( value.contract, { get_self(), "active"_n });
    retire.send( value.quantity, memo );
}

void sx::vaults::transfer( const name from, const name to, const extended_asset value, const string memo )
{
    eosio::token::transfer_action transfer( value.contract, { from, "active"_n });
    transfer.send( from, to, value.quantity, memo );
}
