#!/bin/bash
rm -f versionInfo

echo -n "string versionString1(\"" >>versionInfo
git log |sed -n '1p'|awk '{print $2,"\");"}'>>versionInfo
echo -n "string versionString2(\"git date: ">>versionInfo
git log --date=format:'%Y-%m-%d %H:%M:%S'|sed -n '3p'|awk '{print $2,$3"\");"}'>>versionInfo

cat versionInfo