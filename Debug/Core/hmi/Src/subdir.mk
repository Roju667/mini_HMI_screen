################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/hmi/Src/hmi.c \
../Core/hmi/Src/hmi_draw.c \
../Core/hmi/Src/hmi_edit_menu.c \
../Core/hmi/Src/hmi_mock.c \
../Core/hmi/Src/xgb_comm.c 

OBJS += \
./Core/hmi/Src/hmi.o \
./Core/hmi/Src/hmi_draw.o \
./Core/hmi/Src/hmi_edit_menu.o \
./Core/hmi/Src/hmi_mock.o \
./Core/hmi/Src/xgb_comm.o 

C_DEPS += \
./Core/hmi/Src/hmi.d \
./Core/hmi/Src/hmi_draw.d \
./Core/hmi/Src/hmi_edit_menu.d \
./Core/hmi/Src/hmi_mock.d \
./Core/hmi/Src/xgb_comm.d 


# Each subdirectory must supply rules for building sources it contributes
Core/hmi/Src/%.o: ../Core/hmi/Src/%.c Core/hmi/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103x6 -c -I../Core/Inc -I../Core/hmi/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

