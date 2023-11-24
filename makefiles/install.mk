.PHONY:

install:
ifneq ($(wildcard ~/shoggoth/),)
	echo "Shoggoth is already installed in $$HOME/shoggoth/"
	echo "moving $$HOME/shoggoth/ to $$HOME/shoggoth_old/"
	rm -rf ~/shoggoth_old/
	mv ~/shoggoth/ ~/shoggoth_old/
endif
	
	rm -rf ~/shoggoth/
	
	unzip -o -q $(TARGET_DIR)/shoggoth-release-$(VERSION)-$(OS)-$(ARCH).zip -d ~/shoggoth/
	
	cd ~/shoggoth/ && ./scripts/install.sh


uninstall:
	cd ~/shoggoth/ && ./scripts/uninstall.sh
