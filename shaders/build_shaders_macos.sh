#!/bin/sh

GLSLANG=${VULKAN_SDK}/bin/glslang
INPUT_DIR=${PROJECT_DIR}/../shaders
OUTPUT_DIR=${BUILT_PRODUCTS_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/shaders

mkdir -p ${OUTPUT_DIR}

files=("test.vert.glsl" "test.frag.glsl")
for file in ${files[@]}; do
    file_name=$(basename ${file} .glsl)
    echo ${GLSLANG} -V ${INPUT_DIR}/${file} -o ${OUTPUT_DIR}/${file_name}.spv
    ${GLSLANG} -V ${INPUT_DIR}/${file} -o ${OUTPUT_DIR}/${file_name}.spv
done
