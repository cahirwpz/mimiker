# some generic settings
set output-radix 0x10
set pagination off
set confirm off
set verbose off

# favorite set of breakpoints
break kernel_init
break assert_fail
break panic_fail

# favorite set of debugging printf's
#dprintf proc_lock,"proc_lock(0x%08x)\n",p
#dprintf proc_unlock,"proc_unlock(0x%08x)\n",p

# user space program debugging
#add-symbol-file user/utest/utest.uelf 0x400000
#break main

# make extra commands available
python import os, sys
python sys.path.append(os.getcwd())
python import debug
