/*************************************************************************
      > File Name: test.cc
      > Author: Xu Qingqing
      > Mail: xuqnqn@qq.com
      > Created Time: 2019年03月02日 星期六 00时22分58秒
 ************************************************************************/

#include<iostream>
#include"MemCachedClient.h"
 
using std::cout;
using std::endl;
 
int main()
{
    MemCachedClient mc;
    int result = mc.Insert("mem_key","/view/index?name=Jame");   
	string get_value =  mc.Get("mem_key");
	
	cout << "get_value: " << get_value << endl;
    return 1;
};


/*
$./memcached -m 10 -l 127.0.0.1 -p 11211
$./test 
get_value: /view/index?name=Jame

 *
 */
