/*************************************************************************
      > File Name: memcached_test.c
      > Author: Xu Qingqing
      > Mail: xuqnqn@qq.com
      > Created Time: 2019年03月01日 星期五 23时05分54秒
 ************************************************************************/

#include <stdio.h>
#include <libmemcached/memcached.h>
#include <libmemcached/util.h>
#include <assert.h>

#if 0
//g++ -std=c++11 -m32 -g memcached_test.c -o memcached_test -I/usr/local/lib/libmemcached/include -L/usr/local/lib/libmemcached/lib  -lmemcached  -lmemcachedutil
//g++ -std=c++11 -m32 -g memcached_test.c -o memcached_test -I/usr/local/lib/libmemcached/include /usr/local/lib/libmemcached/lib/libmemcached.a  /usr/local/lib/libmemcached/lib/libmemcachedutil.a
//不支持 gcc 静态链接方式,因为 pool 实现是 C++ 实现
//gcc -m32 -g memcached_test.c -o memcached_test -I/usr/local/lib/libmemcached/include /usr/local/lib/libmemcached/lib/libmemcached.a  /usr/local/lib/libmemcached/lib/libmemcachedutil.a
#endif



//!!!! xqq 经过实验: !!!!!
// 1. 编译
//gcc -std=gnu99 -g memcached_test.c -o memcached_test -I/usr/local/include/libmemcached/ -L/usr/local/lib/ -lmemcached -lmemcachedutil
//
// 2. 启动三个memcached服务，模拟集群
//./memcached -m 10 -l 127.0.0.1 -p 12111
//./memcached -m 10 -l 127.0.0.1 -p 12112
//./memcached -m 10 -l 127.0.0.1 -p 12113

//http://libmemcached.org/libMemcached.html

//从连接池内取出一个连接集群对象,支持错误码和超时功能
memcached_st * fetch_memcached_wait(memcached_pool_st * pool, struct timespec* wait, memcached_return_t *rc)
{
    assert(pool || wait || rc);

    return memcached_pool_fetch(pool, wait, rc);
}

//从连接池内取出一个连接集群对象,无空闲对象直接返回
memcached_st * fetch_memcached(memcached_pool_st * pool)
{
    assert(pool);

    static struct timespec wait = { 0, 0 };
    static memcached_return_t rc;
    return fetch_memcached_wait(pool, &wait, &rc);
}

//释放一个从连接池内取出的一个连接集群对象
void release_memcached(memcached_pool_st * pool, memcached_st *memc)
{
    assert(pool || memc);

    memcached_return_t rc = memcached_pool_push(pool, memc);
    if (MEMCACHED_SUCCESS != rc)
    {
        //printf("%s\n", memcached_strerror(0, rc));
    }
}

//例子程序,通过配置构造一个连接池的种子,构造的对象是一个cache集群
memcached_st * create_memcached_seed_by_config()
{
    const char *config_string = "--SERVER=127.0.0.1:12111/?1 --SERVER=127.0.0.1:12112/?2 --SERVER=127.0.0.1:12113 --CONNECT-TIMEOUT=10";
    memcached_return_t rc;
    char errmsg[255] = { 0 };
    rc = libmemcached_check_configuration(config_string, strlen(config_string), errmsg, sizeof(errmsg));
    memcached_st *st = 0;
    if (MEMCACHED_SUCCESS != rc)
    {
        printf("check_configuration fail,%s.%s\n", errmsg, config_string);
        return st;
    }

    return st = memcached(config_string, strlen(config_string));
}


//例子程序,通过服务列表构造一个连接池的种子,构造的对象是一个cache集群
memcached_st * create_memcached_seed_by_srv_list()
{
    const char *hosts[] = { "127.0.0.1", "127.0.0.1", "127.0.0.1" };
    size_t port[] = { 12111, 12112, 12113 };
    size_t weight[] = { 1, 2, 1 };

    memcached_server_list_st list = 0;
    bool result = true;
    size_t total = sizeof(weight) / sizeof(size_t);
    for (size_t i = 0; i < total; ++i)
    {
        memcached_return_t rc;
        list = memcached_server_list_append_with_weight(list, hosts[i], port[i], weight[i], &rc);
        if (MEMCACHED_SUCCESS != rc)
        {
            printf("server_list_append_with_weight fail,%s\n", memcached_strerror(0, rc));
            result = false;
            break;
        }
    }

    memcached_st *st = 0;
    if (result)
    {
        st = memcached_create(0);
        memcached_server_push(st, list);
        printf("memcached_server_list_count:%u\n", memcached_server_list_count(list));
    }

    if (list)
    {
        memcached_server_list_free(list);
    }
    return st;
}

//例子程序,通过服务直接构造一个连接池的种子,构造的对象是一个cache集群
memcached_st * create_memcached_seed_by_host()
{
    const char *hosts[] = { "127.0.0.1", "127.0.0.1", "127.0.0.1" };
    size_t port[] = { 12111, 12112, 12113 };
    size_t weight[] = { 1, 2, 1 };

    bool result = true;
    size_t total = sizeof(weight) / sizeof(size_t);
    memcached_st *st = memcached_create(0);
    assert(st);
    for (size_t i = 0; i < total; ++i)
    {
        memcached_return_t rc;
        rc = memcached_server_add_with_weight(st, hosts[i], port[i], weight[i]);
        if (MEMCACHED_SUCCESS != rc)
        {
            printf("server_add_with_weight fail,%s\n", memcached_strerror(0, rc));
            result = false;
            break;
        }
    }

    if (!result)
    {
        memcached_free(st);
        st = 0;
    }
    else
    {
        printf("memcached_server_count,%u\n", memcached_server_count(st));
    }
    return st;
}

//例子程序,得到一个cache集群的配置
void get_memcached_config(memcached_st *st)
{
    assert(st);

    uint64_t result = memcached_behavior_get(st, MEMCACHED_BEHAVIOR_REMOVE_FAILED_SERVERS);
    printf("MEMCACHED_BEHAVIOR_REMOVE_FAILED_SERVERS,%llu\n", result);

    result = memcached_behavior_get(st, MEMCACHED_BEHAVIOR_SND_TIMEOUT);
    printf("MEMCACHED_BEHAVIOR_SND_TIMEOUT,%llu\n", result);

    result = memcached_behavior_get(st, MEMCACHED_BEHAVIOR_NO_BLOCK);
    printf("MEMCACHED_BEHAVIOR_NO_BLOCK ,%lld\n", result);

    result = memcached_behavior_get(st, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS);
    printf("MEMCACHED_BEHAVIOR_BUFFER_REQUESTS,%llu\n", result);

    result = memcached_behavior_get(st, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT);
    printf("MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT,%llu\n", result);

    result = memcached_behavior_get(st, MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT);
    printf("MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT,%llu\n", result);    
}

//例子程序,设置一个cache集群的配置
void set_memcached_config(memcached_st *st)
{
    assert(st);
    
    memcached_return_t rc;
    rc = memcached_behavior_set(st, MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
    if (MEMCACHED_SUCCESS != rc)
    {
        printf("set MEMCACHED_BEHAVIOR_NO_BLOCK fail,%s\n", memcached_strerror(st, rc));
    }

    rc = memcached_behavior_set(st, MEMCACHED_BEHAVIOR_RCV_TIMEOUT, 1000 * 1000 * 3);
    if (MEMCACHED_SUCCESS != rc)
    {
        printf("set MEMCACHED_BEHAVIOR_RCV_TIMEOUT fail,%s\n", memcached_strerror(st, rc));
    } 
}

//例子程序,获取一个集群池的配置
void get_memcached_pool_config(memcached_pool_st *pool)
{
    assert(pool);

    uint64_t result = 0;
    memcached_return_t rc;
    rc = memcached_pool_behavior_get(pool, MEMCACHED_BEHAVIOR_NO_BLOCK, &result);
    printf("MEMCACHED_BEHAVIOR_NO_BLOCK ,%llu\n", result);

    rc = memcached_pool_behavior_get(pool, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, &result);
    printf("MEMCACHED_BEHAVIOR_BUFFER_REQUESTS,%llu\n", result);

    rc = memcached_pool_behavior_get(pool, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, &result);
    printf("MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT,%llu\n", result);

    rc = memcached_pool_behavior_get(pool, MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT, &result);
    printf("MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT,%llu\n", result);

    rc = memcached_pool_behavior_get(pool, MEMCACHED_BEHAVIOR_SND_TIMEOUT, &result);
    printf("MEMCACHED_BEHAVIOR_SND_TIMEOUT,%llu\n", result);

    
    rc = memcached_pool_behavior_get(pool, MEMCACHED_BEHAVIOR_RCV_TIMEOUT, &result);
    printf("MEMCACHED_BEHAVIOR_RCV_TIMEOUT,%llu\n", result);


    
    rc = memcached_pool_behavior_get(pool, MEMCACHED_BEHAVIOR_REMOVE_FAILED_SERVERS, &result);
    printf("MEMCACHED_BEHAVIOR_REMOVE_FAILED_SERVERS,%llu\n", result);
}

//例子程序,设置一个集群池的配置
bool set_memcached_pool_config(memcached_pool_st *pool)
{
    assert(pool);

    memcached_return_t rc;
    rc = memcached_pool_behavior_set(pool, MEMCACHED_BEHAVIOR_SND_TIMEOUT, 1000 * 1000 * 3);
    if (MEMCACHED_SUCCESS != rc)
    {
        printf("MEMCACHED_BEHAVIOR_SND_TIMEOUT fail,%s\n", memcached_strerror(0, rc));
    }

    rc = memcached_pool_behavior_set(pool, MEMCACHED_BEHAVIOR_RCV_TIMEOUT, 1000 * 1000 * 4);
    if (MEMCACHED_SUCCESS != rc)
    {
        printf("MEMCACHED_BEHAVIOR_RCV_TIMEOUT fail,%s\n", memcached_strerror(0, rc));
    }

    rc = memcached_pool_behavior_set(pool, MEMCACHED_BEHAVIOR_REMOVE_FAILED_SERVERS, 3);
    if (MEMCACHED_SUCCESS != rc)
    {
        printf("MEMCACHED_BEHAVIOR_REMOVE_FAILED_SERVERS fail,%s\n", memcached_strerror(0, rc));
    }    
}


//例子程序,一次获取多个key的值
void mget_memcached(memcached_st *st,const char *keys[], size_t key_length[], size_t key_count)
{
    assert(st || keys || key_length > 0 || key_count > 0);

    uint32_t count = 0, flags = 0;
    char return_key[MEMCACHED_MAX_KEY] = { 0 };
    size_t return_key_length;
    char *return_value = 0;
    size_t return_value_length;
    
    memcached_return_t rc = memcached_mget(st, keys, key_length, key_count);
    while ((return_value = memcached_fetch(st, return_key, &return_key_length,
        &return_value_length, &flags, &rc)))
    {
        count++;
        printf("key=%s,keylength=%u,value=%s,total=%u\n", return_key, return_key_length, return_value, count);
        free(return_value);
        //memset(return_key, 0, sizeof(return_key));
    }
}

//例子程序,一次获取指定一个key的值
void get_memcached_by_key(memcached_st *st,const char *key, size_t key_length)
{
    assert(st || key || key_length > 0 );

    uint32_t flags = 0, value_length = 0;
    memcached_return_t rc;
    char *value = memcached_get(st, key, key_length, &value_length, &flags, &rc);
    if (MEMCACHED_SUCCESS != rc)
    {
        printf("%s\n", memcached_strerror(st, rc));
    }
    else
    {
        printf("key=%s,key_length=%u,value=%s,value_length=%u\n", key, key_length, value, value_length);
        free(value);
    }
}

//完整测试程序
int main(int argc, char *argv[])
{
    printf("memcached_lib_version:%s\n", memcached_lib_version());

    //得到一个集群种子,三个方法目的和功能一样,任选一种既可
    memcached_st * memc = create_memcached_seed_by_host();
    //memcached_st * memc = create_memcached_seed_by_srv_list();
    //memcached_st * memc = create_memcached_seed_by_config();
    assert(memc);

    //设置集群种子的属性
    set_memcached_config(memc);
    get_memcached_config(memc);
    printf("----------\n");

    //创建集群池,集群池释放,自动释放全部资源(包括种子)
    const uint32_t INITIAL = 1, MAX = 4;
    memcached_pool_st *pool = memcached_pool_create(memc, INITIAL, MAX);
    assert(pool);

    //设置集群种子的属性
    set_memcached_pool_config(pool);
    get_memcached_pool_config(pool);
    printf("----------\n");

    //从集群池取出一个集群访问对象
    struct timespec spec = { 3, 0 };
    memcached_return_t rc;
    memc = fetch_memcached_wait(pool, &spec, &rc);
    if (MEMCACHED_SUCCESS != rc)
    {
        printf("fetch_memcached fail,%s\n", memcached_strerror(0, rc));        
        memcached_pool_destroy(pool);
        return 0;
    }

    const char *keys[] = { "P", "1", "2", "3", "a", "#", "^", "*", "&", ",", "?" };
    size_t key_length[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    const char *values[] = { "11111", "222b2", "33333", "aaaa", "PPO1", "OO#", "^ww", "***", "&&&", ",,,", "?????" };
    size_t i = 0, total = sizeof(keys) / sizeof(char *);
    while(memc && total > i)
    {
        //设置多个值,分散存储在多个服务
        rc = memcached_set(memc, keys[i], strlen(keys[i]), values[i], strlen(values[i]), 0, 0);
        if (MEMCACHED_SUCCESS != rc)
        {
            printf("%s,%d\n", memcached_strerror(memc, rc), i);
        }
        i++;
    }

    if (memc)
    {
        //释放取出的集群访问对象
        release_memcached(pool,memc);
    }

    //无阻塞,取出一个访问对象
    memc = fetch_memcached(pool);
    //使用不同方式访问对象
    mget_memcached(memc,keys, key_length, sizeof(keys) / sizeof(char *));
    printf("----------\n");
    //mget_memcached(memc,keys, key_length, sizeof(keys) / sizeof(char *));
    get_memcached_by_key(memc,keys[10], strlen(keys[10]));
    get_memcached_by_key(memc,keys[4], strlen(keys[4]));
    get_memcached_by_key(memc,keys[3], strlen(keys[3]));
    
    if (memc)
    {
        //释放取出的集群访问对象
        release_memcached(pool, memc);
    }

    //集群池释放, 自动释放全部资源(包括种子)
    memcached_pool_destroy(pool);

    //memcached_return_t rc = MEMCACHED_NOTFOUND;
    //printf("aa=%s\n", memcached_strerror(0, rc));
    printf("over .\n");
    return 0;
}

/*
xuqnqn@xuqnqn-VirtualBox:~/work/memcache/test$ ./memcached_test 
memcached_lib_version:1.0.18
memcached_server_count,3
MEMCACHED_BEHAVIOR_REMOVE_FAILED_SERVERS,5
MEMCACHED_BEHAVIOR_SND_TIMEOUT,0
MEMCACHED_BEHAVIOR_NO_BLOCK ,1
MEMCACHED_BEHAVIOR_BUFFER_REQUESTS,0
MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT,4000
MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT,5
----------
MEMCACHED_BEHAVIOR_NO_BLOCK ,1
MEMCACHED_BEHAVIOR_BUFFER_REQUESTS,0
MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT,4000
MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT,3
MEMCACHED_BEHAVIOR_SND_TIMEOUT,3000000
MEMCACHED_BEHAVIOR_RCV_TIMEOUT,4000000
MEMCACHED_BEHAVIOR_REMOVE_FAILED_SERVERS,3
----------
key=P,keylength=1,value=11111,total=1
key=#,keylength=1,value=OO#,total=2
key=*,keylength=1,value=***,total=3
key=&,keylength=1,value=&&&,total=4
key=?,keylength=1,value=?????,total=5
key=1,keylength=1,value=222b2,total=6
key=2,keylength=1,value=33333,total=7
key=3,keylength=1,value=aaaa,total=8
key=a,keylength=1,value=PPO1,total=9
key=^,keylength=1,value=^ww,total=10
key=,,keylength=1,value=,,,,total=11
----------
key=?,key_length=1,value=?????,value_length=5
key=a,key_length=1,value=PPO1,value_length=4
key=3,key_length=1,value=aaaa,value_length=4
over .

*/

/*
其中 127.0.0.1:12111有下面的6个key
1,2,3,a,(逗号),,^

其中 127.0.0.1:12112有下面4个key
P,*,&,#

其中 127.0.0.1:12113有下面的1个key
?

*/

