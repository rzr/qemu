
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(CPPFLAGS) -c -o $@ $<

%.o: %.m
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

