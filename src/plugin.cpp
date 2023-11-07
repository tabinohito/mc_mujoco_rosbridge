#include <mujoco/mjplugin.h>

#include <mc_mujoco_rosbridge/MujocoSensors.h>

namespace mujoco::plugin::sensor
{

mjPLUGIN_LIB_INIT
{
  MujocoSensors::RegisterPlugin();
}

} // namespace mujoco::plugin::sensor
