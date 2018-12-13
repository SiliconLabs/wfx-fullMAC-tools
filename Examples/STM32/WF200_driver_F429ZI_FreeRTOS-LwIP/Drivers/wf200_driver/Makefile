# WF200 Wi-Fi driver

# Specific build
WF200_BUS  := spi
#WF200_BUS  := sdio


spi_SOURCE  := bus/wf200_bus_spi.c
sdio_SOURCE := bus/wf200_bus_sdio.c


SOURCE := wf200.c \
          wf200_sl_wifi_api.c \
          wf200_pds.c \
          bus/wf200_bus.c \
          $($(WF200_BUS)_SOURCE)
          
INCLUDE := sl \
           bus \
           firmware/1.2.14 \
           .

# Build recipes
all: .output/wf200.a

.output/wf200.a: $(addprefix .output/,$(SOURCE:.c=.o)) | .output
	$(AR) -rcs $@ $^

.output/%.o: %.c | .output/%.d
	$(CC) -c -o $@ $(addprefix -I,$(INCLUDE)) $<

.output/%.d:
	@mkdir -p $(dir $@)