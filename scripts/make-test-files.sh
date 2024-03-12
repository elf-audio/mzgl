#!/bin/bash

sample_rates=(22050 44100 48000 88200 96000)
bit_depths=(8 16 24 32)
channel_counts=(1 2)
frequency=440
volume="-3dB"
formats=("wav" "aiff" "mp3" "m4a")
endians=("signed" "signed-integer")
script_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
output_dir=${script_dir}/../test-files
valid_files_dir=${output_dir}/valid-files
invalid_files_dir=${output_dir}/invalid-files

get_channel_type() {
    local channels="$1"
    if [ "$channels" -eq 1 ]; then
        echo "Mono"
    elif [ "$channels" -eq 2 ]; then
        echo "Stereo"
    else
        echo "Unsupported channel count"
    fi
}

get_endian_type() {
    local endian="$1"
    if [ "$endian" = "signed" ]; then
        echo "LE"
    else
        echo "BE"
    fi
}

if ! command -v sox &> /dev/null; then
    echo "SoX is not installed. Installing SoX using Homebrew..."
    brew install sox
fi

if ! command -v ffmpeg &> /dev/null; then
    echo "FFmpeg is not installed. Installing FFmpeg using Homebrew..."
    brew install ffmpeg
fi

for sr in "${sample_rates[@]}"; do
    for bd in "${bit_depths[@]}"; do
        for fmt in "${formats[@]}"; do
            for channels in "${channel_counts[@]}"; do
                for endian in "${endians[@]}"; do
                    if [[ "$fmt" == "mp3" && "$sr" -gt 48000 ]]; then
                        continue
                    fi
                    if [[ "$fmt" == "m4a" && "$bd" -lt 16 ]]; then
                        continue
                    fi
                    if [[ "$fmt" == "m4a" && "$sr" -lt 44100 ]]; then
                        continue
                    fi
                    channel_type=$(get_channel_type "$channels")
                    endian_type=$(get_endian_type "$endian")
                    filename="${sr}Hz - ${bd}Bits - ${endian_type} - ${channel_type}.${fmt}"
                    outfile="${valid_files_dir}/${filename}"
                    sox -n -r "$sr" -b "$bd" -e signed -c "$channels" "${outfile%.*}".raw synth 1 sine ${frequency} vol ${volume}
                    echo "Generating $outfile"
                    if [ "$fmt" == "m4a" ]; then
                        ffmpeg -hide_banner -f s${bd}le -ar "$sr" -ac "$channels" -i "${outfile%.*}".raw -strict -2 "$outfile" >/dev/null 2>&1
                    else
                        sox -e "$endian" -b "$bd" -r "$sr" -c "$channels" "${outfile%.*}".raw "$outfile" rate -v -L
                    fi
                    touch "${invalid_files_dir}/${filename}"
                    rm "${outfile%.*}".raw
                done
            done
        done
    done
done