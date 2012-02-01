all: tunslip tapslip6 tunslip6 senslip senslip6

%: src/%.c
	$(CC) $? -o $@

clean:
	rm -f tunslip tapslip6 tunslip6 senslip senslip6
