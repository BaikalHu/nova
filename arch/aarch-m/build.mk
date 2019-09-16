src-prefix            := src/
src-$(CONFIG_GNUC)    += context.c cpu.c exception.c handlers.c nvic.c trace.c vectors.c
src-$(CONFIG_SESA)    += sesa/*.c
src-$(CONFIG_SYSTICK) += systick.c
src-$(CONFIG_SVC)     += svc.c
src-$(CONFIG_MPU)     += mpu.c
usr-y                 += user.c
usr-$(CONFIG_SESA)    += sesa/*.c
inc-g                 += h/ h/cmsis/