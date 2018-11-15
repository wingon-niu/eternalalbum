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

#define  PAY_FOR_ALBUM   5*10000   // 每创建一个相册收费  5 eosc
#define  PAY_FOR_PIC    10*10000   // 每张图片存储收费   10 eosc
#define  NAME_MAX_LEN   36         // 相册名称、图片名称的最大长度(单位：字节)

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

    // 创建相册
    // @abi action
    void createalbum(const account_name& owner, const string& name);

    // 上传图片
    // @abi action
    void uploadpic(const account_name& owner, const uint64_t& album_id, const string& name,
                   const string& md5_sum, const string& ipfs_sum);

    // 设置图片展示到公共区
    // @abi action
    void displaypic(const account_name& owner, const uint64_t& id, const asset& fee);

    // 为图片点赞
    // @abi action
    void upvotepic(const uint64_t& id);

    // 删除图片（只删除EOS中的数据，IPFS中的图片依然存在）
    // @abi action
    void deletepic(const account_name& owner, const uint64_t& id);

    // 删除相册
    // @abi action
    void deletealbum(const account_name& owner, const uint64_t& id);

    // 清除 multi_index 中的所有数据，测试时使用，上线时去掉
    // @abi action
    void clearalldata();

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
