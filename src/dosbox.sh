#! /bin/sh

if [ -z "$@" ]; then
	exec ./dosbox
else
	extension="${@##*.}"
	case "${extension}" in
	[cC][oO][nN][fF]) exec ./dosbox -exit -userconf -conf "$@";;
	*) exec ./dosbox -exit "$@";;
	esac
fi
