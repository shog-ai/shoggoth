#!/bin/bash

_shog_completion() {
    local cur
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    options="--help --version --debug -c -r"
    commands="node client"
    client_commands="clone init publish id"
    node_commands="id run start stop restart status logs"

    case "$cur" in
        -*) 
            # Suggest options starting with '-'
            COMPREPLY=( $(compgen -W "$options" -- "$cur") )
            ;;
        *)
            case "${COMP_WORDS[1]}" in
                node)
                    # Suggest completions for 'node' command
                    COMPREPLY=( $(compgen -W "$node_commands" -- "$cur") )
                    ;;
                client)
                    # Suggest completions for 'client' command
                    COMPREPLY=( $(compgen -W "$client_commands" -- "$cur") )
                    ;;
                *)
                    # Suggest top-level commands
                    COMPREPLY=( $(compgen -W "$commands" -- "$cur") )
                    ;;
            esac
            ;;
    esac
}

complete -F _shog_completion shog
