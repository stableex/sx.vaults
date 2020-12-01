#!/bin/bash

# unlock wallet
cleos wallet unlock --password $(cat ~/eosio-wallet/.pass)

# create accounts
cleos create account eosio vault.sx EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio flash.sx EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio token.sx EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio eosio.token EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio account.sx EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV

# deploy
cleos set contract vault.sx . vault.sx.wasm vault.sx.abi
cleos set contract token.sx ../eosio.token eosio.token.wasm eosio.token.abi
cleos set contract eosio.token ../eosio.token eosio.token.wasm eosio.token.abi

# permission
cleos set account permission vault.sx active --add-code
cleos set account permission token.sx active vault.sx --add-code
cleos set account permission flash.sx active vault.sx --add-code

# create EOS token
cleos push action eosio.token create '["eosio", "100000000.0000 EOS"]' -p eosio.token
cleos push action eosio.token issue '["eosio", "5000000.0000 EOS", "init"]' -p eosio
cleos transfer eosio account.sx "50000.0000 EOS" "init"
