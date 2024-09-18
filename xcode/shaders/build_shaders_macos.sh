#!/bin/sh

GLSLANG=${VULKAN_SDK}/bin/glslang
INPUT_DIR=${PROJECT_DIR}/../shaders
OUTPUT_DIR=${BUILT_PRODUCTS_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/shaders

mkdir -p ${OUTPUT_DIR}

for path in ${INPUT_DIR}/*.glsl; do
    file_name=$(basename ${path} .glsl)
    echo ${GLSLANG} -V ${path} -o ${OUTPUT_DIR}/${file_name}.spv
    ${GLSLANG} -V ${path} -o ${OUTPUT_DIR}/${file_name}.spv
done









