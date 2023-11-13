.PHONY:

install:
	rm -rf ~/shoggoth/
	
	unzip -o -q $(TARGET_DIR)/shoggoth-release-$(VERSION)-$(OS)-$(ARCH).zip -d ~/shoggoth/
	
	cd ~/shoggoth/ && ./scripts/install.sh


uninstall:
	cd ~/shoggoth/ && ./scripts/uninstall.sh
