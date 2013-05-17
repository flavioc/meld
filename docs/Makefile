
all: vm_format.pdf manual.pdf vm.pdf

vm_format.pdf: vm_format.tex
	pdflatex vm_format.tex

manual.pdf: manual.tex
	pdflatex manual.tex

vm.pdf: vm.tex
	pdflatex vm.tex

clean:
	rm -f vm_format.pdf vm_format.aux \
		vm_format.log manual.pdf manual.aux manual.log \
		vm.log vm.pdf vm.aux
