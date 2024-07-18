#!/bin/bash

# Debug toggle
GT_DEBUG=0

# Debug function
gt_debug() {
    if [[ $GT_DEBUG -eq 1 ]]; then
        printf "Debug: $1\n" >&2
    fi
}

# Function to change directory
gt() {
    if [ $# -eq 0 ]; then
        echo "Usage: gt <directory>"
        return 1
    fi

    local target="$1"
    local search_type="immediate"

    # Check for '^' suffix
    if [[ "$target" == *\^ ]]; then
        target="${target%\^}"  # Remove '^'
        # Strip any number of leading "../" occurrences
        target="${target#"${target%%[!../]*}"}"
        # Add a single "../" prefix
        target="../$target"
        search_type="search"
    fi

    # Check for '../' prefix
    if [[ "$target" == ../* ]]; then
        search_type="search"
    fi

    local matches

    if [[ "$search_type" == "search" ]]; then
        matches=$(gts search "$target")
    elif [[ "$target" == /* ]]; then
        matches="$target"
    else
        matches=$(gts immediate "../$target")
    fi

    if [ -z "$matches" ]; then
        echo "No matching directories found."
        return 1
    fi

    local match_count=$(echo "$matches" | wc -l)
    if [ $match_count -eq 1 ]; then
        if [ -d "$matches" ]; then
            cd "$matches"
        else
            echo "Directory not found: $matches"
            return 1
        fi
    else
        echo "Multiple matches found:"
        echo "$matches" | nl
        echo "Enter the number of the directory to change to (or any other key to cancel):"
        read choice
        if [[ "$choice" =~ ^[0-9]+$ ]] && [ "$choice" -le "$match_count" ]; then
            local selected_dir=$(echo "$matches" | sed -n "${choice}p")
            if [ -d "$selected_dir" ]; then
                cd "$selected_dir"
            else
                echo "Directory not found: $selected_dir"
                return 1
            fi
        else
            echo "Selection cancelled."
        fi
    fi
}

# Completion function for gt
_gt_completion() {
    local cur

    if [ -n "$ZSH_VERSION" ]; then
        # Zsh
        if [[ -n "$CURRENT_WORD" ]]; then
            cur="$CURRENT_WORD"
        elif [[ -n "${words[CURRENT]}" ]]; then
            cur="${words[CURRENT]}"
        elif [[ -n "$PREFIX" ]]; then
            cur="$PREFIX"
        else
            cur="${${(z)BUFFER}[$CURRENT]}"
        fi
    else
        # Bash
        cur="${COMP_WORDS[COMP_CWORD]}"
    fi

    gt_debug "Captured current word: '$cur'"

    # Handle '^' suffix for upward search
    if [[ "$cur" == *\^ ]]; then
        gt_debug "Matched '^' suffix"
        cur="${cur%^}"  # Remove '^'
        # Strip any number of leading "../" occurrences
        cur="${cur#"${cur%%[!../]*}"}"
        cur="../$cur"
        gt_debug "Modified current word: '$cur'"
    else
        gt_debug "No '^' suffix detected"
    fi

    local matches

    if [[ -z "$cur" ]]; then
        gt_debug "Calling gts immediate with empty string"
        matches=$(gts immediate "" 2>/dev/null)
    else
        gt_debug "Calling gt search with '$cur'"
        matches=$(gts search "$cur" 2>/dev/null)
    fi

    gt_debug "Raw matches: $matches"

    if [ -n "$matches" ]; then
        if [ -n "$ZSH_VERSION" ]; then
            local -a match_array
            match_array=(${(f)matches})
            compadd -U -q -d match_array -a match_array
            gt_debug "Zsh completions added: ${#match_array[@]} items"
        else
            # Bash
            readarray -t COMPREPLY < <(compgen -W "$matches" -- "$cur")
            gt_debug "Bash completions added: ${#COMPREPLY[@]} items"
        fi
    else
        gt_debug "No matches found"
        [ -n "$ZSH_VERSION" ] && _message "No matching directories found"
    fi
}

# For Bash
if [ -n "$BASH_VERSION" ]; then
    complete -F _gt_completion gt
fi

# For Zsh
if [ -n "$ZSH_VERSION" ]; then
    autoload -Uz compinit && compinit
    compdef _gt_completion gt

    # Zsh-specific settings for better completion display
    zstyle ':completion:*' menu select
    zstyle ':completion:*' list-colors ''
    zstyle ':completion:*:descriptions' format '%F{yellow}-- %d --%f'
    zstyle ':completion:*:messages' format '%d'
    zstyle ':completion:*:warnings' format '%F{red}No matches for:%f %d'
    zstyle ':completion:*' group-name ''
    
    # Force menu completion
    zstyle ':completion:*' insert-tab false
    bindkey '^I' menu-complete
fi
