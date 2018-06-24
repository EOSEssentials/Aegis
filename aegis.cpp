#include <eosiolib/eosio.hpp>
#include <eosiolib/currency.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/time.hpp>

using eosio::block_timestamp;
using eosio::currency;
using eosio::asset;
using eosio::action;
using eosio::unpack_action_data;
using eosio::permission_level;

class aegis : public eosio::contract {

public:
    aegis(action_name self) : contract(self), _patrons(_self, _self), _history(_self, _self), _global(_self, _self) {}

    //@abi action
    void claim() {
        eosio_assert(is_account(_global.get().receiver), "Not receiver set");
        require_auth(_global.get().receiver);
        // TODO get 10% of money + update tables (current money / all time money)
    }

    //@abi action
    void refund(const account_name patron) {
        require_auth(patron);

        auto itr = _patrons.find(patron);
        if( itr == _patrons.end() ) {
            eosio_assert( false, "Patron does not exist" );
        }

        action(permission_level{ _self, N(active) }, N(eosio.token), N(transfer), std::make_tuple(_self, itr->name, itr->balance, std::string("Refund Aegis"))
        ).send();

        _patrons.erase(itr);
    }

    //@abi action
    void setReceiver(const account_name receiver)
    {
        require_auth(_self);
        // TODO add receiver
    }

    void apply(const account_name contract, const account_name action) {

        if (action == N(transfer)) {
            transferReceived(unpack_action_data<currency::transfer>(), contract);
            return;
        }

        auto &thiscontract = *this;
        switch (action) {
            EOSIO_API(aegis, (refund)(claim)(setReceiver))
        };
    }

private:
    void transferReceived(const currency::transfer &transfer, const account_name code) {
        eosio_assert( transfer.quantity.symbol == CORE_SYMBOL, "only core token allowed" );
        eosio_assert( transfer.quantity.is_valid(), "invalid quantity" );
        eosio_assert( transfer.quantity.amount > 0.1000, "must be higher than 0.1000 EOS" );

        if (transfer.to != _self) {
            return;
        }

        auto itr = _patrons.find(transfer.from);
        if( itr == _patrons.end() ) {
            itr = _patrons.emplace(_self, [&](auto& acnt){
                acnt.name = transfer.from;
            });
        }

        _patrons.modify( itr, 0, [&]( auto& acnt ) {
            acnt.balance += transfer.quantity;
        });
    }

    //@abi table patrons i64
    //@abi table patrons_all_times i64
    struct patron {
        account_name name;
        asset balance;

        uint64_t primary_key()const { return name; }

        EOSLIB_SERIALIZE(patron, (name)(balance))
    };

    struct aegis_state {
        account_name receiver;
        block_timestamp last_claim;
        EOSLIB_SERIALIZE(aegis_state, (receiver)(last_claim))
    };

    typedef eosio::singleton<N(global), aegis_state> global_state_singleton;
    typedef eosio::multi_index<N(patrons), patron> patrons_index;
    typedef eosio::multi_index<N(history), patron> patrons_history_index;

    global_state_singleton _global;
    patrons_index _patrons;
    patrons_history_index _history;
};

extern "C" {
void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    auto self = receiver;
    aegis contract(self);
    contract.apply(code, action);
    eosio_exit(0);
}
}

// EOSIO_ABI(aegis, (refund)(claim)(setReceiver))