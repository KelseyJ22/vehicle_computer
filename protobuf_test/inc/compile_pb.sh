# Compiles protobufs in this directory.
# Rerun this script every time the .proto file is edited.

echo 'Compiling protobuf file:'
../../../nanopb-0.3.6-windows-x86/generator-bin/protoc --nanopb_out=. test.proto

echo 'Copying remaining necessary files:'
mv test.pb.c ../src
