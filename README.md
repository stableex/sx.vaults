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

## TABLE `vault`

- `{asset} supply` - vault active supply
- `{asset} balance` - vault deposit balance
- `{time_point_sec} last_updated` - last updated timestamp

### example

```json
{
    "supply": {"quantity": "1000000.0000 SXEOS", "contract": "token.sx"},
    "balance": {"quantity": "100.0000 EOS", "contract": "eosio.token"},
    "last_updated": "2020-11-23T00:00:00"
}
```