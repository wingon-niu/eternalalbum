#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>

using namespace eosio;

class hello : public eosio::contract {
  public:
      using contract::contract;

      /// @abi action
      void hi( account_name user ) {
         require_auth( user );
         eosio::print( "\nHello: ", eosio::name{user}.to_string().c_str(), "\n" );

         auto count = 5;
         auto some_name = "This is my dog!";
         eosio::print_f("Count=%, Name=%\n", count, some_name);

         auto header = "---------- debug info begin ----------";
         eosio::print( "\n", header, "\n" );

         eosio::print( "            user   =  ",             user,  "\n" );
         eosio::print( "eosio::name{user}  =  ", eosio::name{user}, "\n" );

         std::string strTest = "ab1234512345";
         eosio::print( "std::string strTest  =  ", strTest, "\n" );

         eosio::print( "            eosio::string_to_name(strTest.c_str())   =  ",             eosio::string_to_name(strTest.c_str()),  "\n" );
         eosio::print( "eosio::name{eosio::string_to_name(strTest.c_str())}  =  ", eosio::name{eosio::string_to_name(strTest.c_str())}, "\n" );

         eosio::print( "            N(bb1234512345)   =  ",             N(bb1234512345),  "\n" );
         eosio::print( "eosio::name{N(bb1234512345)}  =  ", eosio::name{N(bb1234512345)}, "\n" );

         auto footer = "---------- debug info end ----------";
         eosio::print( footer, "\n" );
      }
};

EOSIO_ABI( hello, (hi) )

