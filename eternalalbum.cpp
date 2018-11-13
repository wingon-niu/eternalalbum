#include <utility>
#include <vector>
#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/print.hpp>

using eosio::key256;
using eosio::indexed_by;
using eosio::const_mem_fun;
using eosio::asset;
using eosio::permission_level;
using eosio::action;
using eosio::print;
using eosio::name;
using std::string;

#define MAIN_SYMBOL S(4, SYS)

// 永恒相册

class eternalalbum : public eosio::contract {
public:
    eternalalbum(account_name self)
        : contract(self),
          _accounts(_self, _self){};

    void transfer(const account_name& from, const account_name& to, const asset& quantity, const string& memo);

    // @abi action
    void withdraw( const account_name to, const asset& quantity );

    // 测试时使用，上线时去掉
    // @abi action
    void clearalldata();

    //

private:
    // @abi table accounts i64
    struct st_account {
       account_name owner;
       asset        quantity;

       uint64_t primary_key() const { return owner; }

       EOSLIB_SERIALIZE( st_account, (owner)(quantity) )
    };
    typedef eosio::multi_index<N(accounts), st_account> tb_accounts;

    //

    tb_accounts     _accounts;
};

extern "C" {
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        eternalalbum thiscontract(receiver);

        if ((code == N(eosio.token)) && (action == N(transfer))) {
            execute_action(&thiscontract, &eternalalbum::transfer);
            return;
        }
    
        if (code != receiver) return;
    
        switch (action) { EOSIO_API(eternalalbum, (withdraw)(clearalldata)) };
    }
}

void eternalalbum::transfer(const account_name& from,
                            const account_name& to,
                            const asset& quantity,
                            const string& memo) {

    if (from == _self || to != _self) {
        return;
    }
    if ("eternalalbum deposit" != memo) {
        return;
    }
    if (!quantity.is_valid()) {
        return;
    }
    if (quantity.symbol != MAIN_SYMBOL) {
        return;
    }
    if (quantity.amount <= 0) {
        return;
    }

    auto itr = _accounts.find(from);
    if( itr == _accounts.end() ) {
       itr = _accounts.emplace(_self, [&](auto& acnt){
          acnt.owner = from;
       });
    }
    
    _accounts.modify( itr, 0, [&]( auto& acnt ) {
       acnt.quantity += quantity;
    });
}

void eternalalbum::withdraw( const account_name to, const asset& quantity ) {
    require_auth( to );

    if (!quantity.is_valid()) {
        return;
    }
    if (quantity.symbol != MAIN_SYMBOL) {
        return;
    }
    if (quantity.amount <= 0) {
        return;
    }

    auto itr = _accounts.find( to );
    eosio_assert(itr != _accounts.end(), "unknown account");

    _accounts.modify( itr, 0, [&]( auto& acnt ) {
        eosio_assert( acnt.quantity >= quantity, "insufficient balance" );
        acnt.quantity -= quantity;
    });

    action(
        permission_level{ _self, N(active) },
        N(eosio.token), N(transfer),
        std::make_tuple(_self, to, quantity, std::string("eternalalbum withdraw"))
    ).send();
}

// 测试时使用，上线时去掉
void eternalalbum::clearalldata()
{
    require_auth( _self );
    std::vector<uint64_t> keysForDeletion;

    for (auto& item : _accounts) {
        keysForDeletion.push_back(item.owner);
    }
    for (uint64_t key : keysForDeletion) {
        auto itr = _accounts.find(key);
        if (itr != _accounts.end()) {
            _accounts.erase(itr);
        }
    }
    keysForDeletion.clear();

    //
}
