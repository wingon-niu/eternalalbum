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

#define  MAIN_SYMBOL    S(4, SYS)

#define  PAY_FOR_ALBUM  5    // 每创建一个相册收费5eosc
#define  PAY_FOR_PIC    10   // 每张图片存储收费10eosc

// 永恒相册

class eternalalbum : public eosio::contract {
public:
    eternalalbum(account_name self) : contract(self),
          _accounts(_self, _self),
          _albums  (_self, _self),
          _pics    (_self, _self){};

    // 响应用户充值
    void transfer(const account_name& from, const account_name& to, const asset& quantity, const string& memo);

    // 用户提现
    // @abi action
    void withdraw( const account_name to, const asset& quantity );

    // 清除 multi_index 中的所有数据，测试时使用，上线时去掉
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

    // @abi table albums i64
    struct st_album {
        account_name owner;
        uint64_t     id;
        string       name;
        uint32_t     pay;

        uint64_t primary_key() const { return id; }
        uint64_t by_owner()    const { return owner; }

        EOSLIB_SERIALIZE( st_album, (owner)(id)(name)(pay) )
    };
    typedef eosio::multi_index<
        N(albums), st_album,
        indexed_by< N(byowner), const_mem_fun<st_album, uint64_t, &st_album::by_owner> >
    > tb_albums;

    // @abi table pics i64
    struct st_pic {
        account_name owner;
        uint64_t     album_id;
        uint64_t     id;
        string       name;
        string       md5_sum;
        string       ipfs_sum;
        uint32_t     pay;
        uint64_t     display_fee;
        uint32_t     upvote_num;

        uint64_t primary_key()    const { return id; }
        uint64_t by_owner()       const { return owner; }
        uint64_t by_album_id()    const { return album_id; }
        uint64_t by_display_fee() const { return ~display_fee; }

        EOSLIB_SERIALIZE( st_pic, (owner)(album_id)(id)(name)(md5_sum)(ipfs_sum)(pay)(display_fee)(upvote_num) )
    };
    typedef eosio::multi_index<
        N(pics), st_pic,
        indexed_by< N(byowner),      const_mem_fun<st_pic, uint64_t, &st_pic::by_owner> >,
        indexed_by< N(byalbumid),    const_mem_fun<st_pic, uint64_t, &st_pic::by_album_id> >,
        indexed_by< N(bydisplayfee), const_mem_fun<st_pic, uint64_t, &st_pic::by_display_fee> >
    > tb_pics;

    tb_accounts _accounts;
    tb_albums   _albums;
    tb_pics     _pics;
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

// 响应用户充值
void eternalalbum::transfer(const account_name& from,
                            const account_name& to,
                            const asset& quantity,
                            const string& memo) {

    if (from == _self) {
        return;
    }
    if (to != _self) {
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

// 用户提现
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

// 清除 multi_index 中的所有数据，测试时使用，上线时去掉
void eternalalbum::clearalldata()
{
    require_auth( _self );
    print("\nclear all data.\n");
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
