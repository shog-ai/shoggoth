.PHONY:

install:
ifneq ($(wildcard ~/shogstudio/),)
	echo "Shog Studio is already installed in $$HOME/shogstudio/"
	echo "moving $$HOME/shogstudio/ to $$HOME/shogstudio_old/"
	rm -rf ~/shogstudio_old/
	mv ~/shogstudio/ ~/shogstudio_old/
endif
	
	rm -rf ~/shogstudio/
	
	unzip -o -q $(TARGET_DIR)/studio-release-$(VERSION)-$(OS)-$(ARCH).zip -d ~/shogstudio/
	
	cd ~/shogstudio/ && ./scripts/install.sh


uninstall:
	cd ~/shogstudio/ && ./scripts/uninstall.sh
