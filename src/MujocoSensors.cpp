#include <mujoco/mujoco.h>
#include <mc_mujoco_rosbridge/MujocoSensors.h>
#include <iostream>

namespace mujoco::plugin::sensor
{

void MujocoSensors::RegisterPlugin()
{
  mjpPlugin plugin;
  mjp_defaultPlugin(&plugin);

  plugin.name = "MujocoSensorsPlugin";
  plugin.capabilityflags |= mjPLUGIN_SENSOR;

  const char * attributes[] = {""};
  plugin.nattribute = 1;
  plugin.attributes = attributes;

  plugin.nstate = +[](const mjModel *, // m
                      int // plugin_id
                   ) { return 0; };

  plugin.nsensordata = +[](const mjModel * m, int plugin_id, int // sensor_id
                        ){ return 0;};

  // Can only run after forces have been computed
  plugin.needstage = mjSTAGE_ACC;

  plugin.init = +[](const mjModel * m, mjData * d, int plugin_id)
  {
    auto * plugin_instance = MujocoSensors::Create(m, d, plugin_id);
    if(!plugin_instance)
    {
      return -1;
    }
    d->plugin_data[plugin_id] = reinterpret_cast<uintptr_t>(plugin_instance);
    return 0;
  };

  plugin.destroy = +[](mjData * d, int plugin_id)
  {
    delete reinterpret_cast<MujocoSensors *>(d->plugin_data[plugin_id]);
    d->plugin_data[plugin_id] = 0;
  };

  plugin.reset = +[](const mjModel * m, double *, // plugin_state
                     void * plugin_data, int plugin_id)
  {
    auto * plugin_instance = reinterpret_cast<class MujocoSensors *>(plugin_data);
    plugin_instance->reset(m, plugin_id);
  };

  plugin.compute = +[](const mjModel * m, mjData * d, int plugin_id, int // capability_bit
                    )
  {
    auto * plugin_instance = reinterpret_cast<class MujocoSensors *>(d->plugin_data[plugin_id]);
    plugin_instance->compute(m, d, plugin_id);
  };

  mjp_registerPlugin(&plugin);
}

MujocoSensors * MujocoSensors::Create(const mjModel * m, mjData * d, int plugin_id)
{
  // Set sensor_id
  int sensor_id = 0;
  for(; sensor_id < m->nsensor; sensor_id++)
  {
    if(m->sensor_type[sensor_id] == mjSENS_PLUGIN && m->sensor_plugin[sensor_id] == plugin_id)
    {
      break;
    }
  }
  if(sensor_id == m->nsensor)
  {
    mju_error("[MujocoSensors] Plugin not found in sensors.");
    return nullptr;
  }
  
  std::cout << "[MujocoSensors] Create." << std::endl;

  return new MujocoSensors(m, d, sensor_id);
}

MujocoSensors::MujocoSensors(const mjModel * m,
                             mjData *, // d
                             int sensor_id)
: sensor_id_(sensor_id), site_id_(m->sensor_objid[sensor_id])
{
    int argc = 0;
    char ** argv = nullptr;
    if(!ros::isInitialized())
    {
        ros::init(argc, argv, "mujoco_ros", ros::init_options::NoSigintHandler);
    }

    nh_ = std::make_shared<ros::NodeHandle>();
    initSensors(m); 
}

MujocoSensors::~MujocoSensors()
{

}

void MujocoSensors::initSensors(const mjModel * model){
    std::string sensor_name, site, frame_id;
    for (int n = 0; n < model->nsensor; n++) {
        int adr       = model->sensor_adr[n];
        int site_id   = model->sensor_objid[n];
        int parent_id = model->site_bodyid[site_id];
        int type      = model->sensor_type[n];
        
        //load only candidate sensors
        if(SENSOR_STRING.find(type) != SENSOR_STRING.end()){
            site = mj_id2name(model, model->sensor_objtype[n], site_id);

            if (model->names[model->name_sensoradr[n]]) {
                sensor_name = mj_id2name(model, mjOBJ_SENSOR, n);
            } else {
                std::cerr << "Sensor name resolution error. Skipping sensor of type " << type << " on site " << site << std::endl;
                continue;
            }

            // Global frame sensors
            bool global_frame = false;
            frame_id          = "world";
            switch (type) {
                {
                    case mjSENS_FRAMEXAXIS:
                    case mjSENS_FRAMEYAXIS:
                    case mjSENS_FRAMEZAXIS:
                    case mjSENS_FRAMELINVEL:
                    case mjSENS_FRAMELINACC:
                    case mjSENS_FRAMEANGACC:
                        int refid = model->sensor_refid[n];
                        if (refid != -1) {
                            int reftype = model->sensor_reftype[n];
                            if (reftype == mjOBJ_SITE) {
                                refid   = model->site_bodyid[refid];
                                reftype = mjOBJ_BODY;
                            }
                            frame_id = mj_id2name(model, reftype, refid);
                            std::cerr << "Sensor has relative frame with id " << refid << " and type "
                                                                                                << reftype << " and ref_frame "
                                                                                                << frame_id << std::endl;
                        }
                        sensor_map_[sensor_name] =
                            std::pair(nh_->advertise<geometry_msgs::Vector3Stamped>(topic_ + sensor_name, 1, true), frame_id);
                        break;
                }
                case mjSENS_SUBTREECOM:
                case mjSENS_SUBTREELINVEL:
                case mjSENS_SUBTREEANGMOM:
                    sensor_map_[sensor_name] =
                        std::pair(nh_->advertise<geometry_msgs::Vector3Stamped>(topic_ + sensor_name, 1, true), frame_id);
                    global_frame = true;
                    break;
                    {
                        case mjSENS_FRAMEPOS:
                            int refid = model->sensor_refid[n];
                            if (refid != -1) {
                                int reftype = model->sensor_reftype[n];
                                if (reftype == mjOBJ_SITE) {
                                    refid   = model->site_bodyid[refid];
                                    reftype = mjOBJ_BODY;
                                }
                                frame_id = mj_id2name(model, reftype, refid);
                                std::cerr << "Sensor has relative frame with id "
                                                                    << refid << " and type " << reftype << " and ref_frame "
                                                                    << frame_id << std::endl;
                            }
                            sensor_map_[sensor_name] =
                                std::pair(nh_->advertise<geometry_msgs::PointStamped>(topic_ + sensor_name, 1, true), frame_id);
                            global_frame = true;
                            break;
                    }

                case mjSENS_BALLQUAT:
                case mjSENS_FRAMEQUAT:
                    sensor_map_[sensor_name] =
                        std::pair(nh_->advertise<geometry_msgs::QuaternionStamped>(topic_ + sensor_name, 1, true), frame_id);
                    global_frame = true;
                    break;
            }

            // Check if sensor is in global frame and already setup
            if (global_frame || frame_id != "world") {
                std::cerr << "Setting up sensor " << sensor_name << " on site " << site << " (frame_id: "
                                                                    << frame_id << ") of type " << SENSOR_STRING.at(type) << std::endl;
                continue;
            }

            frame_id = mj_id2name(model, mjOBJ_BODY, parent_id);
            if(SENSOR_STRING.count(type)){
                std::cerr << "Setting up sensor " << sensor_name << " on site " << site << " (frame_id: "
                                                    << frame_id << ") of type " << SENSOR_STRING.at(type) << std::endl;
            }
            
            switch (type) {
                case mjSENS_ACCELEROMETER:
                case mjSENS_VELOCIMETER:
                case mjSENS_GYRO:
                case mjSENS_FORCE:
                case mjSENS_TORQUE:
                case mjSENS_MAGNETOMETER:
                case mjSENS_BALLANGVEL:
                    sensor_map_[sensor_name] =
                        std::pair(nh_->advertise<geometry_msgs::Vector3Stamped>(topic_ + sensor_name, 1, true), frame_id);
                    break;

                case mjSENS_TOUCH:
                case mjSENS_RANGEFINDER:
                case mjSENS_JOINTPOS:
                case mjSENS_JOINTVEL:
                case mjSENS_TENDONPOS:
                case mjSENS_TENDONVEL:
                case mjSENS_ACTUATORPOS:
                case mjSENS_ACTUATORVEL:
                case mjSENS_ACTUATORFRC:
                case mjSENS_JOINTLIMITPOS:
                case mjSENS_JOINTLIMITVEL:
                case mjSENS_JOINTLIMITFRC:
                case mjSENS_TENDONLIMITPOS:
                case mjSENS_TENDONLIMITVEL:
                case mjSENS_TENDONLIMITFRC:
                    sensor_map_[sensor_name] =
                        std::pair(nh_->advertise<mc_mujoco_rosbridge::ScalarStamped>(topic_ + sensor_name, 1, true), frame_id);
                    break;

                default:
                    std::cerr << "Sensor of type '" << type << "' (" << sensor_name
                                                                        << ") is unknown! Cannot publish to ROS" << std::endl;
                    break;
            }
        }
    }
}


void MujocoSensors::reset(const mjModel *, // m
                          int // plugin_id
)
{
}

void MujocoSensors::compute(const mjModel * m, mjData * d, int // plugin_id
)
{
    mjMARKSTACK;
    ros::Publisher pub;
    std::string frame_id, sensor_name;

    int adr, type;
    mjtNum cutoff;

    for (int n = 0; n <  m->nsensor; n++) {
        adr    =  m->sensor_adr[n];
        type   =  m->sensor_type[n];
        cutoff = ( m->sensor_cutoff[n] > 0 ?  m->sensor_cutoff[n] : 1);

        if ( m->names[ m->name_sensoradr[n]]) {
            sensor_name = mj_id2name(m, mjOBJ_SENSOR, n);
        } else {
            continue;
        }

        if (sensor_map_.find(sensor_name) == sensor_map_.end())
            continue;

        std::tie(pub, frame_id) = sensor_map_[sensor_name];

        switch (type) {
            {
                case mjSENS_FRAMELINVEL:
                case mjSENS_FRAMELINACC:
                case mjSENS_FRAMEANGACC:
                case mjSENS_SUBTREECOM:
                case mjSENS_SUBTREELINVEL:
                case mjSENS_SUBTREEANGMOM:
                case mjSENS_ACCELEROMETER:
                case mjSENS_VELOCIMETER:
                case mjSENS_GYRO:
                case mjSENS_FORCE:
                case mjSENS_TORQUE:
                case mjSENS_MAGNETOMETER:
                case mjSENS_BALLANGVEL:
                case mjSENS_FRAMEXAXIS:
                case mjSENS_FRAMEYAXIS:
                case mjSENS_FRAMEZAXIS:
                    geometry_msgs::Vector3Stamped msg;
                    msg.header.frame_id = frame_id;
                    msg.header.stamp    = ros::Time::now();
                    msg.vector.x        = (float)( d->sensordata[adr] / cutoff);
                    msg.vector.y        = (float)( d->sensordata[adr + 1] / cutoff);
                    msg.vector.z        = (float)( d->sensordata[adr + 2] / cutoff);
                    pub.publish(msg);
                    break;
            }

            case mjSENS_FRAMEPOS: {
                geometry_msgs::PointStamped msg;
                msg.header.frame_id = frame_id;
                msg.header.stamp    = ros::Time::now();
                msg.point.x         = (float)( d->sensordata[adr] / cutoff);
                msg.point.y         = (float)( d->sensordata[adr + 1] / cutoff);
                msg.point.z         = (float)( d->sensordata[adr + 2] / cutoff);
                pub.publish(msg);
                break;
            }

                {
                    case mjSENS_TOUCH:
                    case mjSENS_RANGEFINDER:
                    case mjSENS_JOINTPOS:
                    case mjSENS_JOINTVEL:
                    case mjSENS_TENDONPOS:
                    case mjSENS_TENDONVEL:
                    case mjSENS_ACTUATORPOS:
                    case mjSENS_ACTUATORVEL:
                    case mjSENS_ACTUATORFRC:
                    case mjSENS_JOINTLIMITPOS:
                    case mjSENS_JOINTLIMITVEL:
                    case mjSENS_JOINTLIMITFRC:
                    case mjSENS_TENDONLIMITPOS:
                    case mjSENS_TENDONLIMITVEL:
                    case mjSENS_TENDONLIMITFRC:
                        mc_mujoco_rosbridge::ScalarStamped msg;
                        msg.header.frame_id = frame_id;
                        msg.header.stamp    = ros::Time::now();
                        msg.value           = (float)( d->sensordata[adr] / cutoff);
                        pub.publish(msg);
                        break;
                }

            case mjSENS_BALLQUAT: {
                case mjSENS_FRAMEQUAT:
                    geometry_msgs::QuaternionStamped msg;
                    msg.header.frame_id = frame_id;
                    msg.header.stamp    = ros::Time::now();
                    msg.quaternion.w    = (float)( d->sensordata[adr] / cutoff);
                    msg.quaternion.x    = (float)( d->sensordata[adr + 1] / cutoff);
                    msg.quaternion.y    = (float)( d->sensordata[adr + 2] / cutoff);
                    msg.quaternion.z    = (float)( d->sensordata[adr + 3] / cutoff);
                    pub.publish(msg);
                    break;
            }

            default:
                std::cerr << 
                    "Sensor publisher and frame_id defined but type can't be serialized. This shouldn't happen! ("
                        << sensor_name << " of type " << type << ")" << std::endl;
                break;
        }
    }

  mjFREESTACK;
} 
}// namespace mujoco::plugin::sensor