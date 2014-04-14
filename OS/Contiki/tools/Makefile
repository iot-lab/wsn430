all: tunslip tapslip6 tunslip6 senslip sentapslip6

%: src/%.c
	$(CC) $? -Wall -Wextra -o $@

clean:
	rm -f tunslip tapslip6 tunslip6 senslip sentapslip6
