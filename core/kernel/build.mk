src-prefix               := src/
src-y                    += critical.c mutex.c sem.c task.c tick.c class.c obj.c
src-$(CONFIG_IPC_EVENT)  += event.c
src-$(CONFIG_IPC_MQ)     += msg_queue.c
src-$(CONFIG_SOFT_TIMER) += timer.c
inc-g-y                  += h/