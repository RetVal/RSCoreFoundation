//
//  Fix.m
//  RSCoreFoundation
//
//  Created by closure on 7/16/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, UserSex) {
    UserSexMale = 1,
    UserSexFemale
};

@interface UserModel : NSObject
@property (copy, nonatomic, readonly) NSString *name;
@property (assign, nonatomic, readonly) NSInteger age;
@property (assign, nonatomic, readonly) UserSex sex;
- (instancetype)initWithName:(NSString *)name age:(NSInteger)age;
- (void)loginWithComplete:(void (^)())handler;
@end

@implementation UserModel

- (void)observeValueForKeyPath:(nullable NSString *)keyPath ofObject:(nullable id)object change:(nullable NSDictionary *)change context:(nullable void *)context {
    
}
@end

/*
@property 后面可以有哪些修饰符？
什么情况使用 weak 关键字，相比 assign 有什么不同？
怎么用 copy 关键字？
这个写法会出什么问题： @property (copy) NSMutableArray *array;
如何让自己的类用 copy 修饰符？如何重写带 copy 关键字的 setter？
这一套问题区分度比较大，如果上面的问题都能回答正确，可以延伸问更深入点的：

@property 的本质是什么？ivar、getter、setter 是如何生成并添加到这个类中的
@protocol 和 category 中如何使用 @property
runtime 如何实现 weak 属性
每个人擅长的领域不一样，我们一般会从简历上找自己写擅长的技术聊，假如自己并不是很熟，最好别写出来或扯出来，万一面试官刚好非常精通这里就露馅了。

Checklist

总结过些面试题，没坚持下去，后来把这些当 checklist，面试的时候实在没话聊的时候做个提醒，语言、框架、运行机制性质的：

[※]@property中有哪些属性关键字？
[※]weak属性需要在dealloc中置nil么？
[※※]@synthesize和@dynamic分别有什么作用？
[※※※]ARC下，不显示指定任何属性关键字时，默认的关键字都有哪些？
[※※※]用@property声明的NSString（或NSArray，NSDictionary）经常使用copy关键字，为什么？如果改用strong关键字，可能造成什么问题？
[※※※]@synthesize合成实例变量的规则是什么？假如property名为foo，存在一个名为_foo的实例变量，那么还会自动合成新变量么？
[※※※※※]在有了自动合成属性实例变量之后，@synthesize还有哪些使用场景？

[※※]objc中向一个nil对象发送消息将会发生什么？
[※※※]objc中向一个对象发送消息[obj foo]和objc_msgSend()函数之间有什么关系？
[※※※]什么时候会报unrecognized selector的异常？
[※※※※]一个objc对象如何进行内存布局？（考虑有父类的情况）
[※※※※]一个objc对象的isa的指针指向什么？有什么作用？
[※※※※]下面的代码输出什么？

@implementation Son : Father
- (id)init
{
self = [super init];
if (self) {
    NSLog(@"%@", NSStringFromClass([self class]));
    NSLog(@"%@", NSStringFromClass([super class]));
}
return self;
}
@end
[※※※※]runtime如何通过selector找到对应的IMP地址？（分别考虑类方法和实例方法）
[※※※※]使用runtime Associate方法关联的对象，需要在主对象dealloc的时候释放么？
[※※※※※]objc中的类方法和实例方法有什么本质区别和联系？
[※※※※※]_objc_msgForward函数是做什么的，直接调用它将会发生什么？
[※※※※※]runtime如何实现weak变量的自动置nil？
[※※※※※]能否向编译后得到的类中增加实例变量？能否向运行时创建的类中添加实例变量？为什么？

[※※※]runloop和线程有什么关系？
[※※※]runloop的mode作用是什么？
[※※※※]以+ scheduledTimerWithTimeInterval...的方式触发的timer，在滑动页面上的列表时，timer会暂定回调，为什么？如何解决？
[※※※※※]猜想runloop内部是如何实现的？

[※]objc使用什么机制管理对象内存？
[※※※※]ARC通过什么方式帮助开发者管理内存？
[※※※※]不手动指定autoreleasepool的前提下，一个autorealese对象在什么时刻释放？（比如在一个vc的viewDidLoad中创建）
[※※※※]BAD_ACCESS在什么情况下出现？
[※※※※※]苹果是如何实现autoreleasepool的？

[※※]使用block时什么情况会发生引用循环，如何解决？
[※※]在block内如何修改block外部变量？
[※※※]使用系统的某些block api（如UIView的block版本写动画时），是否也考虑引用循环问题？

[※※]GCD的队列（dispatch_queue_t）分哪两种类型？
[※※※※]如何用GCD同步若干个异步调用？（如根据若干个url异步加载多张图片，然后在都下载完成后合成一张整图）
[※※※※]dispatch_barrier_async的作用是什么？
[※※※※※]苹果为什么要废弃dispatch_get_current_queue？
[※※※※※]以下代码运行结果如何？

- (void)viewDidLoad
{
    [super viewDidLoad];
    NSLog(@"1");
    dispatch_sync(dispatch_get_main_queue(), ^{
        NSLog(@"2");
    });
    NSLog(@"3");
}
[※※]addObserver:forKeyPath:options:context:各个参数的作用分别是什么，observer中需要实现哪个方法才能获得KVO回调？
[※※※]如何手动触发一个value的KVO
[※※※]若一个类有实例变量NSString *_foo，调用setValue:forKey:时，可以以foo还是_foo作为key？
[※※※※]KVC的keyPath中的集合运算符如何使用？
[※※※※]KVC和KVO的keyPath一定是属性么？
[※※※※※]如何关闭默认的KVO的默认实现，并进入自定义的KVO实现？
[※※※※※]apple用什么方式实现对一个对象的KVO？

[※※]IBOutlet连出来的视图属性为什么可以被设置成weak?
[※※※※※]IB中User Defined Runtime Attributes如何使用？

[※※※]如何调试BAD_ACCESS错误
[※※※]lldb（gdb）常用的调试命令？*/

// [*] atomic, nonatomic, strong, weak, assign, setter, getter, readonly, readwrite, nonnull, nullable, null_resettable

// [*] weak 作用于对象（弱引用），assign作用于POD type（赋值），weak会自动重置为nil

// [**] @synthesize 对象锁代码块，@dynamic 运行时生成内容绑定

// [***] strong, atomic, null_resettable

// [***] immutable object 去掉副作用

// [*****] ivar的某些同步操作限制

// [**] runtime直接返回 不会产生什么效果

// [***] [obj foo]; <~> objc_msgSend(obj, "foo");

// [***] unrecognized selector:
/*
    1.  消息不存在于目标缓存
    2.  消息不存在于目标消息列表
    3.  如果目标有父类且不是自己，目标改为父类重复1，否则4
    4.  进入动态决议，提供动态添加Method给目标消息，否则5
    5.  进入消息转发流程，先进入快速转发流程，返回对象不为self且响应消息，否则5
    6.  产生invocation，进入普通转发流程，不响应抛出异常
 */

// [****] objc_class, 直接得到类型，最简单的作用是有继承关系, 消息缓存，设置类型相关的使用（objc_assign), 自动KVO实现

// [****] Son\nSon\n 因为实例isa都指向Son

// [****] 类方法：查找消息缓存，查找metaClass消息表，查找父类重复 实例：查找消息缓存

// [****] 不需要, runtime会检查被dealloc的对象中associate对象并调用release

// [*****] 保存的地点不同，类方法存放在metaClass实例中，实例方法放在class(isa)中, 实现方式一致。


// [*****] 进消息转发的runtime入口，直接进入消息转发

// [*****] objc_initWeak, objc_destoryWeak, weak_table

// [*****] 不能，能。编译器编译后，代码段只读不可写。运行时内容动态创建绑定。

// [***] 每个线程都可以有runloop，也可以没有，但最多只有一个, 惰性创建，无法手动销毁。

// [***] 每个runloop的mode其实是一个带名字的集合，集合内有该分支下的source, timer, observer

// [****] 因为是默认加载到Main Thread的runloop上的，滑动时主线程处理滑动手势也在主线程runloop上，其他runloop资源阻塞，timer无法触发。加到其他runloop上／使用common mode

// [******] 读过CoreFoundation, 也有自己版本, 其中runloop也是看了很久，以dispatch的一个4CF私有端口和mach port为事件源核心，以source, timer, observer为基础成员作为事件触发器来提供服务，并以不同mode进行隔离，还有维护block队列，observer根据触发flag在rl各个时间点触发，检查block队列并执行，source在被调度时触发，timer会在触发后计算下次触发时间以及时机计算细节，通过系统提供的mach port进行事件等待，rl进入休眠，从休眠中醒来，检查block队列并执行，触发对应阶段的observer，倾倒当前的自动释放池，判断stop flag，没有stop则是while无限循环，否则从Run中返回，其中顺序可能有问题，步骤有遗漏写了大概没有去再次校对代码细节。

// [*] 引用计数器内存模型，MRC/ARC机制

// [****] 编译时通过规则计算变量生命周期，在生命周期结束时插入release／autorelease，原理跟c++的RAII一致

// [****] 在runloop的下一个周期前释放

// [****] BAD_ACCESS 内存访问错误，最简单的一个例子 delegate野指针

// [*****] 记得是objc-runtime/NSObject.mm中实现，4k内存一个自动释放池，池与池之间双向链表链接，保存在当前线程存储区中，除了头部信息外在池子未满之前所有的autorelease不过是向hotpool添加一条记录，hotpool满之后创建子池重设hotpool继续，倾倒先倾倒子池，然后倾倒父池，直到倾倒完要求倾倒的层级。

// [**] 当前类产生一个block，并内部使用了当前类的self. weak解决，注意weak可能会变成nil

// [**] __block关键字给目标变量添加描述

// [***] 不考虑

// [**] 并发队列，串行队列

// [****] dispatch_group_t

// [※※※※] 在并发队列里，提交的这个block不会和其他block并发执行，而是在之前提交的block并发执行后，执行此处的block，可以规避并发IO操作产生乱序

// [*****] dispatch_get_current_queue

// [*****] 1

// [**] 监听事件的对象，被监听者属性路径，监听事件标示，自定义上下文环境, observeValueForKeyPath:(nullable NSString *)keyPath ofObject:(nullable id)object change:(nullable NSDictionary *)change context:(nullable void *)context

// [***] willChange, didChange

// [***] foo和_foo都可以

// [****] xx.@count.amout

// [****] 方法也可以

// [*****] automaticallyNotifiesObserversForKey 返回NO，keyPathsForValuesAffectingValueForKey 处理自定义内容

// [*****] runtime会自动产生一个子类，将目标isa重定向到新生成的类并重写对应的getter，setter

// [**] IB的storyboard/xib 也是实体的拥有者，具有strong连接

// [※※※※※]

