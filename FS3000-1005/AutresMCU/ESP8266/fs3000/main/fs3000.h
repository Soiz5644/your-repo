#ifndef _FS3000_H_
#define _FS3000_H_

#include <stdint.h>
#include <stdbool.h>

#define FS3000_I2CADDR 0x28

void FS3000_Init(void);
float FS3000_ReadData(void);

#endif /* _FS3000_H_ */