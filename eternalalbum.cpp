#include "eternalalbum.hpp"

extern "C" {
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        eternalalbum thiscontract(receiver);

        if ((code == N(eosio.token)) && (action == N(transfer))) {
            execute_action(&thiscontract, &eternalalbum::transfer);
            return;
        }
    
        if (code != receiver) return;
    
        switch (action) { EOSIO_API(eternalalbum, (withdraw)(createalbum)(clearalldata)) };
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

// 创建相册
void eternalalbum::createalbum(const account_name& owner, const string& name)
{
    require_auth( owner );
    eosio_assert( name.length() <= NAME_MAX_LEN, "name is too long" );

    auto itr = _accounts.find( owner );
    eosio_assert(itr != _accounts.end(), "unknown account");

    _accounts.modify( itr, 0, [&]( auto& acnt ) {
        eosio_assert( acnt.quantity.amount >= PAY_FOR_ALBUM, "insufficient balance" );
        acnt.quantity.amount -= PAY_FOR_ALBUM;
    });

    _albums.emplace(_self, [&](auto& album){
        album.owner = owner;
        album.id    = _albums.available_primary_key();
        album.name  = name;
        album.pay   = PAY_FOR_ALBUM;
    });
}

// 上传图片
void eternalalbum::uploadpic(const account_name& owner, const uint64_t& album_id, const string& name,
                             const string& md5_sum, const string& ipfs_sum)
{
}

// 设置图片展示到公共区
void eternalalbum::displaypic(const account_name& owner, const uint64_t& id, const asset& fee)
{
}

// 为图片点赞
void eternalalbum::upvotepic(const uint64_t& id)
{
}

// 删除图片（只删除EOS中的数据，IPFS中的图片依然存在）
void eternalalbum::deletepic(const account_name& owner, const uint64_t& id)
{
}

// 删除相册
void eternalalbum::deletealbum(const account_name& owner, const uint64_t& id)
{
}

// 清除 multi_index 中的所有数据，测试时使用，上线时去掉
void eternalalbum::clearalldata()
{
    require_auth( _self );
    std::vector<uint64_t> keysForDeletion;
    print("\nclear all data.\n");

    keysForDeletion.clear();
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
    for (auto& item : _albums) {
        keysForDeletion.push_back(item.id);
    }
    for (uint64_t key : keysForDeletion) {
        auto itr = _albums.find(key);
        if (itr != _albums.end()) {
            _albums.erase(itr);
        }
    }

    keysForDeletion.clear();
    for (auto& item : _pics) {
        keysForDeletion.push_back(item.id);
    }
    for (uint64_t key : keysForDeletion) {
        auto itr = _pics.find(key);
        if (itr != _pics.end()) {
            _pics.erase(itr);
        }
    }
}
