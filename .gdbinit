set tcp connect-timeout 30
target remote localhost:1234
break kernel_init
break assert_fail
break panic_fail
continue

source debug/ktrace.py
source debug/kdump.py
source debug/mutex.py
