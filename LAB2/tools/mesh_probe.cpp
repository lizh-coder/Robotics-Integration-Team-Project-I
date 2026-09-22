#include <ros/ros.h>
#include <rviz/ogre_helpers/render_system.h>
#include <rviz/mesh_loader.h>
#include <Ogre.h>
#include <iostream>

int main(int argc, char** argv) {
  ros::init(argc, argv, "lab2_mesh_probe", ros::init_options::AnonymousName);
  rviz::RenderSystem::get();
  auto mesh = rviz::loadMeshFromResource("package://two_drones_pkg/mesh/quadrotor.dae");
  if (mesh.isNull()) return 2;
  size_t vertices=0, indices=0;
  for(unsigned i=0;i<mesh->getNumSubMeshes();++i) {
    auto sub=mesh->getSubMesh(i);
    vertices += sub->vertexData->vertexCount;
    indices += sub->indexData->indexCount;
  }
  std::cout << "PROBE submeshes=" << mesh->getNumSubMeshes() << " vertices=" << vertices << " indices=" << indices << " bounds=" << mesh->getBounds() << std::endl;
  auto root=Ogre::Root::getSingletonPtr();
  auto scene=root->createSceneManager(Ogre::ST_GENERIC);
  scene->setAmbientLight(Ogre::ColourValue(0.6,0.6,0.6));
  auto light=scene->createLight("light"); light->setPosition(2,3,4);
  auto entity=scene->createEntity("drone",mesh->getName());
  scene->getRootSceneNode()->createChildSceneNode()->attachObject(entity);
  auto camera=scene->createCamera("camera");
  camera->setPosition(0.8,-1,0.8); camera->lookAt(0,0,0); camera->setNearClipDistance(0.01);
  camera->setAspectRatio(1);
  auto texture=Ogre::TextureManager::getSingleton().createManual("test",Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,Ogre::TEX_TYPE_2D,600,600,0,Ogre::PF_R8G8B8,Ogre::TU_RENDERTARGET);
  auto target=texture->getBuffer()->getRenderTarget();
  auto vp=target->addViewport(camera); vp->setBackgroundColour(Ogre::ColourValue(0.18,0.18,0.18));
  target->update();
  target->writeContentsToFile(argc>1?argv[1]:"/tmp/lab2-mesh-probe.png");
  std::cout << "PROBE rendered" << std::endl;
}
