#!/bin/bash

# create vault
cleos push action eosio.token open '["flash.sx", "4,EOS", "flash.sx"]' -p flash.sx
cleos push action vault.sx setvault '[["4,EOS", "eosio.token"], "SXEOS", "flash.sx"]' -p vault.sx

# deposit
cleos -v transfer account.sx vault.sx "2.0000 EOS"

# withdraw
cleos -v transfer account.sx vault.sx "10000.0000 SXEOS" --contract token.sx

# update
cleos -v push action vault.sx update '["EOS"]' -p vault.sx

# # get tables
# cleos get table vault.sx vault.sx vault
# cleos get currency stats token.sx SXEOS
