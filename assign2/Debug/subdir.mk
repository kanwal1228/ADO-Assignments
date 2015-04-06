################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../buffer_mgr.c \
../buffer_mgr_stat.c \
../dberror.c \
../storage_mgr.c \
../test_assign2_1.c 

OBJS += \
./buffer_mgr.o \
./buffer_mgr_stat.o \
./dberror.o \
./storage_mgr.o \
./test_assign2_1.o 

C_DEPS += \
./buffer_mgr.d \
./buffer_mgr_stat.d \
./dberror.d \
./storage_mgr.d \
./test_assign2_1.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


