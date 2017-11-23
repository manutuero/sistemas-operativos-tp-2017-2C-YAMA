################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/funcionesPlanificacion.c \
../src/funcionesYAMA.c \
../src/yama.c 

OBJS += \
./src/funcionesPlanificacion.o \
./src/funcionesYAMA.o \
./src/yama.o 

C_DEPS += \
./src/funcionesPlanificacion.d \
./src/funcionesYAMA.d \
./src/yama.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


