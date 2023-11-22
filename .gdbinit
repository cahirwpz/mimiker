# favorite set of breakpoints
break kernel_init
break halt

# skip by default some non-interesting stuff
skip -function tlb_refill -file ebase.S

# extra commands defined by means of gdb scripts
define nextuser
  tbreak user_return
  commands
  silent
  si
  end
  continue
end

document nextuser
Continue until first user-space instruction after exiting kernel-space.
end

# favorite set of debugging printf's
#dprintf proc_lock,"proc_lock(0x%08x)\n",p
#dprintf proc_unlock,"proc_unlock(0x%08x)\n",p

#break main
