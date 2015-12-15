#include <unistd.h>
#include <errno.h>
#include "uart_raw.h"

int write(int __fd, const void *__buf, size_t __nbyte ){
	/* Ignore all file writes as not supported, EXCEPT when writing to
     * file 1 (stdout) or 2 (stderr).
     * In such case, push data to serial port.
     */
    if(__fd == 1 || __fd == 2){
        // TODO: Implement uart_putnstr
		const char* str = (const char*) __buf;
		int i = 0;
        while(i < __nbyte)
            uart_putch(str[i++]);
        // Report we have sucessfully written all bytes.
        return __nbyte;
    }
	errno = ENOTSUP;
	return -1;
}
