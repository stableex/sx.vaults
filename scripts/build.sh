#!/bin/bash

eosio-cpp vaults.sx.cpp -I include
cleos set contract vaults.sx . vaults.sx.wasm vaults.sx.abi
