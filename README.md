# `vault.sx`

> SX Vault

## Quickstart

```bash
# `issue` EOS => SXEOS @ 1:1 ratio
$ cleos transfer myaccount vault.sx "1.0000 EOS" ""

# `retire` SXEOS => EOS @ 1:1 ratio + accrued fees
$ cleos transfer myaccount vault.sx "10000.0000 SXEOS" "" --contract token.sx
```

## Table of Content

- [TABLE `vault`](#table-vault)
- [ACTION `setvault`](#table-setvault)

## TABLE `vault`

- `{asset} balance` - vault deposit balance
- `{asset} supply` - vault active supply
- `{time_point_sec} last_updated` - last updated timestamp

### example

```json
{
    "balance": {"quantity": "100.0000 EOS", "contract": "eosio.token"},
    "supply": {"quantity": "1000000.0000 SXEOS", "contract": "token.sx"},
    "last_updated": "2020-11-23T00:00:00"
}
```

## ACTION `setvault`

Set initial vault balance & supply

- **authority**: `get_self()`

### params

- `{extended_symbol} deposit` - deposit symbol
- `{symbol_code} supply` - liquidity supply symbol

### Example

```bash
$ cleos push action vault.sx setvault '[["4,EOS", "eosio.token"], "SXEOS"]' -p vault.sx
```