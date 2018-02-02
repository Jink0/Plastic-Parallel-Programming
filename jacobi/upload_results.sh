#!/bin/bash
zip -r runs.zip runs/
scp -r runs.zip root@GoldenEye.ydns.eu:/mnt/user/Data
