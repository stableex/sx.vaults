# `vaults.sx`

> SX Vaults stores underlying assets into interest yielding strategies.

### Deposit

Users can send `EOS` tokens to `vaults.sx` to receive `SXEOS` tokens.

### Redeem

Users can send `SXEOS` tokens to `vaults.sx` to receive back their `EOS` + any interest accumulated during the time period holding the `SXEOS` asset.

### Price

Each vault is initially priced at 1:10,000 ratio, as interest are accrued in the vaults, the price ratio decreases, thus increasing the price.

```
deposit / supply
```

## Quickstart

```bash
# `deposit` EOS => SXEOS @ 1:1 ratio
$ cleos transfer myaccount vaults.sx "1.0000 EOS" ""

# `redeem` SXEOS => EOS @ 1:1 ratio + accrued fees
$ cleos transfer myaccount vaults.sx "10000.0000 SXEOS" "" --contract token.sx
```

## Table of Content

- [TABLE `vault`](#table-vault)
- [ACTION `setvault`](#table-setvault)
- [ACTION `update`](#table-update)

## TABLE `vault`

- `{asset} deposit` - vault deposit amount
- `{asset} staked` - vault staked amount
- `{asset} supply` - vault active supply
- `{name} account` - account to/from deposit balance
- `{time_point_sec} last_updated` - last updated timestamp

### example

```json
{
    "deposit": {"quantity": "100.0000 EOS", "contract": "eosio.token"},
    "staked": {"quantity": "80.0000 EOS", "contract": "eosio.token"},
    "supply": {"quantity": "1000000.0000 SXEOS", "contract": "token.sx"},
    "account": "vaults.sx",
    "last_updated": "2020-11-23T00:00:00"
}
```

## ACTION `setvault`

Set initial vault deposit balance & supply

- **authority**: `get_self()`

### params

- `{extended_symbol} deposit` - deposit symbol
- `{symbol_code} supply_id` - liquidity supply symbol
- `{name} account` - account to/from deposit balance

### Example

```bash
$ cleos push action vaults.sx setvault '[["4,EOS", "eosio.token"], "SXEOS", "vaults.sx"]' -p vaults.sx
```

## ACTION `update`

Update vault deposit balance & supply

- **authority**: `get_self()` or vault `account`

### params

- `{symbol_code} id` - deposit symbol

### Example

```bash
$ cleos push action vaults.sx update '["EOS"]' -p vaults.sx
```