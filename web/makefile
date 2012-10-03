
.PHONY: all clean online sync


index.xhtml : pandoc-template.xhtml index.markdown
	pandoc --template=pandoc-template.xhtml -t html -s -o $@ index.markdown

online : index.xhtml
	make -C ../sem dist all
	mkdir -p online/tools/sem/downloads
	tar -C copy -c . --exclude-vcs | tar -C online/tools/sem -x
	cp -t online/tools/sem index.xhtml ../sem/doc/sem.html ../sem/COPYING
	cp -t online/tools/sem/downloads ../sem/sem-*
	chPerm -f 644 -d 755 online
	firefox online/tools/sem/index.xhtml

sync : online
	xmessage -buttons "yes:0,no:1" 'are you sure?'
	rsync -avzP -c --timeout=30 -e ssh online/ stefan-klinger.de:web/www
