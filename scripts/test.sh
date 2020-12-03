#!/bin/bash

# create vault
cleos push action eosio.token open '["flash.sx", "4,EOS", "flash.sx"]' -p flash.sx
cleos push action vaults.sx setvault '[["4,EOS", "eosio.token"], "SXEOS", "flash.sx"]' -p vaults.sx

# deposit
cleos -v transfer account.sx vaults.sx "2.0000 EOS"

# withdraw
cleos -v transfer account.sx vaults.sx "10000.0000 SXEOS" --contract token.sx

# update
cleos -v push action vaults.sx update '["EOS"]' -p vaults.sx

# # get tables
# cleos get table vaults.sx vaults.sx vault
# cleos get currency stats token.sx SXEOS
