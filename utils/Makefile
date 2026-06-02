SUBDIRS := featured-list-creator lvl-icon-extractor lvlbuf-decompressor lvledit package-creator progress-get

.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
