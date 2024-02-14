#!/bin/bash

# Your music files or playlist
music_files=("$@")

# Command template, replace 'your_program' with the actual program you want to call
#cmd_template=(./fmtx --title=)
cmd_template='./fmtx --title="{}"'

pipe="/tmp/mplayer-control"


# Cleanup function to be executed on script exit
cleanup() {
    echo "Cleaning up..."
    echo "quit" > $pipe # Send quit command to MPlayer
    wait $mplayer_pid # Wait for MPlayer to exit
    rm -f $pipe # Remove the named pipe
}

mkfifo /tmp/mplayer-control

trap cleanup EXIT INT TERM

# Start MPlayer in verbose mode, playing the specified files, and parse its output
mplayer -slave -quiet -v -input file="${pipe}" -ao alsa:device=hw=1.0 "${music_files[@]}"  2>&1 | \
while IFS= read -r line; do
    # Check for the start of the clip info block
    if [[ "$line" == "Clip info:" ]]; then
        # Reset variables for safety
        title=""
        artist=""
        # Flag to start capturing
        capture_info=true
    fi

    # Capture title
    if [[ "$capture_info" = true && "$line" =~ ^\ Title:\ (.*) ]]; then
        title="${BASH_REMATCH[1]}"
    fi

    # Capture artist
    if [[ "$capture_info" = true && "$line" =~ ^\ Artist:\ (.*) ]]; then
        artist="${BASH_REMATCH[1]}"
    fi

    # End of the clip info block, execute your command
    if [[ "$line" =~ ^Load\ subtitles\ in.* ]]; then
        # Only proceed if both title and artist have been captured
        if [[ -n "$title" && -n "$artist" ]]; then
            # Replace placeholders in the command template
#	    escaped_input=$(printf "%q" "$artist-$title")
            cmd="${cmd_template//\{\}/"$artist-$title"}"
#            cmd="${cmd//\{\}/"$artist"}"

            # Execute the command
            eval "$cmd"
#	    cmd = cmd_template
#	    cmd+=("$artist-$title")
#	    "${cmd[@]}"
        fi
        # Reset capturing flag
        capture_info=false
    fi
done &
mplayer_pid=$!

# Example control loop in the background
(
    while true; do
        read -p "Command (next, prev, quit): " cmd
        case "$cmd" in
            next) echo "pt_step 1" > $pipe ;;
            prev) echo "pt_step -1" > $pipe ;;
            quit) echo "quit" > $pipe 
		    echo "Exiting..."
		    break ;; # Exit loop
            *) echo "Unknown command: $cmd" ;;
        esac
    done
)

# Wait for MPlayer and loop to finish
wait $mplayer_pid

rm -f $pipe
