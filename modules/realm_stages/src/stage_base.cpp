

#include <realm_core/loguru.h>

#include <realm_stages/stage_base.h>

using namespace realm;

StageBase::StageBase(const std::string &name, const std::string &path, double rate, int queue_size)
: WorkerThreadBase("Stage [" + name + "]", static_cast<int64_t>(1/rate*1000.0), true),
  m_stage_name(name),
  m_stage_path(path),
  m_queue_size(queue_size),
  m_is_output_dir_initialized(false),
  m_t_statistics_period(10),
  m_counter_frames_in(0),
  m_counter_frames_out(0),
  m_timer_statistics_fps(new Timer(std::chrono::seconds(m_t_statistics_period), std::bind(&StageBase::evaluateFpsStatistic, this)))
{
}

bool StageBase::changeParam(const std::string &name, const std::string &val)
{
  LOG_F(WARNING, "Changing parameter not implemented for this stage!");
  return false;
}

void StageBase::initStagePath(const std::string &abs_path)
{
  // Set and create output directory
  m_stage_path = abs_path + "/" + m_stage_name;
  initStageCallback();
  m_is_output_dir_initialized = true;

  // Init logging
  loguru::add_file((m_stage_path + "/stage.log").c_str(), loguru::Append, loguru::Verbosity_MAX);

  LOG_F(INFO, "Successfully initialized!");
  LOG_F(INFO, "Stage path set to: %s", m_stage_path.c_str());
  printSettingsToLog();
}

void StageBase::registerAsyncDataReadyFunctor(const std::function<bool()> &func)
{
  // Return true if either the functor evaluates to true, or when a finish is requested.
  m_data_ready_functor = ([=]{ return (func() || isFinishRequested()); });
}

void StageBase::registerFrameTransport(const std::function<void(const Frame::Ptr&, const std::string&)> &func)
{
  m_transport_frame = func;
}

void StageBase::registerPoseTransport(const std::function<void(const cv::Mat &, uint8_t zone, char band, const std::string &)> &func)
{
  m_transport_pose = func;
}

void StageBase::registerDepthMapTransport(const std::function<void(const cv::Mat&, const std::string&)> &func)
{
  m_transport_depth_map = func;
}

void StageBase::registerPointCloudTransport(const std::function<void(const cv::Mat&, const std::string&)> &func)
{
  m_transport_pointcloud = func;
}

void StageBase::registerImageTransport(const std::function<void(const cv::Mat&, const std::string&)> &func)
{
  m_transport_img = func;
}

void StageBase::registerMeshTransport(const std::function<void(const std::vector<Face>&, const std::string&)> &func)
{
  m_transport_mesh = func;
}

void StageBase::registerCvGridMapTransport(const std::function<void(const CvGridMap &, uint8_t zone, char band, const std::string&)> &func)
{
  m_transport_cvgridmap = func;
}

void StageBase::setStatisticsPeriod(uint32_t s)
{
    std::unique_lock<std::mutex> lock(m_mutex_statistics_fps);
  m_t_statistics_period = s;
}

void StageBase::updateFpsStatisticsIncoming()
{
    std::unique_lock<std::mutex> lock(m_mutex_statistics_fps);
    m_counter_frames_in++;
}

void StageBase::updateFpsStatisticsOutgoing()
{
    std::unique_lock<std::mutex> lock(m_mutex_statistics_fps);
    m_counter_frames_out++;
}

void StageBase::evaluateFpsStatistic()
{
    std::unique_lock<std::mutex> lock(m_mutex_statistics_fps);
    float fps_in = static_cast<float>(m_counter_frames_in) / m_t_statistics_period;
    float fps_out = static_cast<float>(m_counter_frames_out) / m_t_statistics_period;

  m_counter_frames_in = 0;
  m_counter_frames_out = 0;

    LOG_F(INFO, "FPS in: %f, out: %f", fps_in, fps_out);
}