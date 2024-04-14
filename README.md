# 项目文件介绍

- src：参与Doxygen生成文档的项目源码
- libs：不参与Doxgyen生成文档，属于官方提供的库的源码
- makefile：编译文件
- .Doxygen：生成文档文件
- .gitignore：忽略文件，其结构对应此次
- README.md：自述文件
- bin：中间文件，输出文件，Doxygen输出于此；中间编译文件位于此；
- Winyunq.hex：烧录文件，其名称由Makefile决定

# 编译配置:

修改Makefile:

| 参数名称 | 含义 |
| --- | --- |
| TARGET | 相对于Makefile文件，最终生成的烧录hex文件名称 |
| ToolPath | 本地RISC-V GCC工具路径，需要指定至bin文件夹,即编译工具所处文件夹 |
| BoardVersion | 部署平台芯片，芯片不同，其对应的库文件亦不同 |
| ConfigureFlag | 芯片工作配置参数，对应CONFIG.H，该参数较为重要，在后文单独提及用法 |
| C_SOURCES | C源码包含路径，采用“+=”添加，添加文件夹的方式为(路径)/*.c |
| C_INCLUDES | 头文件路径，采用“+=添加” |
| CONFIGURES | 全局.h文件，本项目中，仅Configure.h为全局.h文件 |

编译命令:

    make

make命令编译默认单线程编译，事实上可以指定N线程编译

    make -jN

清除编译文件

    make clean

发布编译文件（即优化等级最高）

    make release