#!/bin/bash

for line in `find . -name "moc_*" 2>/dev/null`; do
	rm $line
done