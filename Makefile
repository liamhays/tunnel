SMSLIB=SMSlib_GG.lib
SDCC_MAX_ALLOCS_PER_NODE=3000
SDCC_FLAGS=--allow-unsafe-read --max-allocs-per-node $(SDCC_MAX_ALLOCS_PER_NODE) --opt-code-speed -mz80 --std-sdcc99
all:
# The architecture of the Z80 means we can enable unsafe reads, according to the SDCC manual
	sdcc $(SDCC_FLAGS) -c --peep-file peep-rules.txt tunnel.c
	sdcc $(SDCC_FLAGS) -o tunnel.ihx --no-std-crt0 --data-loc 0xC000 crt0_sms.rel tunnel.rel $(SMSLIB)
	ihx2sms tunnel.ihx tunnel.gg

clean:
	rm tunnel.asm tunnel.rel tunnel.lst tunnel.ihx tunnel.map tunnel.lk tunnel.noi tunnel.sym

ihx2sms:
	ihx2sms tunnel.ihx tunnel.gg
