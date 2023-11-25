mkdir -p ~/bin && cp ./scripts/shog.sh ~/bin/shog

if [ "$SHELL" = "/bin/bash" ]; then
    echo -e '\nexport PATH=$PATH:~/bin' >> ~/.bashrc
elif [ "$SHELL" = "/bin/zsh" ]; then
    echo -e '\nexport PATH=$PATH:~/bin' >> ~/.zshrc
elif [ "$SHELL" = "/usr/bin/fish" ]; then
    echo 'set PATH $PATH ~/bin' >> ~/.config/fish/config.fish
elif [ "$SHELL" = "/usr/local/bin/fish" ]; then
    echo 'set PATH $PATH ~/bin' >> ~/.config/fish/config.fish
else
    echo "Current shell is not bash, zsh or fish. ensure to manually add ~/bin to PATH"
fi

if [ "$SHELL" = "/bin/bash" ]; then
    mkdir -p ~/.bash_completion.d/ && cp ./scripts/shog_completions.sh ~/.bash_completion.d/
    # echo "Bash completions script copied to ~/.bash_completion.d/"
else
    echo "Current shell is not Bash. completion script not available."
fi

echo "Installation complete!"
echo ""

echo "To use the shog command, you may need to restart your current shell."
echo "This would reload your PATH environment variable to allow you use shog"

echo ""

echo "To configure your current shell, run:"

if [ "$SHELL" = "/bin/bash" ]; then
  echo "source ~/.bashrc"
elif [ "$SHELL" = "/bin/zsh" ]; then
  echo "source ~/.zshrc"
elif [ "$SHELL" = "/usr/bin/fish" ]; then
  echo "source ~/.config/fish/config.fish"
elif [ "$SHELL" = "/usr/local/bin/fish" ]; then
  echo "source ~/.config/fish/config.fish"
else
  echo "Current shell is not bash, zsh or fish. ensure to manually add ~/bin to PATH"
fi
