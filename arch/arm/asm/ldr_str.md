ldr vs str
========================================

* mov:  只能在寄存器之间移动数据，或者把立即数移到寄存器中.

* ldr ：加载指令（将数据从内存中移到寄存器）将数据从内存中移动到寄存器中

1. ldr r0, 0x12345678就是把0x12345678这个地址中的值存放到r0中.

2. ldr伪指令可以在立即数前加上=，以表示把一个地址写到某寄存器中.
ldr r0, =0x12345678这样，就把0x12345678这个地址写到r0中了。
所以，ldr伪指令和mov是比较相似的。只不过mov指令限制了立即数的长度为8位，
也就是不能超过512。而ldr伪指令没有这个限制。如果使用ldr伪指令时，
后面跟的立即数没有超过8位，那么在实际汇编的时候该ldr伪指令是被转换为mov指令的。

伪指令LDR R0,=0x12345678其实就是被编译成LDR R0,[PC,Immediate]。
其中立即数0x12345678被储存在一个文字池中，它的地址和指令LDR R0,[PC,Immediate]
的地址之前差了Immediate。因此指令LDR R0,[PC,Immediate]就可以把立即数0x123456读取到R0中了。

* STR: 存储指令（将数据存储到内存中） 如  STR  R0,[R1]，将R0的值存储到以R1为地址的内存单元中

例子：汇编语言对变量赋值

```
1.COUNT EQU       0x40003100

LDR       R1,=COUNT

MOV      R0,#0

STR       R0,[R1]
```

COUNT是我们定义的一个变量，地址为0x40003100。这中定义方法在汇编语言中是很常见的，如果使用过单片机的话，应该都熟悉这种用法。

LDR       R1,=COUNT是将COUNT这个变量的地址，也就是0x40003100放到R1中。

MOV      R0,#0是将立即数0放到R0中。

最后一句STR      R0,[R1]是一个典型的存储指令，
将R0中的值放到以R1中的值为地址的存储单元去。
实际就是将0放到地址为0x40003100的存储单元中去。

用三条指令来完成对一个变量的赋值，看起来有点不太舒服。这可能跟ARM的采用RISC有关。