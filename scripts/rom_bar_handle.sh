#!/bin/bash

while IFS= read -r -n1 byte; do
    echo -n "$(printf "%02X" \'$byte)"
done < PHGOP.rom
