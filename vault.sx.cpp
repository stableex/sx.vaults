#include <eosio.token/eosio.token.hpp>

#include "vault.sx.hpp"

/**
 * Notify contract when any token transfer notifiers relay contract
 */
[[eosio::on_notify("*::transfer")]]
void sx::vault::on_transfer( const name from, const name to, const asset quantity, const string memo )
{
    // authenticate incoming `from` account
    require_auth( from );

    // ignore transfers
    if ( to != get_self() ) return;

    // check incoming transfer
    const name contract = get_first_receiver();

    // find pairs
    sx::vault::pairs_table _pairs( get_self(), get_self().value );
    auto bydeposit = _pairs.get_index<"bydeposit"_n>();
    auto deposit_itr = bydeposit.find( quantity.symbol.code().raw() );
    auto pairs_itr = _pairs.find( quantity.symbol.code().raw() );

    // handle issuance (ex: EOS => SXEOS)
    if ( deposit_itr != bydeposit.end() ) {
        check( contract == deposit_itr->deposit.get_contract(), "deposit contract does not match" );
        const extended_asset out = calculate_issue( deposit_itr->id, quantity );

        // issue & transfer to sender
        issue( out, "issue" );
        transfer( get_self(), from, out, get_self().to_string() );

        // update internal balance & supply
        bydeposit.modify( deposit_itr, get_self(), [&]( auto& row ) {
            row.balance += quantity;
            row.supply += out.quantity;
            row.last_updated = current_time_point();
        });

    // handle retire (ex: SXEOS => EOS)
    } else if ( pairs_itr != _pairs.end() ) {
        check( contract == pairs_itr->id.get_contract(), "pair contract does not match" );
        const extended_asset out = calculate_retire( pairs_itr->id, quantity );

        // retire & transfer to sender
        retire( { quantity, contract }, "retire" );
        transfer( get_self(), from, out, get_self().to_string() );

        // update internal balance & supply
        _pairs.modify( pairs_itr, get_self(), [&]( auto& row ) {
            row.balance -= out.quantity;
            row.supply -= quantity;
            row.last_updated = current_time_point();
        });
    } else {
        check( false, "incoming transfer asset symbol not supported");
    }
}

[[eosio::action]]
void sx::vault::setpair( const symbol_code id, const extended_symbol deposit )
{
    require_auth( get_self() );
    sx::vault::pairs_table _pairs( get_self(), get_self().value );

    // ID must use same symbol precision as deposit
    const extended_symbol ext_id = {{ id, deposit.get_symbol().precision() }, TOKEN_CONTRACT };

    // deposit token must exists
    const asset supply = eosio::token::get_supply( deposit.get_contract(), deposit.get_symbol().code() );
    check( supply.amount > 0, "deposit has no supply");
    check( deposit.get_symbol() == supply.symbol, "symbol precision mismatch");

    // must have open balance
    // initializing pair must not contain any deposit balance
    const asset balance = eosio::token::get_balance( deposit.get_contract(), get_self(), deposit.get_symbol().code() );
    check( balance.amount <= 0, "must have zero balance");
    check( _pairs.find( id.raw() ) == _pairs.end(), "pair already created");

    // create pair
    create( ext_id );
    _pairs.emplace( get_self(), [&]( auto& row ) {
        row.id = ext_id;
        row.deposit = deposit;
        row.balance = { 0, deposit.get_symbol() };
    });
}

extended_asset sx::vault::calculate_issue( const extended_symbol id, const asset payment )
{
    sx::vault::pairs_table _pairs( get_self(), get_self().value );
    const auto pairs = _pairs.get( id.get_symbol().code().raw(), "id does not exists in pairs" );
    const asset supply = eosio::token::get_supply( id.get_contract(), id.get_symbol().code() );
    const int64_t ratio = 10000;

    // initialize vault supply
    if ( supply.amount == 0 ) return { payment.amount * ratio, id };

    // issue & redeem supply calculation
    // calculations based on fill REX order
    // https://github.com/EOSIO/eosio.contracts/blob/f6578c45c83ec60826e6a1eeb9ee71de85abe976/contracts/eosio.system/src/rex.cpp#L775-L779
    const int64_t S0 = pairs.balance.amount; // vault
    const int64_t S1 = S0 + payment.amount; // payment
    const int64_t R0 = supply.amount; // supply
    const int64_t R1 = (uint128_t(S1) * R0) / S0;

    return { R1 - R0, id };
}

extended_asset sx::vault::calculate_retire( const extended_symbol id, const asset payment )
{
    sx::vault::pairs_table _pairs( get_self(), get_self().value );
    const auto pairs = _pairs.get( id.get_symbol().code().raw(), "id does not exists in pairs" );
    const asset supply = eosio::token::get_supply( id.get_contract(), id.get_symbol().code() );

    // issue & redeem supply calculation
    // calculations based on add to REX pool
    // https://github.com/EOSIO/eosio.contracts/blob/f6578c45c83ec60826e6a1eeb9ee71de85abe976/contracts/eosio.system/src/rex.cpp#L772
    const int64_t S0 = pairs.balance.amount;
    const int64_t R0 = supply.amount;
    const int64_t p  = (uint128_t(payment.amount) * S0) / R0;

    return { p, pairs.deposit };
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
