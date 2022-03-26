#!/usr/bin/env bash

# Copyright (c) 2017, University of Kaiserslautern
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Author: Éder F. Zulian

DIR="$(cd "$(dirname "$0")" && pwd)"
TOPDIR=$DIR
source $TOPDIR/common/defaults.in
source $TOPDIR/common/util.in

toolchain=gcc-linaro-5.4.1-2017.05-x86_64_aarch64-linux-gnu
toolchaintarball=$toolchain.tar.xz

wgethis=(
# ARM full system files
"$FSDIRARM:http://dist.gem5.org/dist/current/arm/aarch-system-20180409.tar.xz"
# # X86 full system files
# "$FSDIRX86:http://www.m5sim.org/dist/current/x86/x86-system.tar.bz2"
# A toolchain for ARM
"$TOOLCHAINSDIR_ARM:https://releases.linaro.org/components/toolchain/binaries/5.4-2017.05/aarch64-linux-gnu/$toolchaintarball"
)

hgrepos=(
# Asimbench android disk images and vmlinux for arm
"$FSDIRARM,https://bitbucket.org/yongbing_huang/asimbench"
)

greetings
wget_into_dir wgethis[@]
hg_clone_into_dir hgrepos[@]

printf "Uncompressing files..."
pulse on
# sys="$FSDIRALPHA/m5_system_2.0b3"
# tb="$sys.tar.bz2"
# if [[ ! -d $sys ]]; then
# 	tar -xaf $tb -C $FSDIRALPHA
# fi

sys="$FSDIRARM/aarch-system-20180409"
tb="$sys.tar.xz"
if [[ ! -d $sys ]]; then
	mkdir -p $sys
	tar -xaf $tb -C $sys
fi

# sys="$FSDIRX86/x86-system"
# tb="$sys.tar.bz2"
# if [[ ! -d $sys ]]; then
# 	mkdir -p $sys
# 	tar -xaf $tb -C $sys
# fi

sys="$FSDIRARM/asimbench"
dpath="$sys/disks"
# mkdir -p $dpath
# if [[ ! -e $dpath/sdcard-1g-mxplayer.img ]]; then
# 	tb="$sys/asimbench_disk_image/sdcard-1g.tar.gz"
# 	tar -xaf $tb -C $dpath
# fi
# if [[ ! -e $dpath/ARMv7a-ICS-Android.SMP.Asimbench-v3.img ]]; then
# 	tb="$sys/asimbench_disk_image/ARMv7a-ICS-Android.SMP.Asimbench.tar.gz"
# 	tar -xaf $tb -C $dpath
# fi

toolchaindir=$TOOLCHAINSDIR_ARM/$toolchain
if [[ ! -d $toolchaindir ]]; then
	tar -xaf $TOOLCHAINSDIR_ARM/$toolchaintarball -C $TOOLCHAINSDIR_ARM
fi
pulse off