#!/bin/bash

shell_name=$(basename "$SHELL")

mkdir -p ~/bin && cp ./scripts/shogstudio.sh ~/bin/shogstudio

case "$shell_name" in
  bash)
    echo -e '\nexport PATH=$PATH:~/bin' >> ~/.bashrc
    ;;
  zsh)
    echo -e '\nexport PATH=$PATH:~/bin' >> ~/.zshrc
    ;;
  fish)
    echo 'set PATH $PATH ~/bin' >> ~/.config/fish/config.fish
    ;;
  *)
    echo "Current shell is not bash, zsh, or fish. Ensure to manually add ~/bin to PATH"
    ;;
esac

if [ "$shell_name" = "bash" ]; then
    echo "completions not implemented"
else
    echo "completion script is not available for $shell_name."
fi

echo "Installation complete!"
echo ""

echo "To use the shogstudio command, you may need to restart your current shell."
echo "This would reload your PATH environment variable to allow you use shogstudio"

echo ""

echo "To configure your current shell, run:"

case "$shell_name" in
  bash)
    echo "source ~/.bashrc"
    ;;
  zsh)
    echo "source ~/.zshrc"
    ;;
  fish)
    echo "source ~/.config/fish/config.fish"
    ;;
  *)
    echo "unrecognized shell. ensure to manually update your PATH in the shell configuration."
    ;;
esac
