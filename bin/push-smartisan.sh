#!/bin/sh -
if [ X$1 != X ]
then
	git push ssh://xipeng.gu@111.207.121.242:29418/$1 HEAD:refs/for/simt/phoenix-rom
else
	echo "Please input the name of the repository!!"
fi
