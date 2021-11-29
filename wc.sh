
#!/bin/bash

WC="$(find -E lib/mzgl -regex '.*\.(h|cpp|m|mm)' -exec cat {} + | wc -l)"
TIME="$(date +'%F %H:%M')"

echo "${TIME}, ${WC}" >> wc.csv

echo "LINE COUNT: ${WC}"

