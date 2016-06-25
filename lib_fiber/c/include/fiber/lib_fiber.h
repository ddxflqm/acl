#ifndef LIB_FIBER_INCLUDE_H
#define LIB_FIBER_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 协程结构类型
 */
typedef struct FIBER FIBER;

/**
 * 创建一个协程
 * @param fn {void (*)(FIBER*, void*)} 协程运行时的回调函数地址
 * @param arg {void*} 回调 fn 函数时的第二个参数
 * @param size {size_t} 所创建协程所占栈空间大小
 * @return {FIBER*}
 */
FIBER *fiber_create(void (*fn)(FIBER *, void *), void *arg, size_t size);

/**
 * 获得所给协程的协程 ID 号
 * @param fiber {const FIBER*} 由 fiber_create 创建的协程对象
 * @return {int} 协程 ID 号
 */
int fiber_id(const FIBER *fiber);

/**
 * 获得当前所运行的协程的 ID 号
 * @return {int} 当前运行协程的 ID 号
 */
int fiber_self(void);

/**
 * 设置所给协程的错误号
 * @param fiber {FIBER*} 协程对象
 * @param errnum {int} 错误号
 */
void fiber_set_errno(FIBER *fiber, int errnum);

/**
 * 获得指定协程的错误号
 * @param fiber {FIBER*} 协程对象
 * @return {int} 所给协程错误号
 */
int fiber_errno(FIBER *fiber);

/**
 * 获得指定协程的当前状态
 * @param fiber {const FIBER*} 协程对象
 * @return {int} 协程状态
 */
int fiber_status(const FIBER *fiber);

/**
 * 将当前运行的协程挂起，由调度器选择下一个需要运行的协程
 * @return {int}
 */
int fiber_yield(void);

/**
 * 将指定协程对象置入待运行队列中
 * @param fiber {FIBER*}
 */
void fiber_ready(FIBER *fiber);

/**
 * 将当前运行的协程挂起，同时执行等待队列下一个待运行的协程
 */
void fiber_switch(void);

/**
 * 调用本函数启动协程的调度过程
 */
void fiber_schedule(void);

/**
 * 停止 IO 协程过程
 */
void fiber_io_stop(void);

/**
 * 使当前运行的协程休眠指定毫秒数
 * @param milliseconds {unsigned int} 指定要休眠的毫秒数
 * @return {unsigned int} 本协程休眠后再次被唤醒后剩余的毫秒数
 */
unsigned int fiber_delay(unsigned int milliseconds);

/**
 * 使当前运行的协程休眠指定秒数
 * @param seconds {unsigned int} 指定要休眠的秒数
 * @return {unsigned int} 本协程休眠后再次被唤醒后剩余的秒数
 */
unsigned int fiber_sleep(unsigned int seconds);

/**
 * 创建一个协程用作定时器
 * @param milliseconds {unsigned int} 所创建定时器被唤醒的毫秒数
 * @param fn {void (*)(FIBER*, void*)} 定时器协程被唤醒时的回调函数
 * @param ctx {void*} 回调 fn 函数时的第二个参数
 * @return {FIBER*} 新创建的定时器协程
 */
FIBER *fiber_create_timer(unsigned int milliseconds,
	void (*fn)(FIBER *, void *), void *ctx);

/**
 * 在定时器协程未被唤醒前，可以通过本函数重置该协程被唤醒的时间
 * @param timer {FIBER*} 由 fiber_create_timer 创建的定时器协程
 * @param milliseconds {unsigned int} 指定该定时器协程被唤醒的毫秒数
 */
void fiber_reset_timer(FIBER *timer, unsigned int milliseconds);

/**
 * 本函数设置 DNS 服务器的地址
 * @param ip {const char*} DNS 服务器 IP 地址
 * @param port {int} DNS 服务器的端口
 */
void fiber_set_dns(const char* ip, int port);

/* for fiber specific */

/**
 * 设定当前协程的局部变量
 * @param ctx {void *} 协程局部变量
 * @param free_fn {void (*)(void*)} 当协程退出时会调用此函数释放协程局部变量
 * @return {int} 返回所设置的协程局部变量的键值，返回 -1 表示当前协程不存在
 */
int fiber_set_specific(void *ctx, void (*free_fn)(void *));

/**
 * 获得当前协程局部变量
 * @param key {int} 由 fiber_set_specific 返回的键值
 * @retur {void*} 返回 NULL 表示不存在
 */
void *fiber_get_specific(int key);

/* fiber locking */

/**
 * 协程互斥锁
 */
typedef struct FIBER_MUTEX FIBER_MUTEX;

/**
 * 协程读写锁
 */
typedef struct FIBER_RWLOCK FIBER_RWLOCK;

/**
 * 创建协程互斥锁
 * @return {FIBER_MUTEX*}
 */
FIBER_MUTEX *fiber_mutex_create(void);

/**
 * 释放协程互斥锁
 * @param l {FIBER_MUTEX*} 由 fiber_mutex_create 创建的协程互斥锁
 */
void fiber_mutex_free(FIBER_MUTEX *l);

/**
 * 对协程互斥锁进行阻塞式加锁，如果加锁成功则返回，否则则阻塞
 * @param l {FIBER_MUTEX*} 由 fiber_mutex_create 创建的协程互斥锁
 */
void fiber_mutex_lock(FIBER_MUTEX *l);

/**
 * 对协程互斥锁尝试性进行加锁，无论是否成功加锁都会立即返回
 * @param l {FIBER_MUTEX*} 由 fiber_mutex_create 创建的协程互斥锁
 * @return {int} 如果加锁成功则返回非 0 值，否则返回 0
 */
int fiber_mutex_trylock(FIBER_MUTEX *l);

/**
 * 加锁成功的协程调用本函数进行解锁，调用本函数的协程必须是该锁的属主，否则
 * 内部会产生断言
 * @param l {FIBER_MUTEX*} 由 fiber_mutex_create 创建的协程互斥锁
 */
void fiber_mutex_unlock(FIBER_MUTEX *l);

/**
 * 创建协程读写锁
 * @return {FIBER_RWLOCK*}
 */
FIBER_RWLOCK *fiber_rwlock_create(void);

/**
 * 释放协程读写锁
 * @param l {FIBER_RWLOCK*} 由 fiber_rwlock_create 创建的读写锁
 */
void fiber_rwlock_free(FIBER_RWLOCK *l);

/**
 * 对协程读写锁加读锁，如果该锁当前正被其它协程加了读锁，则本协程依然可以
 * 正常加读锁，如果该锁当前正被其它协程加了写锁，则本协程进入阻塞状态，直至
 * 写锁释放
 * @param l {FIBER_RWLOCK*} 由 fiber_rwlock_create 创建的读写锁
 */
void fiber_rwlock_rlock(FIBER_RWLOCK *l);

/**
 * 对协程读写锁尝试性加读锁，加锁无论是否成功都会立即返回
 * @param l {FIBER_RWLOCK*} 由 fiber_rwlock_create 创建的读写锁
 * @retur {int} 返回 1 表示加锁成功，返回 0 表示加锁失败
 */
int fiber_rwlock_tryrlock(FIBER_RWLOCK *l);

/**
 * 对协程读写锁加写锁，只有当该锁未被任何协程加读/写锁时才会返回，否则阻塞，
 * 直至该锁可加写锁
 * @param l {FIBER_RWLOCK*} 由 fiber_rwlock_create 创建的读写锁
 */
void fiber_rwlock_wlock(FIBER_RWLOCK *l);

/**
 * 对协程读写锁尝试性加写锁，无论是否加锁成功都会立即返回
 * @param l {FIBER_RWLOCK*} 由 fiber_rwlock_create 创建的读写锁
 * @return {int} 返回 1 表示加写锁成功，返回 0 表示加锁失败
 */
int fiber_rwlock_trywlock(FIBER_RWLOCK *l);

/**
 * 对协程读写锁成功加读锁的协程调用本函数解读锁，调用者必须是之前已成功加读
 * 锁成功的协程
 * @param l {FIBER_RWLOCK*} 由 fiber_rwlock_create 创建的读写锁
 */
void fiber_rwlock_runlock(FIBER_RWLOCK *l);
/**
 * 对协程读写锁成功加写锁的协程调用本函数解写锁，调用者必须是之前已成功加写
 * 锁成功的协程
 * @param l {FIBER_RWLOCK*} 由 fiber_rwlock_create 创建的读写锁
 */
void fiber_rwlock_wunlock(FIBER_RWLOCK *l);

/* channel communication */

/**
 * 协程间通信的管道
 */
typedef struct CHANNEL CHANNEL;

/**
 * 创建协程通信管道
 * @param elemsize {int} 在 CHANNEL 进行传输的对象的固定尺寸大小（字节）
 * @param bufsize {int} CHANNEL 内部缓冲区大小，即可以缓存 elemsize 尺寸大小
 *  对象的个数
 * @return {CHANNNEL*}
 */
CHANNEL* channel_create(int elemsize, int bufsize);

/**
 * 释放由 channel_create 创建的协程通信管道对象
 * @param c {CHANNEL*} 由 channel_create 创建的管道对象
 */
void channel_free(CHANNEL *c);

/**
 * 阻塞式向指定 CHANNEL 中发送指定的对象地址
 * @param c {CHANNEL*} 由 channel_create 创建的管道对象
 * @param v {void*} 被发送的对象地址
 * @return {int} 返回值 >= 0
 */
int channel_send(CHANNEL *c, void *v);

/**
 * 非阻塞式向指定 CHANNEL 中发送指定的对象，内部会根据 channel_create 中指定
 * 的 elemsize 对象大小进行数据拷贝
 * @param c {CHANNEL*} 由 channel_create 创建的管道对象
 * @param v {void*} 被发送的对象地址
 */
int channel_send_nb(CHANNEL *c, void *v);

/**
 * 从指定的 CHANNEL 中阻塞式读取对象，
 * @param c {CHANNEL*} 由 channel_create 创建的管道对象
 * @param v {void*} 存放结果内容
 * @return {int} 返回值 >= 0 表示成功读到数据
 */
int channel_recv(CHANNEL *c, void *v);

/**
 * 从指定的 CHANNEL 中非阻塞式读取对象，无论是否读到数据都会立即返回
 * @param c {CHANNEL*} 由 channel_create 创建的管道对象
 * @param v {void*} 存放结果内容
 * @return {int} 返回值 >= 0 表示成功读到数据，否则表示未读到数据
 */
int channel_recv_nb(CHANNEL *c, void *v);

/**
 * 向指定的 CHANNEL 中阻塞式发送指定对象的地址
 * @param c {CHANNEL*} 由 channel_create 创建的管道对象
 * @param v {void*} 被发送对象的地址
 * @return {int} 返回值 >= 0
 */
int channel_sendp(CHANNEL *c, void *v);

/**
 * 从指定的 CHANNLE 中阻塞式接收由 channel_sendp 发送的对象的地址
 * @param c {CHANNEL*} 由 channel_create 创建的管道对象
 * @return {void*} 返回非 NULL，指定接收到的对象的地址
 */
void *channel_recvp(CHANNEL *c);

/**
 * 向指定的 CHANNEL 中非阻塞式发送指定对象的地址
 * @param c {CHANNEL*} 由 channel_create 创建的管道对象
 * @param v {void*} 被发送对象的地址
 * @return {int} 返回值 >= 0
 */
int channel_sendp_nb(CHANNEL *c, void *v);

/**
 * 从指定的 CHANNLE 中阻塞式接收由 channel_sendp 发送的对象的地址
 * @param c {CHANNEL*} 由 channel_create 创建的管道对象
 * @return {void*} 返回非 NULL，指定接收到的对象的地址，如果返回 NULL 表示
 *  没有读到任何对象
 */
void *channel_recvp_nb(CHANNEL *c);

/**
 * 向指定的 CHANNEL 中发送无符号长整形数值
 * @param c {CHANNEL*} 由 channel_create 创建的管道对象
 * @param val {unsigned long} 要发送的数值
 * @return {int} 返回值 >= 0
 */
int channel_sendul(CHANNEL *c, unsigned long val);

/**
 * 从指定的 CHANNEL 中接收无符号长整形数值
 * @param c {CHANNEL*} 由 channel_create 创建的管道对象
 * @return {unsigned long}
 */
unsigned long channel_recvul(CHANNEL *c);

/**
 * 向指定的 CHANNEL 中以非阻塞方式发送无符号长整形数值
 * @param c {CHANNEL*} 由 channel_create 创建的管道对象
 * @param val {unsigned long} 要发送的数值
 * @return {int} 返回值 >= 0
 */
int channel_sendul_nb(CHANNEL *c, unsigned long val);

/**
 * 从指定的 CHANNEL 中以非阻塞方式接收无符号长整形数值
 * @param c {CHANNEL*} 由 channel_create 创建的管道对象
 * @return {unsigned long}
 */
unsigned long channel_recvul_nb(CHANNEL *c);

/* master fibers server */

/**
 * 基于协程的服务器主函数入口，该模块可以在 acl_master 服务器控制框架下运行
 * @param argc {int} 使用者传入的参数数组 argv 的大小
 * @param argv {char*[]} 参数数组大小
 * @param service {void (*)(ACL_VSTREAM*, void*)} 接收到一个新客户端连接请求
 *  后创建一个协程回调本函数
 * @param ctx {void*} service 回调函数的第二个参数
 * @param name {int} 控制参数列表中的第一个控制参数
 */
void fiber_server_main(int argc, char *argv[],
	void (*service)(ACL_VSTREAM*, void*), void *ctx, int name, ...);

#ifdef __cplusplus
}
#endif

#endif
