#!/bin/bash

# create vault
cleos push action eosio.token open '["vault.sx", "4,EOS", "vault.sx"]' -p vault.sx
cleos push action vault.sx setvault '[["4,EOS", "eosio.token"], "SXEOS"]' -p vault.sx

# deposit
cleos -v transfer myaccount vault.sx "2.0000 EOS"

# withdraw
cleos -v transfer myaccount vault.sx "10000.0000 SXEOS" --contract token.sx

# get tables
cleos get table vault.sx vault.sx vault
cleos get currency stats token.sx SXEOS
