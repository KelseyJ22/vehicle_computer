# Compiles protobufs in this directory.
# Rerun this script every time the .proto file is edited.

for file in ../lib/protobuf/*.proto; do
    proto_file="${file##*/}"
    extension="${proto_file##*.}"
    filename="${proto_file%.*}"

    echo Compiling protobuf file $filename.proto:
    nanopb_bin/protoc --proto_path=../lib/protobuf --nanopb_out=. ../lib/protobuf/$filename.proto

    echo Moving $filename.pb.* into protobuf folder:
    mv $filename.pb.h ../lib/protobuf/
    mv $filename.pb.c ../lib/protobuf/
done

# echo 'Compiling protobuf file:'
# nanopb_bin/protoc --proto_path=../lib/protobuf --nanopb_out=. ../lib/protobuf/test.proto

# echo 'Moving protobuf files into protobuf folder:'
# mv test.pb.h ../lib/protobuf/
# mv test.pb.c ../lib/protobuf/
