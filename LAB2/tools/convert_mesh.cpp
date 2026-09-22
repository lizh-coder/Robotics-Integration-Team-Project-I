#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <iostream>

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "usage: convert_mesh INPUT.dae OUTPUT.obj\n";
    return 2;
  }

  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(
      argv[1], aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                   aiProcess_GenSmoothNormals | aiProcess_PreTransformVertices);
  if (!scene) {
    std::cerr << importer.GetErrorString() << '\n';
    return 1;
  }

  Assimp::Exporter exporter;
  // OBJ keeps explicit vertex/normal indices and avoids Assimp 5.0's STL
  // normal-buffer mismatch for this particular multi-mesh Collada file.
  const aiReturn result = exporter.Export(scene, "objnomtl", argv[2]);
  if (result != aiReturn_SUCCESS) {
    std::cerr << exporter.GetErrorString() << '\n';
    return 1;
  }

  std::cout << "meshes=" << scene->mNumMeshes
            << " materials=" << scene->mNumMaterials << '\n';
  return 0;
}
