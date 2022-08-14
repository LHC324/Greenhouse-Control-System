#ifndef _SHELL__PORT_H_
#define _SHELL__PORT_H_

#include "shell.h"
// #define USING_FREERTOS
//#define USING_RTTHREAD

/*定义shell缓冲区尺寸*/
#define SHELL_BUFFER_SIZE 128U

/*声明shell对象*/
extern Shell shell;
#define Shell_Object &shell

/*初始化shell*/
extern void User_Shell_Init(Shell *shell);

#endif /* _SHELL_PORT_H_ */
