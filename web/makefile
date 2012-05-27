
.PHONY: all clean sync sem.tar.bz2

all : index.xhtml

clean :
	rm -rf online index.xhtml sem.tar.bz2

online : index.xhtml sem.tar.bz2
	mkdir -p online/tools/sem
	tar -C copy -c . --exclude-vcs | tar -C online/tools/sem -vx
	cp -t online/tools/sem index.xhtml sem.tar.bz2 ../sem/long.txt ../sem/COPYING
	chPerm -f 644 -d 755 online
	firefox online/tools/sem/index.xhtml

index.xhtml : pandoc-template.xhtml index.txt
	pandoc --template=pandoc-template.xhtml -t html -s -o $@ index.txt

sem.tar.bz2 :
	$(MAKE) -C ../sem clean
	tar -C .. -c sem --exclude-vcs | bzip2 > $@

sync : online
	xmessage -buttons "yes:0,no:1" 'are you sure?'
	rsync -avzP -c --timeout=30 -e ssh online/ stefan-klinger.de:web/www
