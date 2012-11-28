FILE = WSN430_FreeRTOS_Tutorial

all: ${FILE}.pdf


%.pdf: %.tex
	pdflatex $^


clean:
	rm -f *.aux *.dvi *.log *.toc
realclean: clean
	rm -f ${FILE}.pdf


