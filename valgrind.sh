#!/bin/sh
valgrind --tool=memcheck --leak-check=full --show-reachable=yes --track-origins=yes -v --log-file=valgrind.log ./wxMailto
