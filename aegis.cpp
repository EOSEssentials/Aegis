#include <eosiolib/eosio.hpp>
#include <eosiolib/currency.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/time.hpp>

using eosio::currency;
using eosio::asset;
using eosio::action;
using eosio::unpack_action_data;
using eosio::permission_level;
using eosio::time_point_sec;
using eosio::multi_index;
using eosio::singleton;

class aegis : public eosio::contract {

public:
    aegis(action_name self) : contract(self), _patrons(_self, _self), _history(_self, _self), _global(_self, _self) {}
    const int64_t WEEK = 60*60*7;
    const int64_t MIN_PAYMENT = 1000;
    const int64_t PERCENTAGE_PER_WEEK = 10;

    //@abi action
    void claim() {
        eosio_assert(is_account(_global.get().receiver), "Not receiver set");
        require_auth(_global.get().receiver);
        eosio_assert((time_point_sec(now()) > (_global.get().last_claim + WEEK)), "You must wait 7 days to claim again");
        asset total_amount = asset();

        for( const auto& itr : _patrons ) {
            asset patron_amount = asset();

            if (itr.balance.amount <= MIN_PAYMENT) {
                patron_amount = itr.balance;
                _patrons.erase(itr);
            } else {
                _patrons.modify( itr, 0, [&]( auto& acnt ) {
                    patron_amount = asset(acnt.balance.amount * PERCENTAGE_PER_WEEK/100);
                    acnt.balance -= patron_amount;
                });
            }

            auto itr_history = _history.find(itr.name);
            if( itr_history == _history.end() ) {
                itr_history = _history.emplace(_self, [&](auto& acnt){
                    acnt.name = itr.name;
                    acnt.balance = patron_amount;
                });
            }

            _history.modify( itr_history, 0, [&]( auto& acnt ) {
                acnt.balance += patron_amount;
            });
        }

        action(permission_level{ _self, N(active) }, N(eosio.token), N(transfer), std::make_tuple(_self, _global.get().receiver, total_amount, std::string("Refund Aegis"))
        ).send();

        _global.set(aegis_state{_global.get().receiver, time_point_sec(now())}, 0);
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
    void setreceiver(const account_name receiver)
    {
        require_auth(_self);
        _global.set(aegis_state{receiver, _global.get().last_claim}, 0);
    }

    void apply(const account_name contract, const account_name action) {

        if (action == N(transfer)) {
            transfer_received(unpack_action_data<currency::transfer>(), contract);
            return;
        }

        auto &thiscontract = *this;
        switch (action) {
            EOSIO_API(aegis, (refund)(claim)(setreceiver))
        };
    }

private:
    void create_or_edit() {

    }

    void transfer_received(const currency::transfer &transfer, const account_name code) {
        eosio_assert( transfer.quantity.symbol == CORE_SYMBOL, "only core token allowed" );
        eosio_assert( transfer.quantity.is_valid(), "invalid quantity" );
        eosio_assert( transfer.quantity.amount > MIN_PAYMENT, "must be higher than 0.1000 EOS" );

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
    //@abi table history i64
    struct patron {
        account_name name;
        asset balance;

        uint64_t primary_key()const { return name; }

        EOSLIB_SERIALIZE(patron, (name)(balance))
    };

    struct aegis_state {
        account_name receiver;
        time_point_sec last_claim = time_point_sec(0);
        EOSLIB_SERIALIZE(aegis_state, (receiver)(last_claim))
    };

    typedef singleton<N(global), aegis_state> global_state_singleton;
    typedef multi_index<N(patrons), patron> patrons_index;
    typedef multi_index<N(history), patron> patrons_history_index;

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

// EOSIO_ABI(aegis, (refund)(claim)(setreceiver))