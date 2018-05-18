#!/bin/bash

VIDEO=$2
directory=$1
i=1

if [[ "$#" -le 2 ]]
then
	echo "Usage: $0 directoryName Path/to/videoFile [list of desired bitrates]"
	exit
fi

mkdir $directory
for var in "$@"
do
	if [[ "$var" == "$1" || "$var" == "$2" ]]
	then
		echo .
	else
		if [[ "$var" -lt 1000  ]]
		then
			echo lower than 1000
#			in 360p
			x264 --output intermediate.264 --fps 25 --preset slow --bitrate $var --vbv-maxrate $((2*$var)) --vbv-bufsize $((4*$var)) --min-keyint 50 --keyint 50 --scenecut 0 --no-scenecut --pass 1 --video-filter "resize:width=640,height=360" $VIDEO

		else
			if [[ "$var" -lt 2000 ]]
			then
				echo lower than 2000
#				in 480p
				x264 --output intermediate.264 --fps 25 --preset slow --bitrate $var --vbv-maxrate $((2*$var)) --vbv-bufsize $((4*$var)) --min-keyint 50 --keyint 50 --scenecut 0 --no-scenecut --pass 1 --video-filter "resize:width=640,height=480" $VIDEO
			else
				if [[ "$var" -lt 3000 ]]
				then
					echo lower than 3000
#					in 720p
					x264 --output intermediate.264 --fps 25 --preset slow --bitrate $var --vbv-maxrate $((2*$var)) --vbv-bufsize $((4*$var)) --min-keyint 50 --keyint 50 --scenecut 0 --no-scenecut --pass 1 --video-filter "resize:width=1280,height=720" $VIDEO
				else
					if [[ "$var" -lt 5000 ]]
					then
						echo lower than 5000
#						in 1080p
						x264 --output intermediate.264 --fps 25 --preset slow --bitrate $var --vbv-maxrate $((2*$var)) --vbv-bufsize $((4*$var)) --min-keyint 50 --keyint 50 --scenecut 0 --no-scenecut --pass 1 --video-filter "resize:width=1920,height=1080" $VIDEO
					else
						if [[ "$var" -lt 8000 ]]
						then
							echo lower than 8000
#							in 2K (1440p)
							x264 --output intermediate.264 --fps 25 --preset slow --bitrate $var --vbv-maxrate $((2*$var)) --vbv-bufsize $((4*$var)) --min-keyint 50 --keyint 50 --scenecut 0 --no-scenecut --pass 1 --video-filter "resize:width=2560,height=1440" $VIDEO
						else
							echo higher than 8000
#							in 4K (2160p)
							x264 --output intermediate.264 --fps 25 --preset slow --bitrate $var --vbv-maxrate $((2*$var)) --vbv-bufsize $((4*$var)) --min-keyint 50 --keyint 50 --scenecut 0 --no-scenecut --pass 1 --video-filter "resize:width=3840,height=2160" $VIDEO
						fi
					fi
				fi
			fi
		fi
		MP4Box -add intermediate.264 -fps 25 output_${var}k.mp4
		mkdir $directory/$var
		mv output_${var}k.mp4 $directory/$var
		cd $directory
		MP4Box -dash 2000 -frag 2000 -rap -segment-name $var/seg_ $var/output_${var}k.mp4
		mv output_${var}k_dash.mpd $var/
		if [[ $i -eq 1 ]]
		then
			echo "i = 1"
			if [[ $# -eq 3  ]] #only one quality
			then
				cat $var/output_${var}k_dash.mpd > mpd
			else
				head -n -3 $var"/output_"${var}"k_dash.mpd" | sed -e 's/Period/BaseURL\>http:\/\/webserver\/get\/$directory\<\/BaseURL\>\n\<Period/' > mpd
			fi
		else
			if [[ $i -eq  $(($# - 2)) ]]
			then
				echo "LAST ONE"
				tail -n $(($(wc -l $var/output_${var}k_dash.mpd | awk '{print $1}') - $(grep -m 1 -n Representation $var/output_${var}k_dash.mpd | awk '{print $1}' | cut -d: -f1) + 2)) $var/output_${var}k_dash.mpd | sed -e "s/id=\"1\"/id=\"${i}\"/" >> mpd
			else
				echo "i > 1"
				tail -n $(($(wc -l $var/output_${var}k_dash.mpd | awk '{print $1}') - $(grep -m 1 -n Representation $var/output_${var}k_dash.mpd | awk '{print $1}' | cut -d: -f1) + 2)) $var/output_${var}k_dash.mpd | head -n -3 | sed -e 's/id="1"/id="${i}"/' >> mpd
			fi
		fi
		cd ..
		i=$(($i+1))
	fi
done

tar -cvf $directory.tar $directory
