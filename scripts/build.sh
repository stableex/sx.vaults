#!/bin/bash

eosio-cpp vault.sx.cpp -I include
cleos set contract vault.sx . vault.sx.wasm vault.sx.abi
