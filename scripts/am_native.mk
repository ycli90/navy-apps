LNK_ADDR = $(if $(VME), 0x40000000, 0x06000000)
LDFLAGS += -Ttext-segment $(LNK_ADDR)
