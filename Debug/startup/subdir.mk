################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../startup/startup_stm32.s 

OBJS += \
./startup/startup_stm32.o 


# Each subdirectory must supply rules for building sources it contributes
startup/%.o: ../startup/%.s
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Assembler'
	@echo $(PWD)
	arm-none-eabi-as -mcpu=cortex-m0 -mthumb -mfloat-abi=soft -I"/home/shay/a/gscheng/workspace/Lab_7_Student/StdPeriph_Driver/inc" -I"/home/shay/a/gscheng/workspace/Lab_7_Student/inc" -I"/home/shay/a/gscheng/workspace/Lab_7_Student/CMSIS/device" -I"/home/shay/a/gscheng/workspace/Lab_7_Student/CMSIS/core" -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


