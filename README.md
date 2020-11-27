# `vault.sx`

> SX Vault

## Quickstart

```bash
# `issue` EOS => SXEOS @ 1:1 ratio
$ cleos transfer myaccount vault.sx "1.0000 EOS" ""

# `redeem` SXEOS => EOS @ 1:1 ratio + accrued fees
$ cleos transfer myaccount vault.sx "1.0000 SXEOS" "" --contract token.sx
```

## Table of Content

- [TABLE `pairs`](#table-pairs)

## TABLE `pairs`

- `{extended_symbol} id` - vault symbol ID
- `{extended_symbol} deposit` - accepted deposit symbol
- `{asset} balance` - vault balance
- `{asset} supply` - vault supply
- `{time_point_sec} last_updated` - last updated timestamp

### example

```json
{
    "id": {"symbol": "4,SXEOS", "contract": "token.sx"},
    "deposit": {"symbol": "4,EOS", "contract": "eosio.token"},
    "balance": "100.0000 EOS",
    "supply": "1000000.0000 SXEOS",
    "last_updated": "2020-11-23T00:00:00"
}
```