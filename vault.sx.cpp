#include <eosio.token/eosio.token.hpp>

#include "vault.sx.hpp"

[[eosio::action]]
void sx::vault::update( const symbol_code id )
{
    sx::vault::vault_table _vault( get_self(), get_self().value );

    auto& vault = _vault.get( id.raw(), "vault does not exist" );
    // only vault account is allowed to update the account balance externally
    // internal vault.sx contract can only update balance/supply from `on_transfer`
    require_auth( vault.account );

    // helpers
    const name contract = vault.deposit.contract;
    const symbol sym = vault.deposit.quantity.symbol;

    // get internal balance from account
    const asset balance = eosio::token::get_balance( contract, vault.account, sym.code() );

    // ADD EXCEPTION FOR REX & STAKED EOS

    // update balance
    _vault.modify( vault, get_self(), [&]( auto& row ) {
        row.deposit.quantity = balance;
        row.last_updated = current_time_point();
    });
}

/**
 * Notify contract when any token transfer notifiers relay contract
 */
[[eosio::on_notify("*::transfer")]]
void sx::vault::on_transfer( const name from, const name to, const asset quantity, const string memo )
{
    // authenticate incoming `from` account
    require_auth( from );

    // ignore outgoing transfers
    if ( to != get_self() ) return;

    // incoming token contract
    const name contract = get_first_receiver();

    check(from.suffix() == "sx"_n, "contract is under maintenance");

    // table & index
    sx::vault::vault_table _vault( get_self(), get_self().value );
    auto _vault_by_supply = _vault.get_index<"bysupply"_n>();

    // iterators
    auto deposit_itr = _vault.find( quantity.symbol.code().raw() );
    auto supply_itr = _vault_by_supply.find( quantity.symbol.code().raw() );

    // handle issuance (ex: EOS => SXEOS)
    if ( deposit_itr != _vault.end() ) {
        // calculate issuance supply token by providing balance
        check( contract == deposit_itr->deposit.contract, "deposit token contract does not match" );
        const symbol_code id = deposit_itr->deposit.quantity.symbol.code();
        const extended_asset out = calculate_issue( id, quantity );

        // issue & transfer to sender
        issue( out, "issue" );
        transfer( get_self(), from, out, get_self().to_string() );

        // update internal balance & supply
        _vault.modify( deposit_itr, get_self(), [&]( auto& row ) {
            row.deposit.quantity += quantity;
            row.supply += out;
            row.last_updated = current_time_point();
        });

    // handle retire (ex: SXEOS => EOS)
    } else if ( supply_itr != _vault_by_supply.end() ) {
        // calculate redeem amount from retiring supply token
        check( contract == supply_itr->supply.contract, "supply token contract does not match" );
        const symbol_code id = supply_itr->deposit.quantity.symbol.code();
        const extended_asset out = calculate_retire( id, quantity );

        // retire & transfer to sender
        retire( { quantity, contract }, "retire" );
        transfer( get_self(), from, out, get_self().to_string() );

        // update internal deposit & supply
        _vault_by_supply.modify( supply_itr, get_self(), [&]( auto& row ) {
            row.deposit -= out;
            row.supply.quantity -= quantity;
            row.last_updated = current_time_point();
        });
    } else {
        check( false, "incoming transfer asset symbol not supported");
    }
}

[[eosio::action]]
void sx::vault::setvault( const extended_symbol deposit, const symbol_code supply_id, const name account )
{
    require_auth( get_self() );
    sx::vault::vault_table _vault( get_self(), get_self().value );

    // ID must use same symbol precision as deposit
    const extended_symbol ext_supply = {{ supply_id, deposit.get_symbol().precision() }, TOKEN_CONTRACT };

    // deposit token must exists
    const symbol_code id = deposit.get_symbol().code();
    const asset supply = eosio::token::get_supply( deposit.get_contract(), id );
    check( supply.amount > 0, "deposit has no supply");
    check( deposit.get_symbol() == supply.symbol, "deposit symbol precision mismatch");

    // account must have open balance
    // initializing vault must not contain any deposit balance
    check( is_account(account), "account does not exists");
    const asset balance = eosio::token::get_balance( deposit.get_contract(), account, id );
    check( balance.amount <= 0, account.to_string() + " must have zero balance");

    // create vault
    check( _vault.find( id.raw() ) == _vault.end(), "vault already created");
    create( ext_supply );
    _vault.emplace( get_self(), [&]( auto& row ) {
        row.deposit = { 0, deposit };
        row.staked = { 0, deposit };
        row.supply = { 0, ext_supply };
        row.account = account;
        row.last_updated = current_time_point();
    });
}

extended_asset sx::vault::calculate_issue( const symbol_code id, const asset payment )
{
    sx::vault::vault_table _vault( get_self(), get_self().value );
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

extended_asset sx::vault::calculate_retire( const symbol_code id, const asset payment )
{
    sx::vault::vault_table _vault( get_self(), get_self().value );
    const auto vault = _vault.get( id.raw(), "id does not exists in vault" );

    // issue & redeem supply calculation
    // calculations based on add to REX pool
    // https://github.com/EOSIO/eosio.contracts/blob/f6578c45c83ec60826e6a1eeb9ee71de85abe976/contracts/eosio.system/src/rex.cpp#L772
    const int64_t S0 = vault.deposit.quantity.amount;
    const int64_t R0 = vault.supply.quantity.amount;
    const int64_t p  = (uint128_t(payment.amount) * S0) / R0;

    return { p, vault.deposit.get_extended_symbol() };
}

void sx::vault::create( const extended_symbol value )
{
    eosio::token::create_action create( value.get_contract(), { value.get_contract(), "active"_n });
    create.send( get_self(), asset{ asset_max, value.get_symbol() } );
}

void sx::vault::issue( const extended_asset value, const string memo )
{
    eosio::token::issue_action issue( value.contract, { get_self(), "active"_n });
    issue.send( get_self(), value.quantity, memo );
}

void sx::vault::retire( const extended_asset value, const string memo )
{
    eosio::token::retire_action retire( value.contract, { get_self(), "active"_n });
    retire.send( value.quantity, memo );
}

void sx::vault::transfer( const name from, const name to, const extended_asset value, const string memo )
{
    eosio::token::transfer_action transfer( value.contract, { from, "active"_n });
    transfer.send( from, to, value.quantity, memo );
}
