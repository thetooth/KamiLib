SUBDIRS = buildnumber kami crossplatform-test
CLEANDIRS = $(SUBDIRS)

.PHONY: subdirs $(SUBDIRS) clean

subdirs: $(SUBDIRS)

all: $(SUBDIRS)
$(SUBDIRS):
	@echo -e "\x1b[32;01mBuilding $@...\x1b[0m"
	$(MAKE) -C $@ --no-print-directory

clean:
	@for DIR in $(SUBDIRS); do (cd $$DIR; $(MAKE) clean --no-print-directory ); done