
.PHONY: all doc configuration examples

DOCFOLDER = doc


all: configuration doc examples


configuration:
	@ if [ "${WSN430_DRIVERS_PATH}" != "$(CURDIR)" ]; then echo '$$WSN430_DRIVERS_PATH does not point to drivers directory'; echo 'Please set variable to current directory. Add the following line to your .bashrc'; echo ''; echo 'export WSN430_DRIVERS_PATH="$(CURDIR)"'; echo ''; exit 1; fi

doc:
	doxygen Doxyfile $(DOCFOLDER)

examples:
	cd examples; ./compile_all.sh

clean:
	rm -r $(DOCFOLDER)
