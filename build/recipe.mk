# Disable all built-in recipes
.SUFFIXES:

%.S: %.c
	@echo "[CC] $(DIR)$< -> $(DIR)$@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -S -o $@ $<

%.o: %.c
	@echo "[CC] $(DIR)$< -> $(DIR)$@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

%.o: %.S
	@echo "[AS] $(DIR)$< -> $(DIR)$@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

%.a:
	@echo "[AR] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(AR) rs $@ $^ 2> /dev/null
