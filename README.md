# rcast_media_player
Cast Media Player used by rCast ROM for Chromecast

This code implements the minimal subset of the Cast protocol that is needed
to control a cast receiver running on the Chromecast device.
In addition the program reads input from the button on the Chromecast and 
controls the receiver that comes with the rCast ROM

The target for this program is the Chromecast device, but since I have not 
found a proper toolchain for that platform everything has been built with
the Android NDK toolchain and static linking. Because of this the build 
scripts will need some manual editing before it will compile. See below.

This program has dependencies on OpenSSL and Protobuf and also the 
Android NDK toolchain.
You will need to edit the build shell scripts with the paths for all the 
dependencies before you build anything. See build_native.sh and build_arm.sh

