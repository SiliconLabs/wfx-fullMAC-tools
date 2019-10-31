################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include targets/slstk3701a/wfx_host/subdir.mk
-include targets/slstk3701a/subdir.mk
-include src/subdir.mk
-include sl_drivers/subdir.mk
-include emlib/subdir.mk
-include emdrv/subdir.mk
-include Drivers/wfx_fmac_driver/secure_link/subdir.mk
-include Drivers/wfx_fmac_driver/bus/subdir.mk
-include Drivers/wfx_fmac_driver/subdir.mk
-include CMSIS/EFM32GG11B/subdir.mk
-include BSP/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: RF_test_agent.axf

# Tool invocations
RF_test_agent.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GNU ARM C Linker'
	arm-none-eabi-gcc -g3 -gdwarf-2 -mcpu=cortex-m4 -mthumb -T "RF_test_agent.ld" -Xlinker --gc-sections -Xlinker -Map="RF_test_agent.map" -mfpu=fpv4-sp-d16 -mfloat-abi=softfp --specs=nano.specs -o RF_test_agent.axf "./src/main.o" "./emlib/em_assert.o" "./emlib/em_cmu.o" "./emlib/em_core.o" "./emlib/em_emu.o" "./emlib/em_gpio.o" "./emlib/em_system.o" "./emlib/em_usart.o" "./Drivers/wfx_fmac_driver/secure_link/sl_wfx_secure_link.o" "./Drivers/wfx_fmac_driver/bus/sl_wfx_bus.o" "./Drivers/wfx_fmac_driver/bus/sl_wfx_bus_spi.o" "./Drivers/wfx_fmac_driver/sl_wfx.o" "./CMSIS/EFM32GG11B/startup_efm32gg11b.o" "./CMSIS/EFM32GG11B/system_efm32gg11b.o" "./BSP/bsp_bcc.o" "./BSP/bsp_stk_leds.o" "./BSP/bsp_trace.o" "./emdrv/dmadrv.o" "./emdrv/rtcdriver.o" "./emdrv/spidrv.o" "./emlib/em_ldma.o" "./emlib/em_prs.o" "./emlib/em_rmu.o" "./emlib/em_rtcc.o" "./emlib/em_timer.o" "./sl_drivers/retargetio.o" "./sl_drivers/retargetserial.o" "./sl_drivers/udelay.o" "./src/sl_wfx_test_agent.o" "./targets/slstk3701a/host_resources.o" "./targets/slstk3701a/wfx_host/wfx_host.o" "./targets/slstk3701a/wfx_host/wfx_host_spi.o" -Wl,--start-group -lgcc -lc -lnosys -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

	@echo 'Building hex file: RF_test_agent.hex'
	arm-none-eabi-objcopy -O ihex "RF_test_agent.axf" "RF_test_agent.hex"
	@echo ' '

	@echo 'Building bin file: RF_test_agent.bin'
	arm-none-eabi-objcopy -O binary "RF_test_agent.axf" "RF_test_agent.bin"
	@echo ' '

	@echo 'Building s37 file: RF_test_agent.s37'
	arm-none-eabi-objcopy -O srec "RF_test_agent.axf" "RF_test_agent.s37"
	@echo ' '

	@echo 'Running size tool'
	arm-none-eabi-size "RF_test_agent.axf"
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) RF_test_agent.axf
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
