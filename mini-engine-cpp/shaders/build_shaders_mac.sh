#!/bin/sh

mkdir -p ${BUILT_PRODUCTS_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/shaders

${VULKAN_SDK}/bin/glslang -V ${PROJECT_DIR}/../shaders/triangle.vert.glsl -o ${BUILT_PRODUCTS_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/shaders/triangle.vert.spv
${VULKAN_SDK}/bin/glslang -V ${PROJECT_DIR}/../shaders/triangle.frag.glsl -o ${BUILT_PRODUCTS_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/shaders/triangle.frag.spv

${VULKAN_SDK}/bin/glslang -V ${PROJECT_DIR}/../shaders/color_triangle.vert.glsl -o ${BUILT_PRODUCTS_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/shaders/color_triangle.vert.spv
${VULKAN_SDK}/bin/glslang -V ${PROJECT_DIR}/../shaders/color_triangle.frag.glsl -o ${BUILT_PRODUCTS_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/shaders/color_triangle.frag.spv

${VULKAN_SDK}/bin/glslang -V ${PROJECT_DIR}/../shaders/tri_mesh.vert.glsl -o ${BUILT_PRODUCTS_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/shaders/tri_mesh.vert.spv
