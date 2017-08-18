# volumetric
Flow metering application example for the MAX35103EVKIT2.

## Overview

This application example shows how to measure flow using the flow body included in the MAX35103EVKIT2 and control a standard 24VAC residential water value (not included in the kit).

Please see [volumetric/MAX35103EVKIT2_VOLUMETRIC.PDF](https://github.com/maxim-ic-flow/volumetric/blob/master/MAX35103EVKIT2_VOLUMETRIC.pdf) for details.

## Repository

Please note that this project uses git submodules.  The proper way to clone this repository is as follows:

```
git clone --recursive https://github.com/maxim-ic-flow/volumetric.git
```
To switch between branches:

```
git checkout <branch>
git submodule update --recursive --remote --init
```

## Branches

<i>master</i> contains IAR and uVision projects.
<p><i>mbed</i> contains a mbed-cli project which is currently under development.  Portions of this repo are not yet publicly available.

## Tools

<b>IAR Embedded Workbench for ARM 7.70+</b>
<p>https://www.iar.com/iar-embedded-workbench/tools-for-arm/arm-cortex-m-edition/

<b>Keil uVision V5.23.0.0+</b>
<p>http://www2.keil.com/mdk5

<b>ARM mbed</b>
<p>Not yet available.
