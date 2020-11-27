#!/bin/bash

# create pairs
cleos push action eosio.token open '["vault.sx", "4,EOS", "vault.sx"]' -p vault.sx
cleos push action vault.sx setvault '["SXEOS", ["4,EOS", "eosio.token"]]' -p vault.sx

cleos push action eosio.token open '["vault.sx", "6,USDT", "vault.sx"]' -p vault.sx
cleos push action vault.sx setvault '["SXUSDT", ["6,USDT", "eosio.token"]]' -p vault.sx

# deposit
cleos -v transfer my.sx vault.sx "2.0000 EOS"

# withdraw
cleos -v transfer my.sx vault.sx "10000.0000 SXEOS" --contract token.sx

# # get tables
# cleos get table vault.sx vault.sx pairs
# cleos get currency stats token.sx SXEOS
