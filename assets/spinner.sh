#!/bin/bash

# spinner.sh

COLOR_BLUE="\033[1;34m"
COLOR_GREEN="\033[1;32m"
COLOR_GOLD="\033[1;33m"
COLOR_WHITE="\033[1;37m"
COLOR_RESET="\033[0m"

spinner() {
  local pid=$1
  local delay=0.1
  local spinstr='|/-\'
  local color=$COLOR_BLUE

  while kill -0 "$pid" 2>/dev/null; do
    local temp=${spinstr#?}
    printf " [$color%c$COLOR_RESET]  " "$spinstr"
    spinstr=$temp${spinstr%"$temp"}
    sleep $delay
    printf "\b\b\b\b\b\b\b\b"
    
    # Cycle colors
    if [ "$color" = "$COLOR_BLUE" ]; then
      color=$COLOR_GREEN
    elif [ "$color" = "$COLOR_GREEN" ]; then
      color=$COLOR_GOLD
    elif [ "$color" = "$COLOR_GOLD" ]; then
      color=$COLOR_WHITE
    else
      color=$COLOR_BLUE
    fi
  done

  printf "$COLOR_RESET"
  printf "    \b\b\b\b"
}

spinner $1

