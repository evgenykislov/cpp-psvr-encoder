#include <iostream>
#include <sstream>

#include <stdlib.h>

#include <boost/filesystem.hpp>

extern "C" {
#include <libavformat/avformat.h>
}

namespace fs = boost::filesystem;

enum VideoType {
  kVType_SBS,
  kVType_OU,
  kVType_MONO,
  kVType_2D
};

enum TargetPlatform {
  kTarget_PSVR
};

const int kExtraWidth = 2560;

static fs::path g_input_file_;
static fs::path g_output_path_;
static fs::path g_output_file_;

static std::string g_target_platform_str_ = "psvr";
static TargetPlatform g_target_platform_ = kTarget_PSVR;
static std::string g_degree_str_ = "0";
static unsigned long g_degree_ = 0;
static std::string g_video_type_str_ = "mono";
static VideoType g_video_type_ = kVType_MONO;
static int g_video_width_ = 0;


void PrintHelp() {
  std::cout << "" << std::endl;
  std::cout << "Required Parameters:" << std::endl;
  std::cout << "  -i input_file - source file for encoding" << std::endl;
  std::cout << "Optional Parameters:" << std::endl;
  std::cout << "  -d degree - video degree. 0 (default) - for flat frame" << std::endl;
  std::cout << "  -o output_folder - folder for output file. Default: forlder of source file" << std::endl;
  std::cout << "  -t video_type - Available types: sbs, ou, mono, 2d. Default: mono" << std::endl;
}

void ParseArguments(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    std::string at = argv[i];
    int ni = i + 1;
    bool niv = ni < argc;
    if (at == "-d" && niv) {
      g_degree_str_ = argv[ni];
      ++i;
      continue;
    }
    if (at == "-i" && niv) {
      g_input_file_ = argv[ni];
      ++i;
      continue;
    }
    if (at == "-o" && niv) {
      g_output_path_ = argv[ni];
      ++i;
      continue;
    }
    if (at == "-t" && niv) {
      g_video_type_str_ = argv[ni];
      ++i;
      continue;
    }
  }
}

bool CheckArguments() {
  // Input file
  if (g_input_file_.empty()) {
    std::cerr << "Input file isn't specified" << std::endl;
    return false;
  }
  if (!fs::is_regular_file(g_input_file_)) {
    // File isn't exist
    std::cerr << "Input file isn't exist" << std::endl;
    return false;
  }

  // Degree
  const char* dgr_begin = g_degree_str_.c_str();
  const char* dgr_end = dgr_begin + g_degree_str_.size();
  char* real_end = nullptr;
  g_degree_ = strtoul(dgr_begin, &real_end, 10);
  if (real_end != dgr_end) {
    std::cerr << "Wrong video degree argument" << std::endl;
    return false;
  }
  if (!(g_degree_ == 0 || g_degree_ == 180 || g_degree_ == 360)) {
    std::cerr << "Wrong video degree value" << std::endl;
    return false;
  }

  // Video Type
  if (g_video_type_str_ == "sbs") {
    g_video_type_ = kVType_SBS;
  }
  else if (g_video_type_str_ == "ou") {
    g_video_type_ = kVType_OU;
  }
  else if (g_video_type_str_ == "mono") {
    g_video_type_ = kVType_MONO;
  }
  else if (g_video_type_str_ == "2d") {
    g_video_type_ = kVType_2D;
  }
  else {
    std::cerr << "Wrong type of video" << std::endl;
    return false;
  }

  // Output path/file
  if (g_output_path_.empty()) {
    g_output_path_ = g_input_file_.parent_path();
  }
  std::stringstream name;
  name << g_input_file_.stem().string() << "_" << g_target_platform_str_;
  switch (g_degree_) {
    case 0:
      if (g_video_type_ == kVType_MONO || g_video_type_ == kVType_2D) {
        name << "_2dff.mp4";
      }
      else if (g_video_type_ == kVType_SBS || g_video_type_ == kVType_OU) {
        name << "_3dff_" << g_video_type_str_ << ".mp4";
      }
      else {
        assert(false);
        std::cerr << "Logical error" << std::endl;
        return false;
      }
      break;
    case 180:
    case 360:
      name << "_" << g_degree_str_ + "_" << g_video_type_str_ << ".mp4";
      break;
  }
  g_output_file_ = g_output_path_ / name.str();

  return true;
}

bool GetVideoInfo() {
  auto ctx = avformat_alloc_context();
  if (!ctx) {
    std::cerr << "Memory error" << std::endl;
    return false;
  }
  if (avformat_open_input(&ctx, g_input_file_.c_str(), nullptr, nullptr) == 0) {
    if (avformat_find_stream_info(ctx, nullptr) >= 0) {
//      std::cout << "Duration: " << ctx->duration << std::endl;
//      std::cout << "Bitrate: " << ctx->bit_rate << std::endl;

      for (unsigned int i = 0; i < ctx->nb_streams; ++i) {
        auto& cdc = ctx->streams[i]->codecpar;
        if (cdc->codec_type != AVMEDIA_TYPE_VIDEO) {
          continue;
        }

        g_video_width_ = cdc->width;
//        std::cout << "Height: " << cdc->height << std::endl;
        break;
      }

    }
    avformat_close_input(&ctx);
  }
  avformat_free_context(ctx);

  if (g_video_width_ <= 0) {
    std::cerr << "Input file without video stream" << std::endl;
    return false;
  }

  return true;
}

bool SelectCodecs() {
  AVCodec* codec = nullptr;

  codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (!codec) {
    std::cerr << "There isn't codec h264" << std::endl;
    return false;
  }

  codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
  if (!codec) {
    std::cerr << "There isn't codec aac" << std::endl;
    return false;
  }

  return true;
}

void SetEncodingParameters(std::stringstream& cl) {
  switch (g_target_platform_) {
    case kTarget_PSVR:
      cl << " -f mp4";
      cl << " -vcodec libx264";
      cl << " -b:v 15000k";
      cl << " -ar 48000";
      cl << " -ac 2";
      cl << " -b:a 192k";
      cl << " -pix_fmt yuv420p";
      cl << " -profile:v High";
      cl << " -level:v 5.1";
      cl << " -bf 0";
      cl << " -slices 24";
      cl << " -refs 1";
      cl << " -threads 0";
      cl << " -x264opts no-cabac:aq-mode=1:aq-strength=0.7:slices=24:direct=spatial:me=tesa:subme=8:trellis=1";
      cl << " -flags +global_header";
      break;
  }

  cl << " -acodec aac";
  cl << " -map_metadata -1";
  int width = g_video_width_;
  if (width > kExtraWidth) {
    width  = kExtraWidth;
  }
  cl << " -vf scale=" << width << ":-1";
}

void RunEncoding() {
  std::stringstream cl;

  cl << "ffmpeg";

  // Input file
  cl << " -i " << g_input_file_;

  SetEncodingParameters(cl);
  // Output file
  cl << " " << g_output_file_;

  // Run convertation
  std::system(cl.str().c_str());
}

int main(int argc, char** argv)
{
  if (argc <= 1) {
    PrintHelp();
    return 1;
  }

  ParseArguments(argc, argv);
  if (!CheckArguments()) { return 1; }
  if (!SelectCodecs()) { return 1; }

  // Detect video parameters
  GetVideoInfo();
  RunEncoding();

  std::cout << std::endl << "Convertation has complited" << std::endl;
  return 0;
}
