#!/bin/bash

eosio-cpp vault.sx.cpp -I ../
cleos set contract vault.sx . vault.sx.wasm vault.sx.abi
