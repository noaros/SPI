arm-none-eabi-gcc -fno-strict-aliasing -g -Tlink.ld -nostdlib 0.c ../cmsis_f4/Source/Templates/gcc/startup_stm32f429xx.s \
    -I../cmsis_core/CMSIS/Core/Include -I../cmsis_f4/Include -mcpu=cortex-m4 -lc -lgcc --specs nano.specs
arm-none-eabi-objcopy -O binary a.out 0.bin
