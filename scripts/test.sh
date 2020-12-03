#!/bin/bash

# create vault
cleos push action eosio.token open '["vaults.sx", "4,EOS", "vaults.sx"]' -p vaults.sx
cleos push action vaults.sx setvault '[["4,EOS", "eosio.token"], "SXEOS", "vaults.sx"]' -p vaults.sx

# deposit
cleos -v transfer account.sx vaults.sx "2.0000 EOS"

# withdraw
cleos -v transfer account.sx vaults.sx "10000.0000 SXEOS" --contract token.sx

# update
cleos -v push action vaults.sx update '["EOS"]' -p vaults.sx

# # get tables
# cleos get table vaults.sx vaults.sx vault
# cleos get currency stats token.sx SXEOS
