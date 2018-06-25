# Aegis

Patroneos smart contract


## Install
```
$ git clone git@github.com:EOSEssentials/Aegis.git
$ cd Aegis
$ eosiocpp -o aegis.wast aegis.cpp
$ eosiocpp -g aegis.abi aegis.cpp
```

Replace ${your_aegis_account} for your account where stored contract.
Replace ${your_receiver_account} for your account where claim the tokens.

```
$ cleos set contract ${your_aegis_account} ../aegis
$ cleos set account permission ${your_aegis_account} active '{"threshold": 1,"keys": [{"key": "${your_aegis_key}","weight": 1}],"accounts": [{"permission":{"actor":"${your_aegis_account}","permission":"eosio.code"},"weight":1}]}' owner
$ cleos push action aegis setreceiver '["${your_receiver_account}"]' -p ${your_aegis_account}
```

### Useful commands
```
cleos get table aegis aegis patrons
cleos get table aegis aegis history


cleos push action eosio.token transfer '[ "${your_sender_account}", "{your_aegis_account}", "2.0000 EOS", "m" ]' -p ${your_sender_account}
cleos push action ${your_aegis_account} refund '["${your_sender_account}"]' -p eosio
cleos push action ${your_aegis_account} claim '' -p ${your_receiver_account}

```