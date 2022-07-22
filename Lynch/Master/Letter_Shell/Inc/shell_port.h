#ifndef _SHELL__PORT_H_
#define _SHELL__PORT_H_

#include "shell.h"
// #define USING_FREERTOS
//#define USING_RTTHREAD
// #define USING_USB

/*定义shell缓冲区尺寸*/
#define SHELL_BUFFER_SIZE 512U

/*声明shell对象*/
extern Shell shell;
#define Shell_Object &shell

/*初始化shell*/
extern void MX_ShellInit(Shell *shell);

#endif /* _SHELL_PORT_H_ */
